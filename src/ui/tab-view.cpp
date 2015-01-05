#include <QStackedWidget>
#include <QVBoxLayout>

#include "tab-view.h"

TabView::TabView(QWidget *parent)
    : QWidget (parent)
{
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);

    mStack = new QStackedWidget;
    layout->addWidget(mStack);
}

void TabView::showEvent(QShowEvent *event)
{
    startRefresh();
    QWidget::showEvent(event);
}

/**
 * Pause its freshing when this tab is not shown in front.
 */
void TabView::hideEvent(QHideEvent *event)
{
    stopRefresh();
    QWidget::hideEvent(event);
}
