#include <QtGlobal>
#include <QtWidgets>

#include "seafile-applet.h"
#include "empty-folder-view.h"

EmptyFolderView::EmptyFolderView(QWidget *parent, bool readonly)
    : QLabel(parent),
      readonly_(readonly)
{
    setAcceptDrops(true);
    setText(tr("This folder is empty."));
    setAlignment(Qt::AlignCenter);
    setStyleSheet("background-color: white;");
    installEventFilter(this);
}

bool EmptyFolderView::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::DragEnter) {
        QDragEnterEvent* ev = (QDragEnterEvent*)event;
        // only handle external source currently
        if(ev->source() != NULL)
            return false;
        // Otherwise it might be a MoveAction which is unacceptable
        ev->setDropAction(Qt::CopyAction);
        // trivial check
        if(ev->mimeData()->hasFormat("text/uri-list"))
            ev->accept();
        return true;
    }
    else if (event->type() == QEvent::Drop) {
        QDropEvent* ev = (QDropEvent*)event;
        // only handle external source currently
        if(ev->source() != NULL)
            return false;

        QList<QUrl> urls = ev->mimeData()->urls();

        if(urls.isEmpty())
            return false;

        QStringList paths;
        Q_FOREACH(const QUrl& url, urls)
        {
            QString path = url.toLocalFile();
#if defined(Q_OS_MAC) && (QT_VERSION <= QT_VERSION_CHECK(5, 4, 0))
            path = utils::mac::fix_file_id_url(path);
#endif
            if(path.isEmpty())
                continue;
            paths.push_back(path);
        }

        ev->accept();

        if (readonly_) {
            seafApplet->warningBox(tr("You do not have permission to upload to this folder"), this);
        } else {
            emit dropFile(paths);
        }
        return true;
    }

    return QWidget::eventFilter(obj, event);
}
