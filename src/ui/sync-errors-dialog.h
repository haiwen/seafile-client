#ifndef SEAFILE_CLIENT_SYNC_ERRORS_DIALOG_H
#define SEAFILE_CLIENT_SYNC_ERRORS_DIALOG_H

#include <vector>

#include <QTableView>
#include <QHeaderView>
#include <QAbstractTableModel>
#include <QDialog>

#include "ui_sync-errors-dialog.h"
#include "rpc/sync-error.h"

class QTimer;
class QStackedWidget;

class SyncError;
class SyncErrorsTableView;
class SyncErrorsTableModel;

class SyncErrorsDialog : public QDialog,
                         public Ui::SyncErrorsDialog
{
    Q_OBJECT

public:
    SyncErrorsDialog(QWidget *parent=0);
    void updateErrors();

private slots:
    void onModelReset();

private:
    void createEmptyView();

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

private:
    void createContextMenu();
    void prepareContextMenu(const SyncError& error);

    QMenu *context_menu_;
};

class SyncErrorsTableHeader : public QHeaderView {
    Q_OBJECT

public:
    SyncErrorsTableHeader(QWidget *parent=0);
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

public slots:
    void updateErrors();

private:

    std::vector<SyncError> errors_;
    QTimer *update_timer_;
};

#endif // SEAFILE_CLIENT_SYNC_ERRORS_DIALOG_H
