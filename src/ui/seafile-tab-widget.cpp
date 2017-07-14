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

#include "utils/paint-utils.h"

#include "seafile-tab-widget.h"

namespace {

const int kTabIconSize = 24;

const char *kTabsBackgroundColor = "white";
const char *kTabsBackgroundColorAlpha = "#F9F9F9";
const char *kSelectedTabBorderBottomColor = "#D58747";
const char *kBorderColor = "#DCDCDE";
const int kSelectedTabBorderBottomWidth = 3;
const int kSelectedTabBorderBottomHeightAlpha = 2;
const int kSelectedTabBorderBottomWidthAlpha = 20;

} // namespace

SeafileTabBar::SeafileTabBar(QWidget *parent)
    : QTabBar(parent)
{
    setMinimumSize(0, 48);
}

void SeafileTabBar::addTab(const QString& text,
                           const QString& icon_path,
                           const QString& highlighted_icon)
{
    int index = QTabBar::addTab(text);
    setTabToolTip(index, text);
    icons_.push_back(icon_path);
    highlighted_icons_.push_back(highlighted_icon);
}

void SeafileTabBar::paintEvent(QPaintEvent *event)
{
    QStylePainter p(this);
    QPainter painter;
    painter.begin(this);

    for (int index = 0, total = count(); index < total; index++) {
        QRect rect = tabRect(index);
        rect.adjust(0, 0, 0, 12);
        qWarning("tabRect: height: %d", rect.height());

        // QStyleOptionTabV3 tab;
        // initStyleOption(&tab, index);

        // Draw the tab background
        painter.fillRect(rect, QColor(kTabsBackgroundColor));

        // Draw the tab icon in the center
        QPoint top_left;
        top_left.setX(rect.topLeft().x() + ((rect.width() - kTabIconSize) / 2));
        top_left.setY(rect.topLeft().y() + ((rect.height() - kTabIconSize) / 2) + 2);

        QIcon icon(currentIndex() == index ? highlighted_icons_[index]
                                           : icons_[index]);
        QRect icon_rect(top_left, QSize(kTabIconSize, kTabIconSize));
        // get the device pixel radio from current painter device
        int scale_factor = 1;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        scale_factor = globalDevicePixelRatio();
#endif // QT5
        QPixmap icon_pixmap(icon.pixmap(QSize(kTabIconSize, kTabIconSize) * scale_factor));
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        icon_pixmap.setDevicePixelRatio(scale_factor);
#endif // QT5
        painter.drawPixmap(icon_rect, icon_pixmap);

        // int indicator_width = count() * rect.width() / 8;

        // Draw the selected tab indicator
        if (currentIndex() == index) {
            // top_left.setX(rect.bottomLeft().x() + (rect.width() / 2) - (indicator_width / 2));
            top_left.setX(rect.bottomLeft().x() + (rect.width() / 2) -
                          (kSelectedTabBorderBottomWidthAlpha / 2));
            // top_left.setY(rect.bottomLeft().y() - kSelectedTabBorderBottomHeightAlpha + 1);
            top_left.setY(rect.topLeft().y() + ((rect.height() - kTabIconSize) / 2) +
                          + 2 + kTabIconSize + 4);
            QRect border_bottom_rect(top_left, QSize(kSelectedTabBorderBottomWidthAlpha,
                                                     kSelectedTabBorderBottomHeightAlpha));
            painter.fillRect(border_bottom_rect, QColor(kSelectedTabBorderBottomColor));
        }
    }

    // draw border
    QPen borderPen(QColor(kBorderColor), 1);
    painter.save();
    painter.setPen(borderPen);
    painter.drawLine(rect().topLeft(), rect().topRight());
    painter.restore();
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

    connect(tabbar_, SIGNAL(currentChanged(int)),
            this, SIGNAL(currentTabChanged(int)));
}

void SeafileTabWidget::addTab(QWidget* tab,
                              const QString& text,
                              const QString& icon_path,
                              const QString& highlighted_icon)
{
    tabbar_->addTab(text, icon_path, highlighted_icon);
    stack_->addWidget(tab);
}

int SeafileTabWidget::currentIndex() const
{
    return stack_->currentIndex();
}

void SeafileTabWidget::adjustTabsWidth(int full_width)
{
    int tab_width = (full_width / tabbar_->count()) - 1;
    QString style("QTabBar::tab { min-width: %1px; }");
    style = style.arg(tab_width);
    setStyleSheet(style);
}

void SeafileTabWidget::removeTab(int index, QWidget *widget)
{
    tabbar_->removeTab(index);
    stack_->removeWidget(widget);
}

int SeafileTabWidget::count() const
{
    return tabbar_->count();
}
