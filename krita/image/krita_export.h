/*
    This file is part of krita
    Copyright (c) 2005 KOffice Team

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/



#ifndef KRITA_EXPORT_H
#define KRITA_EXPORT_H

#include <kdemacros.h>

/* We use _WIN32/_WIN64 instead of Q_OS_WIN so that this header can be used from C files too */
#if defined(_WIN32) || defined(_WIN64)


#ifndef KRITA_EXPORT
# ifdef MAKE_KRITA_LIB
#  define KRITA_EXPORT KDE_EXPORT
# else
#  define KRITA_EXPORT KDE_IMPORT
# endif
#endif

#ifndef KRITAUI_EXPORT
# ifdef MAKE_KRITAUI_LIB
#  define KRITAUI_EXPORT KDE_EXPORT
# else
#  define KRITAUI_EXPORT KDE_IMPORT
# endif
#endif

#ifndef KRITACORE_EXPORT
# ifdef MAKE_KRITACORE_LIB
#  define KRITACORE_EXPORT KDE_EXPORT
# else
#  define KRITACORE_EXPORT KDE_IMPORT
# endif
#endif

#ifndef KRITATOOL_EXPORT
# ifdef MAKE_KRITATOOL_LIB
#  define KRITATOOL_EXPORT KDE_EXPORT
# else
#  define KRITATOOL_EXPORT KDE_IMPORT
# endif
#endif

#ifndef KRITAPAINT_EXPORT
# ifdef MAKE_KRITAPAINT_LIB
#  define KRITAPAINT_EXPORT KDE_EXPORT
# else
#  define KRITAPAINT_EXPORT KDE_IMPORT
# endif
#endif

#ifndef KRITAIMAGE_EXPORT
# ifdef MAKE_KRITAIMAGE_LIB
#  define KRITAIMAGE_EXPORT KDE_EXPORT
# else
#  define KRITAIMAGE_EXPORT KDE_IMPORT
# endif
#endif

#ifndef KRITASCRIPTING_EXPORT
# ifdef MAKE_KRITASCRIPTING_LIB
#  define KRITASCRIPTING_EXPORT KDE_EXPORT
# else
#  define KRITASCRIPTING_EXPORT KDE_IMPORT
# endif
#endif

#ifndef KROSSKRITACORE_EXPORT
# ifdef MAKE_KROSSKRITACORE_LIB
#  define KROSSKRITACORE_EXPORT KDE_EXPORT
# else
#  define KROSSKRITACORE_EXPORT KDE_IMPORT
# endif
#endif

#else // not windows

#define KRITA_EXPORT KDE_EXPORT
#define KRITACOLOR_EXPORT KDE_EXPORT
#define KRITAUI_EXPORT KDE_EXPORT
#define KRITATOOL_EXPORT KDE_EXPORT
#define KRITAPAINT_EXPORT KDE_EXPORT
#define KRITAIMAGE_EXPORT KDE_EXPORT
#define KRITASCRIPTING_EXPORT KDE_EXPORT
#define KROSSKRITACORE_EXPORT KDE_EXPORT
#endif /* not windows */

#endif /* KRITA_EXPORT_H */
