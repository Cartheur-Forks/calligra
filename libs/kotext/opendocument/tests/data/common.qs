var document = new QTextDocument;
var cursor = new QTextCursor(document);

// See KoCharacterStyle.cpp
var defaultFont = new QFont;
defaultFont.setPointSizeF(12.0);
defaultFont.setWeight(QFont.Normal);

var KoParagraphStyle = {};
KoParagraphStyle.StyleId = QTextFormat.UserProperty + 1;
KoParagraphStyle.PercentLineHeight = QTextFormat.UserProperty + 2;

var defaultBlockFormat = new QTextBlockFormat;
setFormatProperty(defaultBlockFormat, KoParagraphStyle.PercentLineHeight, 120);
defaultBlockFormat.clearBackground();
cursor.setBlockFormat(defaultBlockFormat);

QTextBlockFormat.clone = function(fmt) {
    var newFormat = new QTextBlockFormat;
    copyFormatProperties(newFormat, fmt);
    return newFormat;
}

var defaultTextFormat = new QTextCharFormat;
defaultTextFormat.setFont(defaultFont);
defaultTextFormat.setVerticalAlignment(QTextCharFormat.AlignNormal);

QTextCharFormat.clone = function(fmt) {
    var newFormat = new QTextCharFormat;
    copyFormatProperties(newFormat, fmt);

    // CHECKME: When new formats are created, the color is explicitly set to Black
    if (newFormat.foreground().style() == 0) {
        newFormat.setForeground(new QBrush(new QColor(0, 0, 0)));
    }
    return newFormat;
};

QTextListFormat.clone = function(fmt) {
    var newFormat = new QTextListFormat;
    copyFormatProperties(newFormat, fmt);
    return newFormat;
};

// KOffice specific
var KoListStyle = {};
KoListStyle.ListItemSuffix = 0x1003E9;
KoListStyle.StartValue = 0x1003EA;
KoListStyle.Level = 0x1003EB;

var defaultListItemFormat = new QTextCharFormat;
defaultListItemFormat.setFont(defaultFont);
defaultListItemFormat.setVerticalAlignment(QTextCharFormat.AlignNormal);
defaultListItemFormat.setForeground(new QBrush(new QColor(0, 0, 0)));

var defaultListFormat = new QTextListFormat;
setFormatProperty(defaultListFormat, KoListStyle.StartValue, 1);
setFormatProperty(defaultListFormat, KoListStyle.Level, 1);

var KoCharacterStyle = {};
KoCharacterStyle.StyleId = QTextFormat.UserProperty + 1;
KoCharacterStyle.HasHyphenataion = QTextFormat.UserProperty + 2;
KoCharacterStyle.FontId = QTextFormat.UserProperty + 3;
KoCharacterStyle.StrikeOutStyle = QTextFormat.UserProperty + 4;
KoCharacterStyle.StrikeOutType = QTextFormat.UserProperty + 5;
KoCharacterStyle.StrikeOutColor = QTextFormat.UserProperty + 6;
KoCharacterStyle.UnderlineStyle = QTextFormat.UserProperty + 7;
KoCharacterStyle.UnderlineType = QTextFormat.UserProperty + 8;

KoCharacterStyle.NoLineType = 0;
KoCharacterStyle.SingleLine = 1;
KoCharacterStyle.DoubleLine = 2;

KoCharacterStyle.NoLineStyle = 0; // Qt.NoPen;
KoCharacterStyle.SolidLine = 1; // Qt.SolidLine;
KoCharacterStyle.DottedLine = 3; // Qt.DotLine;
KoCharacterStyle.DashLine = 2; // Qt.DashLine;
KoCharacterStyle.DashDotLine = 4; // Qt.DashDotLine;
KoCharacterStyle.DashDotDotLine = 5; // Qt.DashDotDotLine;
KoCharacterStyle.LongDashLine = KoCharacterStyle.DashDotDotLine + 1;
KoCharacterStyle.WaveLine = KoCharacterStyle.DashDotDotLine + 2;

