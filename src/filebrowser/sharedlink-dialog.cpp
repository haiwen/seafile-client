#include "sharedlink-dialog.h"

#include <QtGlobal>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtWidgets>
#else
#include <QtGui>
#endif

SharedLinkDialog::SharedLinkDialog(const QString &text, QWidget *parent)
  : text_(text)
{

    setWindowTitle(tr("Share Link"));
    QVBoxLayout *layout = new QVBoxLayout;

    QLabel *label = new QLabel(tr("Share link:"));
    layout->addWidget(label);
    layout->setSpacing(5);
    layout->setContentsMargins(9, 9, 9, 9);

    QLineEdit *editor = new QLineEdit;
    editor->setText(text_);
    editor->selectAll();
    editor->setReadOnly(true);
    layout->addWidget(editor);

    QHBoxLayout *hlayout = new QHBoxLayout;

    QWidget *spacer = new QWidget;
    spacer->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    hlayout->addWidget(spacer);

    QWidget *spacer2 = new QWidget;
    spacer2->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    hlayout->addWidget(spacer2);

    QPushButton *copy_to = new QPushButton(tr("Copy to clipboard"));
    hlayout->addWidget(copy_to);
    connect(copy_to, SIGNAL(clicked()), this, SLOT(onCopyText()));

    QPushButton *ok = new QPushButton(tr("Ok"));
    hlayout->addWidget(ok);
    connect(ok, SIGNAL(clicked()), this, SLOT(accept()));

    layout->addLayout(hlayout);

    setLayout(layout);

    setMinimumWidth(300);
    setMaximumWidth(400);
}

void SharedLinkDialog::onCopyText()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(text_);
}

