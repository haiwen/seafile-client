#ifndef SEAFILE_CLIENT_UI_TAB_VIEW_H
#define SEAFILE_CLIENT_UI_TAB_VIEW_H

#include <QWidget>

class QStackedWidget;

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
    QStackedWidget *mStack;
};

#endif // SEAFILE_CLIENT_UI_TAB_VIEW_H
