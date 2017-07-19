#include "loading-label.h"

namespace {

const char *kRefreshIconFileName = ":/images/toolbar/refresh-alpha.png";
const char *kOrangeIconFileName = ":/images/toolbar/refresh-orange-alpha@3x.png";

} // namespace



LoadingLabel::LoadingLabel(QWidget *parent)
    : QLabel()
{
    resize(20, 20);
    setScaledContents(true);

    setPixmap(QPixmap(kRefreshIconFileName));

    show();
}

void LoadingLabel::enterEvent(QEvent *event)
{
    setPixmap(QPixmap(kOrangeIconFileName));
}

void LoadingLabel::leaveEvent(QEvent *event)
{
    setPixmap(QPixmap(kRefreshIconFileName));
}

void LoadingLabel::mousePressEvent(QMouseEvent *event)
{
    emit refresh();
}
