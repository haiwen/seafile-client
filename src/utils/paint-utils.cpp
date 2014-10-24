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

QString getIconPathByDPI(const QString& path)
{
    if (!isHighDPI()) {
        return path;
    }
    QFileInfo finfo(path);
    QString base = finfo.baseName();
    QString ext = finfo.completeSuffix();

    QDir dir = finfo.dir();

    QFileInfo finfo_2x(dir.filePath(base + "@2x" + "." + ext));

    if (finfo_2x.exists()) {
#ifndef QT_NO_DEBUG
        printf ("found @2x icon %s for %s\n",
                finfo_2x.absoluteFilePath().toUtf8().data(),
                path.toUtf8().data());
#endif
        return finfo_2x.absoluteFilePath();
    } else {
        return path;
    }
}

QIcon getIconByDPI(const QString& name)
{
    return QIcon(getIconPathByDPI(name));
}

int getDPIScaledSize(int size)
{
    const double factor = getScaleFactor();
    int ret = isHighDPI() ? (factor * size) : size;
    return ret;
}

QString get2xIconPath(const QString& path)
{
    QFileInfo finfo(path);
    QString base = finfo.baseName();
    QString ext = finfo.completeSuffix();

    QDir dir = finfo.dir();

    return dir.filePath(base + "@2x" + "." + ext);
}

QIcon getIconSet(const QString& path, int base_width, int base_height)
{
    if (base_height <= 0) {
        base_height = base_width;
    }

    QIcon icon;

    icon.addFile(path, QSize(base_width, base_height));
    icon.addFile(get2xIconPath(path), QSize(2 * base_width, 2 * base_height));

    return icon;
}

QIcon getIconSet(const QString& path, int size)
{
    return getIconSet(path, size, size);
}

QIcon getMenuIconSet(const QString& path)
{
    return getIconSet(path, 16);
}

QIcon getToolbarIconSet(const QString& path)
{
    return getIconSet(path, 24);
}
