#ifndef SEAFILE_CLIENT_UI_LOADING_LABEL_H
#define SEAFILE_CLIENT_UI_LOADING_LABEL_H

#include <QWidget>
#include <QLabel>

#include "utils/singleton.h"

class QMovie;

class LoadingLabel : public QLabel
{
    SINGLETON_DEFINE(LoadingLabel)
    Q_OBJECT

public:
    void movieStart();
    void movieStop();
    void movieLock();
    void movieUnlock();

private:
    LoadingLabel(QWidget *parent=0);
    void mousePressEvent(QMouseEvent *event);

    QMovie *loading_movie_;
    bool is_locked_;

signals:
    void refresh();
};

#endif // SEAFILE_CLIENT_UI_LOADING_LABEL_H
