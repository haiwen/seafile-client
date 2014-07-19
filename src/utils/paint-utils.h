#ifndef SEAFILE_CLIENT_PAINT_UTILS_H_
#define SEAFILE_CLIENT_PAINT_UTILS_H_

#include <QFont>


QString fitTextToWidth(const QString& text, const QFont& font, int width);

QFont zoomFont(const QFont& font_in, double ratio);

QFont changeFontSize(const QFont& font_in, int size);

int textWidthInFont(const QString text, const QFont& font);

bool isHighDPI();

QString getIconPathByDPI(const QString& name);

QIcon getIconByDPI(const QString& name);

#endif // SEAFILE_CLIENT_PAINT_UTILS_H_
