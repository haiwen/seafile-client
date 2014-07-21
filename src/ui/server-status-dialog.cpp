#include "server-status-service.h"
#include "server-status-dialog.h"

ServerStatusDialog::ServerStatusDialog(QWidget *parent) : QDialog(parent)
{
    setupUi(this);

#if defined(Q_OS_MAC)
    layout()->setContentsMargins(8, 9, 9, 4);
    layout()->setSpacing(5);
#endif

    setWindowTitle(tr("Servers connection status"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    refreshStatus();

    connect(ServerStatusService::instance(), SIGNAL(serverStatusChanged()),
            this, SLOT(refreshStatus()));
}

void ServerStatusDialog::refreshStatus()
{
    mList->clear();

    foreach (const ServerStatus& status, ServerStatusService::instance()->statuses()) {
        QListWidgetItem *item = new QListWidgetItem(mList);
        item->setData(Qt::DisplayRole, status.url.host());

        if (status.connected) {
            item->setData(Qt::DecorationRole, QIcon(":/images/sync/ok.png"));
            item->setData(Qt::ToolTipRole, tr("connected"));
        } else {
            item->setData(Qt::DecorationRole, QIcon(":/images/remove-red.png"));
            item->setData(Qt::ToolTipRole, tr("disconnected"));
        }

        mList->addItem(item);
    }
}
