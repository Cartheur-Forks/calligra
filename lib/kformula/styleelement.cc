/* This file is part of the KDE project
   Copyright (C) 2006 Alfredo Beaumont Sainz <alfredo.beaumont@gmail.com>

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

#include "basicelement.h"
#include "styleelement.h"

KFORMULA_NAMESPACE_BEGIN

StyleElement::StyleElement( BasicElement* parent ) : SequenceElement( parent ),
                                                     size_type ( NoSize ),
                                                     style( anyChar ),
                                                     custom_style( false ),
                                                     family( anyFamily ),
                                                     custom_family( false ),
                                                     color( "#000000" ),
                                                     custom_color( false ),
                                                     background( "#FFFFFF" ),
                                                     custom_background( false )
{
}

void StyleElement::calcSizes( const ContextStyle& context,
                              ContextStyle::TextStyle tstyle,
                              ContextStyle::IndexStyle istyle,
                              StyleAttributes& style )
{
    style.setSizeFactor( getSizeFactor( context, style.getSizeFactor() ) );
    inherited::calcSizes( context, tstyle, istyle, style );
    style.resetSizeFactor();
}

void StyleElement::draw( QPainter& painter, const LuPixelRect& r,
                         const ContextStyle& context,
                         ContextStyle::TextStyle tstyle,
                         ContextStyle::IndexStyle istyle,
                         StyleAttributes& style,
                         const LuPixelPoint& parentOrigin )
{
    style.setSizeFactor( getSizeFactor( context, style.getSizeFactor() ) );

    if ( customCharFamily() ) {
        style.setCharFamily( getCharFamily() );
    }
    else {
        style.setCharFamily( normalFamily );
    }

    if ( customColor() ) {
        style.setColor( getColor() );
    }
    else {
        style.setColor( style.getColor() );
    }

    if ( customBackground() ) {
        style.setBackground( getBackground() );
    }
    else {
        style.setBackground( style.getBackground() );
    }
    inherited::draw( painter, r, context, tstyle, istyle, style, parentOrigin );
}

// TODO: Support for deprecated attributes
bool StyleElement::readAttributesFromMathMLDom( const QDomElement& element )
{
    kdDebug( DEBUGID ) << "StyleElement::readAttributesFromMathMLDom\n";
    if ( !BasicElement::readAttributesFromDom( element ) ) {
        return false;
    }

    // MathML Section 3.2.2
    QString variantStr = element.attribute( "mathvariant" );
    if ( !variantStr.isNull() ) {
        if ( variantStr == "normal" ) {
            setCharStyle( normalChar );
            setCharFamily( normalFamily );
        }
        else if ( variantStr == "bold" ) {
            setCharStyle( boldChar );
            setCharFamily( normalFamily );
        }
        else if ( variantStr == "italic" ) {
            setCharStyle( italicChar );
            setCharFamily( normalFamily );
        }
        else if ( variantStr == "bold-italic" ) {
            setCharStyle( boldItalicChar );
            setCharFamily( normalFamily );
        }
        else if ( variantStr == "double-struck" ) {
            setCharStyle( normalChar );
            setCharFamily( doubleStruckFamily );
        }
        else if ( variantStr == "bold-fraktur" ) {
            setCharStyle( boldChar );
            setCharFamily( frakturFamily );
        }
        else if ( variantStr == "script" ) {
            setCharStyle( normalChar );
            setCharFamily( scriptFamily );
        }
        else if ( variantStr == "bold-script" ) {
            setCharStyle( boldChar );
            setCharFamily( scriptFamily );
        }
        else if ( variantStr == "fraktur" ) {
            setCharStyle( boldChar );
            setCharFamily( frakturFamily );
        }
        else if ( variantStr == "sans-serif" ) {
            setCharStyle( normalChar );
            setCharFamily( sansSerifFamily );
        }
        else if ( variantStr == "bold-sans-serif" ) {
            setCharStyle( boldChar );
            setCharFamily( sansSerifFamily );
        }
        else if ( variantStr == "sans-serif-italic" ) {
            setCharStyle( italicChar );
            setCharFamily( sansSerifFamily );
        }
        else if ( variantStr == "sans-serif-bold-italic" ) {
            setCharStyle( boldItalicChar );
            setCharFamily( sansSerifFamily );
        }
        else if ( variantStr == "monospace" ) {
            setCharStyle( normalChar );
            setCharFamily( monospaceFamily );
        }
    }

    QString sizeStr = element.attribute( "mathsize" );
    if ( !sizeStr.isNull() ) {
        // FIXME: This is broken
        kdDebug( DEBUGID ) << "MathSize attribute " << sizeStr << endl;
        if ( sizeStr == "small" ) {
            setRelativeSize( 0.8 ); // ### Arbitrary size
        }
        else if ( sizeStr == "normal" ) {
            setRelativeSize( 1.0 );
        }
        else if ( sizeStr == "big" ) {
            setRelativeSize( 1.2 ); // ### Arbitrary size
        }
        else {
            double s = getSize( sizeStr, &size_type );
            switch ( size_type ) {
            case AbsoluteSize:
                setAbsoluteSize( s );
                break;
            case RelativeSize:
                setRelativeSize( s );
                break;
            case PixelSize:
                setPixelSize( s );
                break;
            }
        }
    }

    QString colorStr = element.attribute( "mathcolor" );
    if ( !colorStr.isNull() ) {
        // TODO: Named colors differ from those Qt supports. See section 3.2.2.2
        kdWarning() << "Setting color: " << colorStr << endl;
        color.setNamedColor( colorStr );
        custom_color = true;
	}
    QString backgroundStr = element.attribute( "mathbackground" );
    if ( !backgroundStr.isNull() ) {
        // TODO: Named colors differ from those Qt supports. See section 3.2.2.2
        background.setNamedColor( backgroundStr );
        custom_background = true;
	}
    return true;
}

void StyleElement::setSize( double s )
{ 
        kdDebug( DEBUGID) << "Setting size: " << s << endl;
        size = s;
}

void StyleElement::setAbsoluteSize( double s )
{ 
        kdDebug( DEBUGID) << "Setting absolute size: " << s << endl;
        size_type = AbsoluteSize;
        size = s;
}

void StyleElement::setRelativeSize( double f )
{ 
        kdDebug( DEBUGID) << "Setting relative size: " << f << endl;
        size_type = RelativeSize;
        size = f;
}

void StyleElement::setPixelSize( double px )
{ 
        kdDebug( DEBUGID) << "Setting pixel size: " << px << endl;
        size_type = PixelSize;
        size = px;
}

double StyleElement::getSizeFactor( const ContextStyle& context, double factor )
{
    switch ( size_type ) {
    case NoSize:
        return factor;
    case AbsoluteSize:
        return factor * size / context.baseSize();
    case RelativeSize:
        return factor * size;
    case PixelSize:
        // 3.2.2 says v-unit insteado of h-unit, that's why we use Y and not X
//        kdDebug( DEBUGID ) 
        return factor * context.pixelYToPt( size ) / context.baseSize(); 
    }
    kdWarning( DEBUGID ) << "getSizeFactor: Unknown SizeType\n";
    return factor;
}

void StyleElement::setCharStyle( CharStyle cs )
{
    style = cs;
    custom_style = true;
}

void StyleElement::setCharFamily( CharFamily cf )
{
    family = cf;
    custom_family = true;
}

double StyleElement::str2size( const QString& str, SizeType *st, uint index, SizeType type )
{
    QString num = str.left( index );
    bool ok;
    double size = num.toDouble( &ok );
    if ( ok ) {
        if ( st ) {
            *st = type;
        }
        return size;
    }
    if ( st ) {
        *st = NoSize;
    }
    return -1;
}

double StyleElement::getSize( const QString& str, SizeType* st )
{
    int index = str.find( "%" );
    if ( index != -1 ) {
        return str2size( str, st, index, RelativeSize ) / 100.0;
    }
    index = str.find( "pt", 0, false );
    if ( index != -1 ) {
        return str2size( str, st, index, AbsoluteSize );
    }
    index = str.find( "mm", 0, false );
    if ( index != -1 ) {
        return str2size( str, st, index, AbsoluteSize ) * 72.0 / 20.54;
    }
    index = str.find( "cm", 0, false );
    if ( index != -1 ) {
        return str2size( str, st, index, AbsoluteSize ) * 72.0 / 2.54;
    }
    index = str.find( "in", 0, false );
    if ( index != -1 ) {
        return str2size( str, st, index, AbsoluteSize ) * 72.0;
    }
    index = str.find( "em", 0, false );
    if ( index != -1 ) {
        return str2size( str, st, index, AbsoluteSize );
    }
    index = str.find( "ex", 0, false );
    if ( index != -1 ) {
        return str2size( str, st, index, AbsoluteSize );
    }
    index = str.find( "pc", 0, false );
    if ( index != -1 ) {
        return str2size( str, st, index, AbsoluteSize ) * 12.0;
    }
    index = str.find( "px", 0, false );
    if ( index != -1 ) {
        return str2size( str, st, index, PixelSize );
    }
    kdWarning( DEBUGID ) << "Unknown mathsize unit type\n";
    return -1;
}

KFORMULA_NAMESPACE_END
