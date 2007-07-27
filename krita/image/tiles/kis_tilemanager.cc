/*
 *  Copyright (c) 2005-2006 Bart Coppens <kde@bartcoppens.be>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <kdebug.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <cassert>

#include <QMutex>
#include <QThread>
#include <qfile.h>

#include <kstaticdeleter.h>
#include <kglobal.h>
#include <ksharedconfig.h>

#include "kis_tileddatamanager.h"
#include "kis_tile.h"
#include "kis_tilemanager.h"

#define DO_NOT_PRINT_INFO

// Note: the cache file doesn't get deleted when we crash and so :(

KisTileManager* KisTileManager::m_singleton = 0;

static KStaticDeleter<KisTileManager> staticDeleter;

KisTileManager::KisTileManager()
    : m_bigKritaLock( QMutex::Recursive )
{

    Q_ASSERT(KisTileManager::m_singleton == 0);
    KisTileManager::m_singleton = this;
    m_bytesInMem = 0;
    m_bytesTotal = 0;
    m_swapForbidden = false;

    // Hardcoded (at the moment only?): 4 pools of 1000 tiles each
    m_tilesPerPool = 100;

    m_pools = new quint8*[4];
    m_poolPixelSizes = new qint32[4];
    m_poolFreeList = new PoolFreeList[4];
    for (int i = 0; i < 4; i++) {
        m_pools[i] = 0;
        m_poolPixelSizes[i] = 0;
        m_poolFreeList[i] = PoolFreeList();
    }
    m_currentInMem = 0;

    KConfigGroup cfg = KGlobal::config()->group("");
    m_maxInMem = cfg.readEntry("maxtilesinmem",  4000);
    m_swappiness = cfg.readEntry("swappiness", 100);

    m_tileSize = KisTile::WIDTH * KisTile::HEIGHT;
    for (int i = 0; i < 8; i++) {
        m_freeLists.push_back(FreeList());
    }

    counter = 0;
}

KisTileManager::~KisTileManager() {
    if (!m_freeLists.empty()) { // See if there are any nonempty freelists
        FreeListList::iterator listsIt = m_freeLists.begin();
        FreeListList::iterator listsEnd = m_freeLists.end();

        while(listsIt != listsEnd) {
            if ( ! (*listsIt).empty() ) {
                FreeList::iterator it = (*listsIt).begin();
                FreeList::iterator end = (*listsIt).end();

                while (it != end) {
                    delete *it;
                    ++it;
                }
                (*listsIt).clear();
            }
            ++listsIt;
        }
        m_freeLists.clear();
    }

    for (FileList::iterator it = m_files.begin(); it != m_files.end(); ++it) {
        (*it).tempFile->close();
        (*it).tempFile->setAutoRemove(true);
        delete (*it).tempFile;
    }

    delete [] m_poolPixelSizes;
    delete [] m_pools;
    // Where did this go to? delete [] m_poolFreeList;
}

KisTileManager* KisTileManager::instance()
{
    if(KisTileManager::m_singleton == 0) {
        staticDeleter.setObject(KisTileManager::m_singleton, new KisTileManager()); // threadsafe?
        Q_CHECK_PTR(KisTileManager::m_singleton);
    }
    return KisTileManager::m_singleton;
}

void KisTileManager::registerTile(KisTile* tile)
{
    m_bigKritaLock.lock();

    TileInfo* info = new TileInfo();
    info->tile = tile;
    info->inMem = true;
    info->mmapped = false;
    info->onFile = false;
    info->file = 0;
    info->filePos = 0;
    info->size = tile->WIDTH * tile->HEIGHT * tile->m_pixelSize;
    info->fsize = 0; // the size in the file
    info->validNode = true;

    m_tileMap[tile] = info;
    m_swappableList.push_back(info);
    info->node = -- m_swappableList.end();

    m_currentInMem++;
    m_bytesTotal += info->size;
    m_bytesInMem += info->size;

    doSwapping();

#ifndef DO_NOT_PRINT_INFO
    if (++counter % 50 == 0)
        printInfo();
#endif

    m_bigKritaLock.unlock();

}

void KisTileManager::deregisterTile(KisTile* tile) {

    if (!m_tileMap.contains(tile)) {
            return;
    }
    // Q_ASSERT(m_tileMap.contains(tile));
    m_bigKritaLock.lock();

    TileInfo* info = m_tileMap[tile];

    if (info->onFile) { // It was once mmapped
        // To freelist
        FreeInfo* freeInfo = new FreeInfo();
        freeInfo->file = info->file;
        freeInfo->filePos = info->filePos;
        freeInfo->size = info->fsize;
        uint pixelSize = (info->size / m_tileSize);

        // It is still mmapped?
        if (info->mmapped) {
            // munmap it
            munmap(info->tile->m_data, info->size);
            m_bytesInMem -= info->size;
            m_currentInMem--;
        }

        if (m_freeLists.size() <= static_cast<int>(pixelSize))
            m_freeLists.resize(pixelSize + 1);
        m_freeLists[pixelSize].push_back(freeInfo);

        // the KisTile will attempt to delete its data. This is of course silly when
        // it was mmapped. So change the m_data to NULL, which is safe to delete
        tile->m_data = 0;
    } else {
        m_bytesInMem -= info->size;
        m_currentInMem--;
    }

    if (info->validNode) {
        m_swappableList.erase(info->node);
        info->validNode = false;
    }

    m_bytesTotal -= info->size;

    delete info;
    m_tileMap.remove(tile);

    doSwapping();

    m_bigKritaLock.unlock();
}

void KisTileManager::ensureTileLoaded(const KisTile* tile)
{
    m_bigKritaLock.lock();

    TileInfo* info = m_tileMap[tile];
    if (info->validNode) {
        m_swappableList.erase(info->node);
        info->validNode = false;
    }

    if (!info->inMem) {
        fromSwap(info);
    }
    m_bigKritaLock.unlock();

}

void KisTileManager::maySwapTile(const KisTile* tile)
{
    m_bigKritaLock.lock();

    TileInfo* info = m_tileMap[tile];
    m_swappableList.push_back(info);
    info->validNode = true;
    info->node = -- m_swappableList.end();

    doSwapping();
    m_bigKritaLock.unlock();
}

quint8* KisTileManager::requestTileData(qint32 pixelSize)
{
    if ( pixelSize > 10 )
        return new quint8[ m_tileSize * pixelSize ];
    else {
        m_bigKritaLock.lock();
        quint8* data = findTileFor(pixelSize);
        m_bigKritaLock.unlock();
        if ( !data ) {
            data = new quint8[m_tileSize * pixelSize];
        }
        return data;
    }
}

void KisTileManager::dontNeedTileData(quint8* data, qint32 pixelSize)
{
    m_bigKritaLock.lock();
    if (isPoolTile(data, pixelSize)) {
        reclaimTileToPool(data, pixelSize);
    } else
        delete[] data;
    m_bigKritaLock.unlock();

}

void KisTileManager::configChanged() {
    m_bigKritaLock.lock();
    KConfigGroup cfg = KGlobal::config()->group("");
    m_maxInMem = cfg.readEntry("maxtilesinmem",  4000);
    m_swappiness = cfg.readEntry("swappiness", 100);

    doSwapping();
    m_bigKritaLock.unlock();
}


// =================== PRIVATE, SO NO LOCKS NEEDED ====================

void KisTileManager::toSwap(TileInfo* info)
{

    //Q_ASSERT(info->inMem);
    if (!info || !info->inMem) {
        return;
    }

    KisTile *tile = info->tile;

    if (!info->onFile) {
        // This tile is not yet in the file. Save it there
        uint pixelSize = (info->size / m_tileSize);
        bool foundFree = false;

        if (m_freeLists.size() > static_cast<int>(pixelSize)) {
            if (!m_freeLists[pixelSize].empty()) {
                // found one
                FreeList::iterator it = m_freeLists[pixelSize].begin();

                info->file = (*it)->file;
                info->filePos = (*it)->filePos;
                info->fsize = (*it)->size;

                delete *it;
                m_freeLists[pixelSize].erase(it);

                foundFree = true;
            }
        }

        if (!foundFree) { // No position found or free, create a new
            long pagesize = sysconf(_SC_PAGESIZE);
            TempFile* tfile = 0;
            if (m_files.empty() || m_files.back().fileSize >= MaxSwapFileSize) {
                m_files.push_back(TempFile());
                tfile = &(m_files.back());
                tfile->tempFile = new KTemporaryFile();
                tfile->tempFile->setAutoRemove(false);
                tfile->tempFile->open();
                tfile->fileSize = 0;
            } else {
                tfile = &(m_files.back());
            }
            off_t newsize = tfile->fileSize + info->size;
            newsize = newsize + newsize % pagesize;

            if (ftruncate(tfile->tempFile->handle(), newsize)) {
                // XXX make these maybe i18n()able and in an error box, but then through
                // some kind of proxy such that we don't pollute this with GUI code
                kWarning(DBG_AREA_TILES) << "Resizing the temporary swapfile failed!" << endl;
                // Be somewhat pollite and try to figure out why it failed
                switch (errno) {
                    case EIO: kWarning(DBG_AREA_TILES) << "Error was E IO, "
                            << "possible reason is a disk error!" << endl; break;
                    case EINVAL: kWarning(DBG_AREA_TILES) << "Error was E INVAL, "
                            << "possible reason is that you are using more memory than "
                            << "the filesystem or disk can handle" << endl; break;
                    default: kWarning(DBG_AREA_TILES) << "Errno was: " << errno << endl;
                }
                kWarning(DBG_AREA_TILES) << "The swapfile is: " << tfile->tempFile->fileName() << endl;
                kWarning(DBG_AREA_TILES) << "Will try to avoid using the swap any further" << endl;

                kDebug(DBG_AREA_TILES) << "Failed ftruncate info: "
                        << "tried adding " << info->size << " bytes "
                        << "(rounded to pagesize: " << newsize << ") "
                        << "from a " << tfile->fileSize << " bytes file" << endl;
#ifndef DO_NOT_PRINT_INFO
                printInfo();
#endif
                m_swapForbidden = true;
                return;
            }

            info->file = tfile->tempFile;
            info->fsize = info->size;
            info->filePos = tfile->fileSize;
            tfile->fileSize = newsize;
        }

        //memcpy(data, tile->m_data, info->size);
        QFile* file = info->file;
        if(!file) {
            kWarning() << "Opening the file as QFile failed" << endl;
            m_swapForbidden = true;
            return;
        }

        int fd = file->handle();
        quint8* data = 0;
        if (!kritaMmap(data, 0, info->size, PROT_READ | PROT_WRITE, MAP_SHARED,
             fd, info->filePos)) {
            kWarning() << "Initial mmap failed" << endl;
            m_swapForbidden = true;
            return;
        }

        memcpy(data, info->tile->m_data, info->size);
        munmap(data, info->size);

        if (isPoolTile(tile->m_data, tile->m_pixelSize))
            reclaimTileToPool(tile->m_data, tile->m_pixelSize);
        else
            delete[] tile->m_data;

        tile->m_data = 0;
    } else {
        //madvise(info->tile->m_data, info->fsize, MADV_DONTNEED);
        Q_ASSERT(info->mmapped);

        // munmap it
        munmap(tile->m_data, info->size);
        tile->m_data = 0;
    }

    info->inMem = false;
    info->mmapped = false;
    info->onFile = true;

    m_currentInMem--;
    m_bytesInMem -= info->size;

}

void KisTileManager::doSwapping()
{
    if (m_swapForbidden || m_currentInMem <= m_maxInMem) {
        return;
    }

#if 1 // enable this to enable swapping

    quint32 count = qMin(quint32(m_swappableList.size()), m_swappiness);

    for (quint32 i = 0; i < count && !m_swapForbidden; i++) {
        toSwap(m_swappableList.front());
        m_swappableList.front()->validNode = false;
        m_swappableList.pop_front();
    }

#endif

}

void KisTileManager::printInfo()
{
#ifndef DO_NOT_PRINT_INFO
    kDebug(DBG_AREA_TILES) << m_bytesInMem << " out of " << m_bytesTotal << " bytes in memory\n";
    kDebug(DBG_AREA_TILES) << m_currentInMem << " out of " << m_tileMap.size() << " tiles in memory\n";
    kDebug(DBG_AREA_TILES) << m_files.size() << " swap files in use" << endl;
    kDebug(DBG_AREA_TILES) << m_swappableList.size() << " elements in the swapable list\n";
    kDebug(DBG_AREA_TILES) << "Freelists information\n";
    for (int i = 0; i < m_freeLists.size(); i++) {
        if ( ! m_freeLists[i].empty() ) {
            kDebug(DBG_AREA_TILES) << m_freeLists[i].size()
                    << " elements in the freelist for pixelsize " << i << "\n";
        }
    }
    kDebug(DBG_AREA_TILES) << "Pool stats (" <<  m_tilesPerPool << " tiles per pool)" << endl;
    for (int i = 0; i < 4; i++) {
        if (m_pools[i]) {
            kDebug(DBG_AREA_TILES) << "Pool " << i << ": Freelist count: " << m_poolFreeList[i].count()
                    << ", pixelSize: " << m_poolPixelSizes[i] << endl;
        }
    }
    if (m_swapForbidden)
        kDebug(DBG_AREA_TILES) << "Something was wrong with the swap, see above for details" << endl;
    kDebug(DBG_AREA_TILES) << endl;
#endif
}


quint8* KisTileManager::findTileFor(qint32 pixelSize)
{
    for (int i = 0; i < 4; i++) {
        if (m_poolPixelSizes[i] == pixelSize) {
            if (!m_poolFreeList[i].isEmpty()) {
                quint8* data = m_poolFreeList[i].front();
                m_poolFreeList[i].pop_front();
                return data;
            }
        }
        if (m_pools[i] == 0) {
            // allocate new pool
            m_poolPixelSizes[i] = pixelSize;
            try {
                m_pools[i] = new quint8[pixelSize * m_tileSize * m_tilesPerPool];
            }
            catch ( std::bad_alloc ) {
                kDebug() << ">>>>>>> Could not allocated memory " << pixelSize << " " << m_tileSize << " " << m_tilesPerPool << endl;
                // XXX: bart! What shall we do here?
                abort();
            }

            // j = 1 because we return the first element, so no need to add it to the freelist
            for (int j = 1; j < m_tilesPerPool; j++)
                m_poolFreeList[i].append(&m_pools[i][j * pixelSize * m_tileSize]);
            return m_pools[i];
        }
    }
    return 0;
}

bool KisTileManager::isPoolTile(quint8* data, qint32 pixelSize) {

    if (data == 0) {
        return false;
    }

    for (int i = 0; i < 4; i++) {
        if (m_poolPixelSizes[i] == pixelSize) {
            bool b = data >= m_pools[i]
                     && data < m_pools[i] + pixelSize * m_tileSize * m_tilesPerPool;
            if (b) {
                return true;
            }
        }
    }
    return false;
}

void KisTileManager::reclaimTileToPool(quint8* data, qint32 pixelSize) {

    for (int i = 0; i < 4; i++) {
        if (m_poolPixelSizes[i] == pixelSize)
            if (data >= m_pools[i] && data < m_pools[i] + pixelSize * m_tileSize * m_tilesPerPool) {
                m_poolFreeList[i].append(data);
            }
    }
}


bool KisTileManager::kritaMmap(quint8*& result, void *start, size_t length,
                               int prot, int flags, int fd, off_t offset) {
    result = (quint8*) mmap(start, length, prot, flags, fd, offset);

            // Same here for warning and GUI
    if (result == (quint8*)-1) {
        kWarning(DBG_AREA_TILES) << "mmap failed: errno is " << errno << "; we're probably going to crash very soon now...\n";

        // Try to ignore what happened and carry on, but unlikely that we'll get
        // much further, since the file resizing went OK and this is memory-related...
        if (errno == ENOMEM) {
            kWarning(DBG_AREA_TILES) << "mmap failed with E NOMEM! This means that "
                    << "either there are no more memory mappings available for Krita, "
                    << "or that there is no more memory available!" << endl;
        }

        kWarning(DBG_AREA_TILES) << "Trying to continue anyway (no guarantees)" << endl;
        kWarning(DBG_AREA_TILES) << "Will try to avoid using the swap any further" << endl;
        kDebug(DBG_AREA_TILES) << "Failed mmap info: "
                << "tried mapping " << length << " bytes" << endl;
        if (!m_files.empty()) {
            kDebug(DBG_AREA_TILES) << "Probably to a " << m_files.back().fileSize << " bytes file" << endl;
        }
        printInfo();

        assert(false);

        // Be nice
        result = 0;

        return false;
    }

    return true;
}

void KisTileManager::fromSwap(TileInfo* info)
{

    if (info->inMem) {
        return;
    }

    doSwapping();

    Q_ASSERT(info->onFile);
    Q_ASSERT(info->file);
    Q_ASSERT(!info->mmapped);

    if (!kritaMmap(info->tile->m_data, 0, info->size, PROT_READ | PROT_WRITE, MAP_SHARED,
                   info->file->handle(), info->filePos)) {
        kWarning() << "fromSwap failed!" << endl;
        return;
    }

    info->inMem = true;
    info->mmapped = true;

    m_currentInMem++;
    m_bytesInMem += info->size;

}

