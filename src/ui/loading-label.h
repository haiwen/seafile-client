#ifndef SEAFILE_CLIENT_UI_LOADING_LABEL_H
#define SEAFILE_CLIENT_UI_LOADING_LABEL_H

#include <QWidget>
#include <QLabel>

class QAction;

class LoadingLabel : public QLabel
{
    Q_OBJECT

public:
    LoadingLabel(QWidget *parent=0);

private:
    void enterEvent(QEvent *event);
    void leaveEvent(QEvent *event);
    void mousePressEvent(QMouseEvent *event);

signals:
    void refresh();
};

#endif // SEAFILE_CLIENT_UI_LOADING_LABEL_H
