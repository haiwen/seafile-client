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
    gif_->setScaledSize(QSize(24, 24));
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
    : QWidget(parent)
{
    load_more_btn_ = new QToolButton;
    load_more_btn_->setObjectName("loadMoreBtn");
    load_more_btn_->setText(tr("load more"));
    btn_layout_ = new QHBoxLayout(this);
    btn_layout_->addWidget(load_more_btn_, Qt::AlignCenter);

    loading_label_ = new LoadingView;

    // Must set fill backgound because this button is used as an "index widget".
    // See the doc of QAbstractItemView::setIndexWidget for details.
    setAutoFillBackground(true);

    connect(load_more_btn_, SIGNAL(clicked()),
            this, SLOT(onBtnClicked()));
}

void LoadMoreButton::onBtnClicked()
{
    load_more_btn_->hide();

    btn_layout_->addWidget(loading_label_, Qt::AlignCenter);

    emit clicked();
}
