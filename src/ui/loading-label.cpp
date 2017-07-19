#include <QMovie>

#include "loading-label.h"

namespace {

} // namespace

SINGLETON_IMPL(LoadingLabel);

LoadingLabel::LoadingLabel(QWidget *parent)
    : QLabel(),
      is_locked_(false)
{
    resize(32, 32);

    loading_movie_ = new QMovie(":/images/loadingspinner-2-alpha.gif");
    loading_movie_->setScaledSize(QSize(32, 32));
    loading_movie_->setParent(this);
    setMovie(loading_movie_);

    show();
    movieStart();
    movieStop();
}

void LoadingLabel::movieStart()
{
    if (is_locked_ == false) {
        loading_movie_->start();
    }
}

void LoadingLabel::movieStop()
{
    loading_movie_->stop();
}

void LoadingLabel::mousePressEvent(QMouseEvent *event)
{
    emit refresh();
    movieStart();
}

void LoadingLabel::movieLock()
{
    is_locked_ = true;
}

void LoadingLabel::movieUnlock()
{
    is_locked_ = false;
}
