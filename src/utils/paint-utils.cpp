#include <QtGlobal>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtWidgets>
#else
#include <QtGui>
#endif

#include "paint-utils.h"

QString fitTextToWidth(const QString& text, const QFont& font, int width)
{
    static QString ELLIPSISES = "...";

	QFontMetrics qfm(font);
	QSize size = qfm.size(0, text);
	if (size.width() <= width)
		return text;				// it fits, so just display it

	// doesn't fit, so we need to truncate and add ellipses
	QSize sizeElippses = qfm.size(0, ELLIPSISES); // we need to cut short enough to add these
	QString s = text;
	while (s.length() > 0)     // never cut shorter than this...
	{
		int len = s.length();
		s = text.left(len-1);
		size = qfm.size(0, s);
		if (size.width() <= (width - sizeElippses.width()))
			break;              // we are finally short enough
	}

	return (s + ELLIPSISES);
}

QFont zoomFont(const QFont& font_in, double ratio)
{
    QFont font(font_in);

    if (font.pointSize() > 0) {
        font.setPointSize((int)(font.pointSize() * ratio));
    } else {
        font.setPixelSize((int)(font.pixelSize() * ratio));
    }

    return font;
}

QFont changeFontSize(const QFont& font_in, int size)
{
    QFont font(font_in);

    font.setPixelSize(size);

    // if (font.pointSize() > 0) {
    //     font.setPointSize(size);
    // } else {
    //     font.setPixelSize(size);
    // }

    return font;
}

int textWidthInFont(const QString text, const QFont& font)
{
	QFontMetrics qfm(font);
	QSize size = qfm.size(0, text);

    return size.width();
}

// it might change when screen moves
// QIcon::addFile will add a "@2x" file if it exists.
double getScaleFactor()
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    return static_cast<QGuiApplication *>(QCoreApplication::instance())
                      ->devicePixelRatio();
#else
    return 1.0;
#endif
}

bool isHighDPI()
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    return getScaleFactor() > 1.0;
#else
    return false;
#endif
}

