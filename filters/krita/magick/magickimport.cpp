/*
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <magickimport.h>
#include <kgenericfactory.h>
#include <koDocument.h>
#include <koFilterChain.h>
#include <kis_doc.h>

typedef KGenericFactory<MagickImport, KoFilter> MagickImportFactory;
K_EXPORT_COMPONENT_FACTORY(libmagickimport, MagickImportFactory("magickimport"));

MagickImport::MagickImport(KoFilter *, const char *, const QStringList&) : KoFilter()
{
}

MagickImport::~MagickImport()
{
}

KoFilter::ConversionStatus MagickImport::convert(const QCString&, const QCString& to)
{
	if (to != "application/x-krita")
		return KoFilter::BadMimeType;

	KisDoc *input = dynamic_cast<KisDoc*>(m_chain -> outputDocument());
	QString inputFilename = m_chain -> inputFile();
	
	if (!input)
		return KoFilter::CreationError;
	
	printf("inputFilename = %s\n", inputFilename.latin1());
	printf("input = %p\n", input);
	return input -> importImage(inputFilename) ? KoFilter::OK : KoFilter::BadMimeType;
}

#include <magickimport.moc>

