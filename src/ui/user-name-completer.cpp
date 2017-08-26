#include <QLineEdit>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QtWidgets>

#include "api/api-error.h"
#include "api/contact-share-info.h"
#include "api/requests.h"
#include "avatar-service.h"

#include "user-name-completer.h"

namespace
{
const int kSearchDelayInterval = 150;
const qint64 kCacheEntryExpireMSecs = 5 * 1000;

enum {
    USER_COLUMN_AVATAR = 0,
    USER_COLUMN_NAME,
    USER_MAX_COLUMN
};

} // anonymous namespace

SeafileUserNameCompleter::SeafileUserNameCompleter(const Account &account,
                                                   QLineEdit *parent)
    : QObject(parent), account_(account), editor_(parent)
{
    popup_ = new QTreeWidget;
    popup_->setWindowFlags(Qt::Popup);
    popup_->setFocusPolicy(Qt::NoFocus);
    popup_->setFocusProxy(parent);
    popup_->setMouseTracking(true);

    popup_->setColumnCount(USER_MAX_COLUMN);
    popup_->setUniformRowHeights(true);
    popup_->setRootIsDecorated(false);
    popup_->setEditTriggers(QTreeWidget::NoEditTriggers);
    popup_->setSelectionBehavior(QTreeWidget::SelectRows);
    popup_->setFrameStyle(QFrame::Box | QFrame::Plain);
    popup_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    popup_->header()->hide();

    popup_->installEventFilter(this);

    connect(popup_,
            SIGNAL(itemClicked(QTreeWidgetItem *, int)),
            SLOT(doneCompletion()));

    timer_ = new QTimer(this);
    timer_->setSingleShot(true);
    timer_->setInterval(kSearchDelayInterval);
    connect(timer_, SIGNAL(timeout()), SLOT(autoSuggest()));
    // Calling start on the timer would effectively stop it & restart again.
    // Thus we can achieve: only show the completion list when the user has not
    // typed for a little while, here 150ms.
    connect(editor_, SIGNAL(textEdited(QString)), timer_, SLOT(start()));

    connect(AvatarService::instance(), SIGNAL(avatarUpdated(const QString&, const QImage&)),
            this, SLOT(onAvatarUpdated(const QString&, const QImage&)));
}

SeafileUserNameCompleter::~SeafileUserNameCompleter()
{
    popup_->deleteLater();
}

bool SeafileUserNameCompleter::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj != popup_)
        return false;

    if (ev->type() == QEvent::MouseButtonPress) {
        popup_->hide();
        editor_->setFocus();
        return true;
    }

    if (ev->type() == QEvent::KeyPress) {
        bool consumed = false;
        int key = static_cast<QKeyEvent *>(ev)->key();
        switch (key) {
            case Qt::Key_Enter:
            case Qt::Key_Return:
                doneCompletion();
                consumed = true;
                break;

            case Qt::Key_Escape:
                popup_->hide();
                editor_->setFocus();
                consumed = true;
                break;

            // Pass through the item navigation opeations to the tree widget.
            case Qt::Key_Up:
            case Qt::Key_Down:
            case Qt::Key_Home:
            case Qt::Key_End:
            case Qt::Key_PageUp:
            case Qt::Key_PageDown:
                break;

            default:
                editor_->setFocus();
                editor_->event(ev);
                popup_->hide();
                break;
        }

        return consumed;
    }

    return false;
}

void SeafileUserNameCompleter::showCompletion(const QList<SeafileUser> &users, const QString& pattern)
{
    if (users.isEmpty())
        return;

    if (pattern != editor_->text().trimmed()) {
        // The user has changed the text, so the completions are no longer
        // useful.
        // printf("pattern changed from \"%s\" to \"%s\"\n",
        //        pattern.toUtf8().data(),
        //        editor_->text().trimmed().toUtf8().data());
        return;
    }

    popup_->setUpdatesEnabled(false);
    popup_->clear();
    foreach (const SeafileUser &user, users) {
        // Do not list the user itself in the completion list.
        if (user.email == account_.username) {
            continue;
        }

        AvatarService *service = AvatarService::instance();
        QIcon avatar = QPixmap::fromImage(service->getAvatar(user.email));

        QString text =
            QString("%1 <%2>").arg(user.name).arg(user.getDisplayEmail());
        QTreeWidgetItem *item;
        item = new QTreeWidgetItem(popup_);
        item->setIcon(USER_COLUMN_AVATAR, avatar);
        item->setText(USER_COLUMN_NAME, text);
        item->setData(USER_COLUMN_NAME, Qt::UserRole, QVariant::fromValue(user));
    }
    popup_->setCurrentItem(popup_->topLevelItem(0));
    popup_->resizeColumnToContents(USER_COLUMN_AVATAR);
    popup_->setUpdatesEnabled(true);

    popup_->move(editor_->mapToGlobal(QPoint(0, editor_->height())));

    int w = editor_->width();
    int maxVisibleItems = 7;
    int h = (popup_->sizeHintForRow(0) *
                 qMin(maxVisibleItems, popup_->model()->rowCount()) +
             3) +
            3;
    h = qMax(h, popup_->minimumHeight());
    // printf("w = %d, h = %d\n", w, h);

    QPoint pos = editor_->mapToGlobal(QPoint(0, editor_->height()));
    popup_->setGeometry(pos.x(), pos.y(), w, h);

    popup_->setFocus();
    if (!popup_->isVisible())
        popup_->show();
}

