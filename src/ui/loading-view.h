#ifndef SEAFILE_CLIENT_LOADING_VIEW_H_
#define SEAFILE_CLIENT_LOADING_VIEW_H_

#include <QWidget>
#include <QToolButton>
#include <QLabel>

class QMovie;
class QShowEvent;
class QHideEvent;
class QHBoxLayout;

class LoadingView : public QLabel {
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

class LoadMoreButton : public QWidget {
    Q_OBJECT
public:
    explicit LoadMoreButton(QWidget *parent=0);

signals:
    void clicked();

private slots:
    void onBtnClicked();

private:
    QHBoxLayout *btn_layout_;
    QToolButton *load_more_btn_;
    LoadingView *loading_label_;
};

#endif // SEAFILE_CLIENT_LOADING_VIEW_H_
