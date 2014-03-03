#include <QStyleOptionTab>

#include "proxy-style.h"

void SeafileProxyStyle::drawControl(ControlElement element,
                                    const QStyleOption * option,
                                    QPainter * painter,
                                    const QWidget * widget) const
{

    if (element == CE_TabBarTabLabel) {
        if (const QStyleOptionTab *tb = qstyleoption_cast<const QStyleOptionTab *>(option)) {
            if (tb->state & State_HasFocus) {
                QStyleOptionTab t(*tb);
                t.state = t.state ^ State_HasFocus;
                QProxyStyle::drawControl(element, &t, painter, widget);
                return;
            }
        }
    }

    QProxyStyle::drawControl(element, option, painter, widget);
}
