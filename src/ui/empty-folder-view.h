#ifndef SEAFILE_CLIENT_EMPTY_FOLDER_VIEW_H_
#define SEAFILE_CLIENT_EMPTY_FOLDER_VIEW_H_

#include <QWidget>
#include <QLabel>

class EmptyFolderView : public QLabel {
    Q_OBJECT
public:
    EmptyFolderView(QWidget *parent = 0);


private:
    Q_DISABLE_COPY(EmptyFolderView)

};

#endif // SEAFILE_CLIENT_EMPTY_FOLDER_VIEW_H_
