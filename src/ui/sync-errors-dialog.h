#ifndef SEAFILE_CLIENT_SYNC_ERRORS_DIALOG_H
#define SEAFILE_CLIENT_SYNC_ERRORS_DIALOG_H

#include <vector>

#include <QTableView>
#include <QHeaderView>
#include <QAbstractTableModel>
#include <QDialog>

#include "rpc/sync-error.h"

class QTimer;
class QStackedWidget;
class QResizeEvent;
class QSizeGrip;
class QLabel;
class QEvent;

class SyncError;
class SyncErrorsTableView;
class SyncErrorsTableModel;

class SyncErrorsDialog : public QDialog
{
    Q_OBJECT

public:
    SyncErrorsDialog(QWidget *parent=0);
    void updateErrors();

    void resizeEvent(QResizeEvent *event);

private slots:
    void onModelReset();

private:
    void createTitleBar();
    void createEmptyView();

    bool eventFilter(QObject *obj, QEvent *event);

    // title bar (windows and osx only)
    QWidget *header_;
    QLabel *brand_label_;
    QPushButton *minimize_button_;
    QPushButton *close_button_;
    QPoint old_pos_;

    QSizeGrip *resizer_;

    QStackedWidget *stack_;
    SyncErrorsTableView *table_;
    SyncErrorsTableModel *model_;
    QWidget *empty_view_;
};

class SyncErrorsTableView : public QTableView
{
    Q_OBJECT

public:
    SyncErrorsTableView(QWidget *parent=0);

    void contextMenuEvent(QContextMenuEvent *event);
    void resizeEvent(QResizeEvent *event);

private slots:
    void onItemDoubleClicked(const QModelIndex& index);

private:
    void createContextMenu();
    void prepareContextMenu(const SyncError& error);

private:
    QMenu *context_menu_;
};


class SyncErrorsTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    SyncErrorsTableModel(QObject *parent=0);

    int rowCount(const QModelIndex& parent=QModelIndex()) const;
    int columnCount(const QModelIndex& parent=QModelIndex()) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    SyncError errorAt(size_t i) const { return (i >= errors_.size()) ? SyncError() : errors_[i]; }

    void onResize(const QSize& size);

public slots:
    void updateErrors();

private:

    std::vector<SyncError> errors_;
    QTimer *update_timer_;
    int repo_name_column_width_;
    int path_column_width_;
    int error_column_width_;
};

#endif // SEAFILE_CLIENT_SYNC_ERRORS_DIALOG_H
