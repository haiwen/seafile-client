#ifndef SEAFILE_CLIENT_LOADING_VIEW_H_
#define SEAFILE_CLIENT_LOADING_VIEW_H_

#include <QWidget>
#include <QToolButton>

class QMovie;
class QShowEvent;
class QHideEvent;

class LoadingView : public QWidget {
    Q_OBJECT
public:
    LoadingView(QWidget *parent=0);
    void setQssStyleForTab();

protected:
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);

private:
    Q_DISABLE_COPY(LoadingView)

    QMovie *gif_;
};

class LoadMoreButton : public QToolButton {
    Q_OBJECT
public:
    explicit LoadMoreButton(QWidget *parent=0);
private:
    void paintEvent(QPaintEvent *event);
};

#endif // SEAFILE_CLIENT_LOADING_VIEW_H_
