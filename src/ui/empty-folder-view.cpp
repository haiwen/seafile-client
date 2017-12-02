#include <QtGlobal>
#include <QtWidgets>

#include "seafile-applet.h"
#include "empty-folder-view.h"

EmptyFolderView::EmptyFolderView(QWidget *parent)
    : QLabel(parent)
{
    setText(tr("This folder is empty."));
    setAlignment(Qt::AlignCenter);
    setStyleSheet("background-color: white;");
}
