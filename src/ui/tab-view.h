#ifndef SEAFILE_CLIENT_UI_TAB_VIEW_H
#define SEAFILE_CLIENT_UI_TAB_VIEW_H

#include <QWidget>

class QStackedWidget;
class QShowEvent;
class QHideEvent;

/**
 * Represents one tab of a QTabWidget
 */
class TabView : public QWidget {
    Q_OBJECT
public:
    TabView(QWidget *parent=0);
    virtual ~TabView() {};

public slots:
    virtual void refresh() = 0;

protected:
    virtual void showEvent(QShowEvent *event);
    virtual void hideEvent(QHideEvent *event);

    virtual void startRefresh() = 0;
    virtual void stopRefresh() = 0;

    QStackedWidget *mStack;
};

#endif // SEAFILE_CLIENT_UI_TAB_VIEW_H
