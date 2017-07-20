#include <QToolButton>
#include <QLabel>

#include "search-bar.h"

namespace {

const int kMarginRightSearchBar = 16;
const int kMarginBottom = 5;
const int kHMargin = 5;

} // namespace

SearchBar::SearchBar(QWidget *parent)
    : QLineEdit(parent)
{
    setObjectName("mSearchBar");

    // This property was introduced in Qt 5.2.
#if (QT_VERSION >= QT_VERSION_CHECK(5, 2, 0))
    setClearButtonEnabled(false);
#endif
#ifdef Q_OS_MAC
    setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif

    placeholder_label_ = new QLabel(this);

    clear_button_ = new QToolButton(this);
    QPixmap pixmap(":/images/cancel.png");
    clear_button_size_ = pixmap.width();
    clear_button_->setIcon(QIcon(pixmap));
    clear_button_->setCursor(Qt::ArrowCursor);
    clear_button_->setAutoRaise(false);
    clear_button_->hide();
    connect(clear_button_, SIGNAL(clicked()),
            this, SLOT(clear()));

    connect(this, SIGNAL(textChanged(const QString&)),
            this, SLOT(onTextChanged(const QString&)));

    const QString style = QString(" QLineEdit#mSearchBar { "
                                      " padding-right: %1px; "
                                      " padding-left: %2px; } " )
                                  .arg(clear_button_size_ + kHMargin)
                                  .arg(kHMargin);
    setStyleSheet(style);
}

void SearchBar::paintEvent(QPaintEvent* event)
{
    QLineEdit::paintEvent(event);
}

void SearchBar::resizeEvent(QResizeEvent* event)
{
    clear_button_->move(rect().right() - kMarginRightSearchBar
                        - kHMargin - clear_button_size_ - 2,
                        (rect().bottom() - kMarginBottom - clear_button_size_) / 2 - 2);
    int label_height = placeholder_label_->height();
    int label_width = placeholder_label_->width();
    placeholder_label_->move((rect().right() - label_width) / 2,
                             (rect().bottom() - kMarginBottom - label_height) / 2);
}

void SearchBar::onTextChanged(const QString& text)
{
    clear_button_->setVisible(!text.isEmpty());
    placeholder_label_->setVisible(text.isEmpty());
}

void SearchBar::setPlaceholderText(const QString& text)
{
    placeholder_label_->setText(text);
    placeholder_label_->setStyleSheet("QLabel { font-size: 13px;"
                                              " color: #AAAAAA; }");
    placeholder_label_->show();
}