void SeafileUserNameCompleter::doneCompletion()
{
    timer_->stop();
    popup_->hide();
    editor_->setFocus();
    QTreeWidgetItem *item = popup_->currentItem();
    if (item) {
        SeafileUser user = item->data(USER_COLUMN_NAME, Qt::UserRole).value<SeafileUser>();
        current_selected_user_ = user;
        editor_->setText(user.name);
        QMetaObject::invokeMethod(editor_, "returnPressed");
    }
}

void SeafileUserNameCompleter::autoSuggest()
{
    current_selected_user_ = SeafileUser();
    QString pattern = editor_->text().trimmed();
    if (pattern.isEmpty()) {
        return;
    }

    if (cached_completion_users_by_pattern_.contains(pattern) &&
        cached_completion_users_by_pattern_[pattern].ts +
                kCacheEntryExpireMSecs >
            QDateTime::currentMSecsSinceEpoch()) {
        // printf("cached results for %s\n", pattern.toUtf8().data());
        showCompletion(
            cached_completion_users_by_pattern_[pattern].users.toList(), pattern);
        return;
    }

    if (in_progress_search_requests_.contains(pattern)) {
        // printf("already a request for %s\n", pattern.toUtf8().data());
        return;
    }

    // printf("request completions for username %s\n", pattern.toUtf8().data());
    SearchUsersRequest *req = new SearchUsersRequest(account_, pattern);
    req->setParent(this);
    connect(req,
            SIGNAL(success(const QList<SeafileUser> &)),
            this,
            SLOT(onSearchUsersSuccess(const QList<SeafileUser> &)));
    connect(req,
            SIGNAL(failed(const ApiError &)),
            this,
            SLOT(onSearchUsersFailed(const ApiError &)));
    req->send();

    in_progress_search_requests_.insert(pattern);
}

void SeafileUserNameCompleter::preventSuggest()
{
    timer_->stop();
}

void SeafileUserNameCompleter::onSearchUsersSuccess(
    const QList<SeafileUser> &users)
{
    SearchUsersRequest *req = qobject_cast<SearchUsersRequest *>(sender());
    in_progress_search_requests_.remove(req->pattern());
    req->deleteLater();

    // printf("get %d results for pattern %s\n",
    //        users.size(),
    //        req->pattern().toUtf8().data());

    cached_completion_users_by_pattern_[req->pattern()] = {
        QSet<SeafileUser>::fromList(users),
        QDateTime::currentMSecsSinceEpoch()};
    showCompletion(users, req->pattern());
}

void SeafileUserNameCompleter::onSearchUsersFailed(const ApiError &error)
{
    SearchUsersRequest *req = qobject_cast<SearchUsersRequest *>(sender());
    in_progress_search_requests_.remove(req->pattern());
    req->deleteLater();
}

const SeafileUser& SeafileUserNameCompleter::currentSelectedUser() const
{
    return current_selected_user_;
}

void SeafileUserNameCompleter::onAvatarUpdated(const QString& email,
                                               const QImage& avatar)
{
    for (int i = 0; i < popup_->topLevelItemCount(); i++) {
        QTreeWidgetItem* item =  popup_->topLevelItem(i);
        const QString username_email = item->data(USER_COLUMN_NAME, Qt::DisplayRole).toString();
        if (username_email.contains(email)) {
            item->setIcon(USER_COLUMN_AVATAR, QPixmap::fromImage(avatar));
        }
    }
}
