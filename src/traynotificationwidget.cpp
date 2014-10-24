#include "traynotificationwidget.h"

TrayNotificationWidget::TrayNotificationWidget(QPixmap pixmapIcon, QString headerText, QString messageText) : QWidget(0)
{
    setWindowFlags(
        #if defined(Q_OS_MAC)
            Qt::SubWindow | // This type flag is the second point
        #else
            Qt::Tool |
        #endif
            Qt::FramelessWindowHint |
            Qt::WindowSystemMenuHint |
            Qt::WindowStaysOnTopHint
        );
    setAttribute(Qt::WA_NoSystemBackground, true);
    // set the parent widget's background to translucent
    setAttribute(Qt::WA_TranslucentBackground, true);

    //setAttribute(Qt::WA_ShowWithoutActivating, true);
    setFixedWidth(310);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    // create a display widget for displaying child widgets
    QWidget* displayWidget = new QWidget;
    displayWidget->setStyleSheet(".QWidget { background-color: rgba(0, 0, 0, 75%); border-width: 1px; border-style: solid; border-radius: 10px; border-color: #555555; } .QWidget:hover { background-color: rgba(68, 68, 68, 75%); border-width: 2px; border-style: solid; border-radius: 10px; border-color: #ffffff; }");
    displayWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    QLabel* icon = new QLabel;
    icon->setPixmap(pixmapIcon);
    icon->setMaximumSize(32, 32);
    QLabel* header = new QLabel;
    header->setStyleSheet("QLabel { color: #ffffff; font-weight: bold; font-size: 12px; }");
    header->setText(headerText);
    header->setFixedWidth(200);
    header->setWordWrap(true);
    header->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    QTextEdit *message = new QTextEdit;
    message->setReadOnly(true);
    message->setFrameStyle(QFrame::NoFrame);
    message->setLineWrapMode(QTextEdit::WidgetWidth);
    message->setWordWrapMode(QTextOption::WrapAnywhere);
    QPalette pal = palette();
    pal.setColor(QPalette::Base, Qt::transparent);
    message->setPalette(pal);
    message->setStyleSheet("QTextEdit { color: #ffffff; font-size: 10px; }");
    message->setText(messageText);
    message->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QVBoxLayout* vl = new QVBoxLayout;
    vl->addWidget(header);
    vl->addWidget(message);
    QHBoxLayout* displayMainLayout = new QHBoxLayout;
    displayMainLayout->addWidget(icon);
    displayMainLayout->addLayout(vl);

    displayWidget->setLayout(displayMainLayout);
    QHBoxLayout* containerLayout = new QHBoxLayout;
    containerLayout->addWidget(displayWidget);
    setLayout(containerLayout);
    show();
    resize(this->size().width(), (int)((message->document()->size().height() + header->height()) +70));

    timeout = new QTimer(this);
    connect(timeout, SIGNAL(timeout()), this, SLOT(fadeOut()));
    timeout->start(3000);
}

void TrayNotificationWidget::fadeOut()
{
    emit deleted(this);
    this->hide();
}
