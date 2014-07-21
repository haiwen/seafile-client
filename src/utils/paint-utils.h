#ifndef SEAFILE_CLIENT_PAINT_UTILS_H_
#define SEAFILE_CLIENT_PAINT_UTILS_H_

#include <QFont>

#if defined(Q_OS_MAC)
#include "utils-mac.h"
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
    return __mac_isHiDPI();
#else
    return false;
#endif
}

inline double getScaleFactor()
{
#if defined(Q_OS_MAC)
    return __mac_getScaleFactor();
#else
    return 1.0;
#endif
}


QString getIconPathByDPI(const QString& name);

QIcon getIconByDPI(const QString& name);

int getDPIScaledSize(int size);

/*
 * Returns a QIcon that contains both 1x and 2x icon
 */
QIcon getIconSet(const QString& path, int base_width, int base_height);

/**
 * Shortcut for getIconSet(path, size, size)
 */
QIcon getIconSet(const QString& path, int size);

/**
 * Shortcut for getIconSet(path, 16)
 */
QIcon getMenuIconSet(const QString& path);

/**
 * Shortcut for getIconSet(path, 24)
 */
QIcon getToolbarIconSet(const QString& path);

#endif // SEAFILE_CLIENT_PAINT_UTILS_H_
