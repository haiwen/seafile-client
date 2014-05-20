#include <cstdio>
#include <QStyleOptionTab>

#include "proxy-style.h"

void SeafileProxyStyle::drawControl(ControlElement element,
                                    const QStyleOption * option,
                                    QPainter * painter,
                                    const QWidget * widget) const
{

    if (element == CE_TabBarTabLabel) {
        printf ("[draw tab label] name is %s\n", widget->objectName().toUtf8().data());
        if (const QStyleOptionTab *tb = qstyleoption_cast<const QStyleOptionTab *>(option)) {
            if (tb->state & State_HasFocus) {
                QStyleOptionTab t(*tb);
                t.state = t.state ^ State_HasFocus;
                QProxyStyle::drawControl(element, &t, painter, widget);
                return;
            }
        }
    }

    if (element == CE_TabBarTab) {
        printf ("[draw tab] name is %s\n", widget->objectName().toUtf8().data());
    }

    QProxyStyle::drawControl(element, option, painter, widget);
}
