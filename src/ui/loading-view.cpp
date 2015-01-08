#include <QtGui>

#include "loading-view.h"

LoadingView::LoadingView(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("LoadingView");

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    // QLabel::setMovie won't take the ownship of
    // QMovie, you need to delete it youself
    gif_ = new QMovie(":/images/loading.gif");
    QLabel *label = new QLabel;
    label->setMovie(gif_);
    label->setAlignment(Qt::AlignCenter);

    layout->addWidget(label);
    layout->setContentsMargins(0, 0, 0, 0);
}

LoadingView::~LoadingView() {
    gif_->stop();
    delete gif_;
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

