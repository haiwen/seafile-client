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
