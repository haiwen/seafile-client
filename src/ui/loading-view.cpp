#include <QtGlobal>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtWidgets>
#else
#include <QtGui>
#endif

#include "loading-view.h"

LoadingView::LoadingView(QWidget *parent)
    : QLabel(parent)
{
    gif_ = new QMovie(":/images/loading-spinner.gif");
    gif_->setScaledSize(QSize(48, 48));
    gif_->setParent(this);
    setMovie(gif_);
    setAlignment(Qt::AlignCenter);
}

void LoadingView::showEvent(QShowEvent *event)
{
    gif_->start();
    QWidget::showEvent(event);
}

void LoadingView::hideEvent(QHideEvent *event)
{
    gif_->stop();
    QWidget::hideEvent(event);
}

void LoadingView::setQssStyleForTab()
{
    static const char *kLoadingViewQss = "border: 0; margin: 0;"
                                         "border-top: 1px solid #DCDCDE;"
                                         "background-color: #F5F5F7;";

    setStyleSheet(kLoadingViewQss);
}

LoadMoreButton::LoadMoreButton(QWidget *parent)
    : QToolButton(parent)
{
    setText(tr("load more"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // Must set fill backgound because this button is used as an "index widget".
    // See the doc of QAbstractItemView::setIndexWidget for details.
    setAutoFillBackground(true);
}
