#ifndef SEAFILE_CLIENT_PAINT_UTILS_H_
#define SEAFILE_CLIENT_PAINT_UTILS_H_

#include <QFont>

#if defined(Q_OS_MAC)
#include "paint-cocoa.h"
#endif

QString fitTextToWidth(const QString& text, const QFont& font, int width);

QFont zoomFont(const QFont& font_in, double ratio);

QFont changeFontSize(const QFont& font_in, int size);

int textWidthInFont(const QString text, const QFont& font);

inline bool isHighDPI()
{
    if (getenv("SEAFILE_HDPI_DEBUG")) {
        return true;
    }
#if defined(Q_OS_MAC)
    return _isHiDPI();
#else
    return false;
#endif
}

inline float getScaleFactor()
{
#if defined(Q_OS_MAC)
    return _getScaleFactor();
#else
    return 1.0;
#endif
}


QString getIconPathByDPI(const QString& name);

QIcon getIconByDPI(const QString& name);

int getDPIScaledSize(int size);

#endif // SEAFILE_CLIENT_PAINT_UTILS_H_
