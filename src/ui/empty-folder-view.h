#ifndef SEAFILE_CLIENT_EMPTY_FOLDER_VIEW_H_
#define SEAFILE_CLIENT_EMPTY_FOLDER_VIEW_H_

#include <QWidget>
#include <QLabel>

class EmptyFolderView : public QLabel {
    Q_OBJECT
public:
    EmptyFolderView(QWidget *parent = 0, bool readonly = false);

signals:
    void dropFile(const QStringList& paths);

private:
    Q_DISABLE_COPY(EmptyFolderView)

    bool eventFilter(QObject *obj, QEvent *ev);

    bool readonly_;
};

#endif // SEAFILE_CLIENT_EMPTY_FOLDER_VIEW_H_
