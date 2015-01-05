#ifndef SEAFILE_CLIENT_UI_EVENT_DETAILS_TREE_H
#define SEAFILE_CLIENT_UI_EVENT_DETAILS_TREE_H

#include <QTreeView>
#include <QStandardItemModel>
#include <QStandardItem>

#include "api/event.h"
#include "api/commit-details.h"

class QModelIndex;

enum {
    EVENT_DETAILS_FILE_ITEM_TYPE = QStandardItem::UserType,
    EVENT_DETAILS_CATEGORY_TYPE
};

class EventDetailsCategoryItem : public QStandardItem {
public:
    EventDetailsCategoryItem(const QString& text);
    virtual int type() const { return EVENT_DETAILS_CATEGORY_TYPE; }
};

class EventDetailsFileItem : public QStandardItem {
public:
    enum EType {
        FILE_ADDED = 0,
        FILE_DELETED,
        FILE_MODIFIED,
        FILE_RENAMED,
        DIR_ADDED,
        DIR_DELETED
    };

    EventDetailsFileItem(const QString& path, EType etype);

    virtual QVariant data(int role) const;

    virtual int type() const { return EVENT_DETAILS_FILE_ITEM_TYPE; }

    bool isFileOpenable() const;

    // accessors
    QString name() const;
    const QString& path() const { return path_; }
    EType etype() const { return etype_; }

private:

    QString path_;
    EType etype_;
};

class EventDetailsTreeView : public QTreeView {
    Q_OBJECT
public:
    EventDetailsTreeView(const SeafEvent& event, QWidget *parent=0);

private slots:
    void onItemDoubleClicked(const QModelIndex& index);

private:
    QStandardItem* getFileItem(const QModelIndex& index);

    SeafEvent event_;
};

class EventDetailsTreeModel : public QStandardItemModel {
    Q_OBJECT
public:
    EventDetailsTreeModel(const SeafEvent& event, QObject *parent=0);

    void setCommitDetails(const CommitDetails& details);

private:
    void processEventCategory(const std::vector<QString>& files,
                              const QString& desc,
                              EventDetailsFileItem::EType etype);
    
    SeafEvent event_;
    CommitDetails details_;
};

#endif // SEAFILE_CLIENT_UI_EVENT_DETAILS_TREE_H
