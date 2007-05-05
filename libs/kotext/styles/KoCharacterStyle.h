/* This file is part of the KDE project
 * Copyright (C) 2006-2007 Thomas Zander <zander@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; version 2.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
#ifndef KOCHARACTERSTYLE_H
#define KOCHARACTERSTYLE_H

#include <QObject>
#include <QVector>
#include <QVariant>
#include <QString>
#include <QTextCharFormat>
#include <kotext_export.h>

class StylePrivate;
class QTextBlock;
class KoStyleStack;

/**
 * A container for all properties for a character style.
 * A character style represents all character properties for a set of characters.
 * Each character in the document will have a character style, most of the time
 * shared with all the characters next to it that have the same style (see
 * QTextFragment).
 * In a document the instances of QTextCharFormat which are based on a
 * KoCharacterStyle have a property StyleId with an integer as value which
 * equals styleId() of that style.
 * @see KoStyleManager
 */
class KOTEXT_EXPORT KoCharacterStyle : public QObject {
    Q_OBJECT
public:
    /// list of character style properties we can store in a QCharFormat
    enum Property {
        StyleId = QTextFormat::UserProperty+1, ///< The id stored in the charFormat to link the text to this style.
        HasHyphenation
    };

    /**
     * Constructor. Initializes with standard size/font properties.
     * @param parent the parent object for memory management purposes.
     */
    explicit KoCharacterStyle(QObject *parent = 0);
    /// clone constructor
    KoCharacterStyle(const KoCharacterStyle &other);
    ~KoCharacterStyle();
/*
    void setFont (const QFont &font) { setProperty(QTextFormat::FOO, font); }
    QFont font () const {
        return static_cast<QFont> (propertyObject(QTextFormat::FOO));
    }
*/
    /// See similar named method on QTextCharFormat
    void setFontFamily (const QString &family) { setProperty(QTextFormat::FontFamily, family); }
    /// See similar named method on QTextCharFormat
    QString fontFamily () const { return propertyString(QTextFormat::FontFamily); }
    /// See similar named method on QTextCharFormat
    void setFontPointSize (qreal size) { setProperty(QTextFormat::FontPointSize, size); }
    /// See similar named method on QTextCharFormat
    double fontPointSize () const { return propertyDouble(QTextFormat::FontPointSize); }
    /// See similar named method on QTextCharFormat
    void setFontWeight (int weight) { setProperty(QTextFormat::FontWeight, weight); }
    /// See similar named method on QTextCharFormat
    int fontWeight () const { return propertyInt(QTextFormat::FontWeight); }
    /// See similar named method on QTextCharFormat
    void setFontItalic (bool italic) { setProperty(QTextFormat::FontItalic, italic); }
    /// See similar named method on QTextCharFormat
    bool fontItalic () const { return propertyBoolean(QTextFormat::FontItalic); }
    /// See similar named method on QTextCharFormat
    void setFontOverline (bool overline) { setProperty(QTextFormat::FontOverline, overline); }
    /// See similar named method on QTextCharFormat
    bool fontOverline () const { return propertyBoolean(QTextFormat::FontOverline); }
    /// See similar named method on QTextCharFormat
    void setFontStrikeOut (bool strikeOut) { setProperty(QTextFormat::FontStrikeOut, strikeOut); }
    /// See similar named method on QTextCharFormat
    bool fontStrikeOut () const { return propertyBoolean(QTextFormat::FontStrikeOut); }
    /// See similar named method on QTextCharFormat
    void setUnderlineColor (const QColor &color) { setProperty(QTextFormat::TextUnderlineColor, color); }
    /// See similar named method on QTextCharFormat
    QColor underlineColor () const;
    /// See similar named method on QTextCharFormat
    void setFontFixedPitch (bool fixedPitch) { setProperty(QTextFormat::FontFixedPitch, fixedPitch); }
    /// See similar named method on QTextCharFormat
    bool fontFixedPitch () const { return propertyBoolean(QTextFormat::FontFixedPitch); }
    /// See similar named method on QTextCharFormat
    void setUnderlineStyle (QTextCharFormat::UnderlineStyle style) {
        setProperty(QTextFormat::TextUnderlineStyle, style);
    }
    /// See similar named method on QTextCharFormat
    QTextCharFormat::UnderlineStyle underlineStyle () const {
        return static_cast<QTextCharFormat::UnderlineStyle> (propertyInt(QTextFormat::TextUnderlineStyle));
    }
    /// See similar named method on QTextCharFormat
    void setVerticalAlignment (QTextCharFormat::VerticalAlignment alignment) {
        setProperty(QTextFormat::TextVerticalAlignment, alignment);
    }
    /// See similar named method on QTextCharFormat
    QTextCharFormat::VerticalAlignment verticalAlignment () const {
        return static_cast<QTextCharFormat::VerticalAlignment> (propertyInt(QTextFormat::TextVerticalAlignment));
    }
    /// See similar named method on QTextCharFormat
    void setTextOutline (const QPen &pen) { setProperty(QTextFormat::TextOutline, pen); }
    /// See similar named method on QTextCharFormat
    QPen textOutline () const;

    /// See similar named method on QTextCharFormat
    void setBackground (const QBrush &brush) { setProperty(QTextFormat::BackgroundBrush, brush); }
    /// See similar named method on QTextCharFormat
    QBrush background () const;
    /// See similar named method on QTextCharFormat
    void clearBackground ();

    /// See similar named method on QTextCharFormat
    void setForeground (const QBrush &brush) { setProperty(QTextFormat::ForegroundBrush, brush); }
    /// See similar named method on QTextCharFormat
    QBrush foreground () const;
    /// See similar named method on QTextCharFormat
    void clearForeground ();


    /// return the name of the style.
    const QString& name() const { return m_name; }

    /// set a user-visible name on the style.
    void setName(const QString &name) { m_name = name; }

    /// each style has a unique ID (non persistent) given out by the styleManager
    int styleId() const { return propertyInt(StyleId); }

    /// each style has a unique ID (non persistent) given out by the styleManager
    void setStyleId(int id) { setProperty(StyleId, id); }

    /**
     * Apply this style to a blockFormat by copying all properties from this
     * style to the target char format.
     */
    void applyStyle(QTextCharFormat &format) const;
    /**
     * Apply this style to the textBlock by copying all properties from this
     * style to the target block formats.
     */
    void applyStyle(QTextBlock &block) const;
    /**
     * Reset any styles and apply this style on the whole selection.
     */
    void applyStyle(QTextCursor *selection) const;

    /**
     * Load the style from the \a KoStyleStack style stack using the
     * OpenDocument format.
     */
    void loadOasis(KoStyleStack& styleStack);

private:
    void setProperty(int key, const QVariant &value);
    double propertyDouble(int key) const;
    int propertyInt(int key) const;
    QString propertyString(int key) const;
    bool propertyBoolean(int key) const;

private:
    QString m_name;
    StylePrivate *m_stylesPrivate;
};

#endif
