#include <cstdio>
#include <iostream>
#include <QLabel>
#include <QPaintEvent>
#include <QStylePainter>
#include <QStyleOptionTabV3>
#include <QPixmap>
#include <QIcon>
#include <QStackedLayout>
#include <QVBoxLayout>

#include "seafile-tab-widget.h"

namespace {

const int kTabIconSize = 32;

const char *kTabsBackgroundColor = "white";
const char *kSelectedTabBorderBottomColor = "#D58747";
const int kSelectedTabBorderBottomWidth = 5;

} // namespace

SeafileTabBar::SeafileTabBar(QWidget *parent)
    : QTabBar(parent)
{
}

void SeafileTabBar::addTab(const QString& text, const QString& icon_path)
{
    QTabBar::addTab(text);
    icons_.push_back(icon_path);
}

void SeafileTabBar::paintEvent(QPaintEvent *event)
{
    QStylePainter p(this);
    QPainter painter;
    painter.begin(this);

    for (int index = 0, total = count(); index < total; index++) {
        const QRect rect = tabRect(index);

        // QStyleOptionTabV3 tab;
        // initStyleOption(&tab, index);

        QPixmap icon(icons_[index]);

        // Draw the tab background
        painter.fillRect(rect, QColor(kTabsBackgroundColor));

        // Draw the tab icon in the center
        QPoint top_left;
        top_left.setX(rect.topLeft().x() + ((rect.width() - kTabIconSize) / 2));
        top_left.setY(rect.topLeft().y() + ((rect.height() - kTabIconSize) / 2));
        QRect icon_rect(top_left, QSize(kTabIconSize, kTabIconSize));
        painter.drawPixmap(icon_rect, icon);

        // Draw the selected tab indicator
        if (currentIndex() == index) {
            top_left.setX(rect.bottomLeft().x() + (rect.width() / 4));
            top_left.setY(rect.bottomLeft().y() - kSelectedTabBorderBottomWidth + 1);
            QRect border_bottom_rect(top_left, QSize(rect.width() / 2 , kSelectedTabBorderBottomWidth));
            painter.fillRect(border_bottom_rect, QColor(kSelectedTabBorderBottomColor));
        }
    }
}


SeafileTabWidget::SeafileTabWidget(QWidget *parent)
    : QWidget(parent)
{
    layout_ = new QVBoxLayout;
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(0);
    setLayout(layout_);

    tabbar_ = new SeafileTabBar;
    tabbar_->setExpanding(true);

    // Init content pane
    pane_ = new QWidget;
    // for qss style
    pane_->setObjectName("pane");
    stack_ = new QStackedLayout;
    stack_->setContentsMargins(0, 0, 0, 0);
    pane_->setLayout(stack_);

    layout_->addWidget(tabbar_);
    // layout_->addLayout(stack_);
    layout_->addWidget(pane_);

    connect(tabbar_, SIGNAL(currentChanged(int)),
            stack_, SLOT(setCurrentIndex(int)));
}

void SeafileTabWidget::addTab(QWidget *tab, const QString& text, const QString& icon_path)
{
    tabbar_->addTab(text, icon_path);
    stack_->addWidget(tab);
}

int SeafileTabWidget::currentIndex() const
{
    return stack_->currentIndex();
}

void SeafileTabWidget::adjustTabsWidth(int full_width)
{
    int tab_width = full_width / 3 - 1;
    QString style("QTabBar::tab { min-width: %1px; }");
    style = style.arg(tab_width);
    setStyleSheet(style);
}
