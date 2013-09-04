#include <QtGui>

#include "my-tabwidget.h"

namespace {

class CustomTabStyle : public QProxyStyle
{
    
public:
    QSize sizeFromContents(ContentsType type,
                           const QStyleOption *option,
                           const QSize &size,
                           const QWidget *widget) const;

    void drawControl(ControlElement element,
                     const QStyleOption *option,
                     QPainter *painter,
                     const QWidget *widget) const;
    
};

QSize CustomTabStyle::sizeFromContents(ContentsType type,
                                       const QStyleOption *option,
                                       const QSize &size,
                                       const QWidget *widget) const
{
    QSize s = QProxyStyle::sizeFromContents(type, option, size, widget);
    if (type == QStyle::CT_TabBarTab)
        s.transpose();
    return s;
}

void CustomTabStyle::drawControl(ControlElement element,
                                 const QStyleOption *option,
                                 QPainter *painter,
                                 const QWidget *widget) const
{
    if (element == CE_TabBarTab)
    {
        if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(option))
        {
            QStyleOptionTab opt(*tab);
            opt.shape = QTabBar::RoundedNorth;
            QProxyStyle::drawControl(element, &opt, painter, widget);
            return;
        }
    }
    QProxyStyle::drawControl(element, option, painter, widget);
}

} // namespace

MyTabWidget::MyTabWidget(QWidget *parent) : QTabWidget(parent)
{
    tabBar()->setStyle(new CustomTabStyle);
}
