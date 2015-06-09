#ifndef SEAFILE_CLIENT_UI_EVENT_DETAILS_TREE_H
#define SEAFILE_CLIENT_UI_EVENT_DETAILS_TREE_H

#include <QListView>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QStyledItemDelegate>
#include <QIcon>

#include "api/event.h"
#include "api/commit-details.h"

class QModelIndex;
class QMenu;

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

    EventDetailsFileItem(const QString& path, EType etype, const QString& original_path = "");

    virtual QVariant data(int role) const;

    bool isFileOpenable() const;

    // accessors
    QString name() const;
    const QString& path() const { return path_; }
    const QString& original_path() const { return original_path_; }
    EType etype() const { return etype_; }
    QString etype_desc() const;
    QIcon etype_icon() const;
    const char* etype_color() const;

private:

    QString path_;
    QString original_path_;
    EType etype_;
};

class EventDetailsListView : public QListView {
    Q_OBJECT
public:
    EventDetailsListView(const SeafEvent& event, QWidget *parent=0);

private slots:
    void onItemDoubleClicked(const QModelIndex& index);
    void onOpen();
    void onOpenInFileBrowser();

private:
    QStandardItem* getFileItem(const QModelIndex& index);

    void contextMenuEvent(QContextMenuEvent * event);

    SeafEvent event_;
    QMenu* context_menu_;
    QAction* open_action_;
    QAction* open_in_file_browser_action_;
    EventDetailsFileItem* item_in_action_;
};

class EventDetailsListModel : public QStandardItemModel {
    Q_OBJECT
public:
    EventDetailsListModel(const SeafEvent& event, QObject *parent=0);

    void setCommitDetails(const CommitDetails& details);

private:
    void processEventCategory(const std::vector<QString>& files,
                              const QString& desc,
                              EventDetailsFileItem::EType etype);

    SeafEvent event_;
    CommitDetails details_;
};

class EventDetailsFileItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit EventDetailsFileItemDelegate(QWidget *parent)
        : QStyledItemDelegate(parent) {}
    void paint(QPainter *painter,
               const QStyleOptionViewItem& option,
               const QModelIndex& index) const;

    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const;
};

#endif // SEAFILE_CLIENT_UI_EVENT_DETAILS_TREE_H
