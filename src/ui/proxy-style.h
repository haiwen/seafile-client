#ifndef SEAFILE_CLEINT_PROXY_STYLE
#define SEAFILE_CLEINT_PROXY_STYLE

#include <QProxyStyle>

class QStyleOption;
class QPainter;
class QWidget;

/**
 * Use a custom proxy style to remove the border of tabbar text when selected
 * See http://stackoverflow.com/questions/7492080/styling-the-text-outline-in-qts-tabs
 */
class SeafileProxyStyle: public QProxyStyle {
public:
    virtual void drawControl (ControlElement element, const QStyleOption * option,
                              QPainter * painter, const QWidget * widget = 0) const;
};

#endif // SEAFILE_CLEINT_PROXY_STYLE
