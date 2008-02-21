/*
 *  Copyright (c) 2007 Cyrille Berger <cberger@cberger.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "kis_meta_data_schema.h"
#include <QDebug>
#include <QString>

using namespace KisMetaData;

const QString Schema::TIFFSchemaUri = "http://ns.adobe.com/tiff/1.0/";
const QString Schema::EXIFSchemaUri = "http://ns.adobe.com/exif/1.0/";
const QString Schema::DublinCoreSchemaUri = "http://purl.org/dc/elements/1.1/";
const QString Schema::XMPSchemaUri = "http://ns.adobe.com/xap/1.0/";
const QString Schema::XMPRightsSchemaUri = "http://ns.adobe.com/xap/1.0/rights/";
const QString Schema::XMPMediaManagementUri = "http://ns.adobe.com/xap/1.0/sType/ResourceRef#";
const QString Schema::MakerNoteSchemaUri = "http://www.koffice.org/krita/xmp/MakerNote/1.0";
const QString Schema::IPTCSchemaUri = "http://iptc.org/std/Iptc4xmpCore/1.0/xmlns/";
const QString Schema::PhotoshopSchemaUri = "http://ns.adobe.com/photoshop/1.0/";


struct Schema::Private {
    QString uri;
    QString prefix;
};

Schema::Schema(const QString & _uri, const QString & _ns) : d(new Private)
{
    d->uri = _uri;
    d->prefix = _ns;
}

Schema::~Schema()
{
    delete d;
}

QString Schema::uri() const
{
    return d->uri;
}

QString Schema::prefix() const
{
    return d->prefix;
}

QString Schema::generateQualifiedName(const QString & name) const
{
    return prefix() + ':' + name;
}

QDebug operator<<(QDebug dbg, const KisMetaData::Schema &c)
{
    dbg.nospace() << "Uri = " << c.uri() << " Prefix = " << c.prefix();
    return dbg.space();
}

// ---- Schema Registry ---- //

struct SchemaRegistry::Private {
    static SchemaRegistry *singleton;
    QHash<QString, Schema*> uri2Schema;
    QHash<QString, Schema*> prefix2Schema;
};

SchemaRegistry *SchemaRegistry::Private::singleton = 0;

SchemaRegistry* SchemaRegistry::instance()
{
    if(SchemaRegistry::Private::singleton == 0)
    {
        SchemaRegistry::Private::singleton = new SchemaRegistry();
    }
    return SchemaRegistry::Private::singleton;
}

SchemaRegistry::SchemaRegistry() : d(new Private)
{
    create( Schema::TIFFSchemaUri, "tiff");
    create( Schema::EXIFSchemaUri, "exif");
    create( Schema::DublinCoreSchemaUri, "dc");
    create( Schema::XMPSchemaUri, "xmp");
    create( Schema::XMPRightsSchemaUri, "xmpRights");
    create( Schema::XMPMediaManagementUri, "xmpMM");
    create( Schema::MakerNoteSchemaUri, "mkn");
    create( Schema::IPTCSchemaUri, "Iptc4xmpCore");
    create( Schema::PhotoshopSchemaUri, "photoshop");
}


const Schema* SchemaRegistry::schemaFromUri(const QString & uri) const
{
    return d->uri2Schema[uri];
}

const Schema* SchemaRegistry::schemaFromPrefix(const QString & prefix) const
{
    return d->prefix2Schema[prefix];
}

const Schema* SchemaRegistry::create(const QString & uri, const QString & prefix)
{
    // First search for the schema
    const Schema* schema = schemaFromUri(uri);
    if(schema)
    {
        return schema;
    }
    // Second search for the prefix
    schema = schemaFromPrefix(prefix);
    if(schema)
    {
        return 0; // A schema with the same prefix already exist
    }
    // The schema doesn't exist yet, create it
    Schema* nschema = new Schema(uri, prefix);
    d->uri2Schema[uri] = nschema;
    d->prefix2Schema[prefix] = nschema;
    return nschema;
}

