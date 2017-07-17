#include <QToolButton>

#include "search-bar.h"

namespace {

const int kMarginRightSearchBar = 16;
const int kHMargin = 5;

} // namespace

SearchBar::SearchBar(QWidget *parent)
    : QLineEdit(parent)
{
    setObjectName("repoNameFilter");

    // This property was introduced in Qt 5.2.
#if (QT_VERSION >= QT_VERSION_CHECK(5, 2, 0))
    setClearButtonEnabled(false);
#endif
#ifdef Q_OS_MAC
    setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif

    clear_button_ = new QToolButton(this);
    QPixmap pixmap(":/images/cancel-alpha.png");
    clear_button_size_ = pixmap.width();
    clear_button_->setIcon(QIcon(pixmap));
    clear_button_->setCursor(Qt::ArrowCursor);
    clear_button_->setStyleSheet("QToolButton { border: 0px; }");
    clear_button_->hide();
    connect(this, SIGNAL(textChanged(const QString&)),
            this, SLOT(updateClearButton(const QString&)));
    connect(clear_button_, SIGNAL(clicked()),
            this, SLOT(clear()));

    setStyleSheet(QString("QLineEdit { padding-right: %1px; padding-left: %2px}")
                          .arg(clear_button_size_ + kHMargin)
                          .arg(kHMargin));
}

void SearchBar::paintEvent(QPaintEvent* event)
{
    QLineEdit::paintEvent(event);
}

void SearchBar::resizeEvent(QResizeEvent* event)
{
    clear_button_->move(rect().right() - kMarginRightSearchBar
                        - kHMargin - clear_button_size_ - 2,
                        rect().bottom() - 30);
}

void SearchBar::updateClearButton(const QString& text)
{
    clear_button_->setVisible(!text.isEmpty());
}
