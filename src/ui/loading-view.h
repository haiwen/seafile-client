#ifndef SEAFILE_CLIENT_LOADING_VIEW_H_
#define SEAFILE_CLIENT_LOADING_VIEW_H_

#include <QWidget>

class QMovie;
class QShowEvent;
class QHideEvent;

class LoadingView : public QWidget {
    Q_OBJECT
public:
    LoadingView(QWidget *parent=0);
    ~LoadingView();
    void setQssStyleForTab();

protected:
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);

private:
    Q_DISABLE_COPY(LoadingView)

    QMovie *gif_;
};

#endif // SEAFILE_CLIENT_LOADING_VIEW_H_
