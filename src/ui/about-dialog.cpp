#include <QtWidgets>

#include "seafile-applet.h"
#include "utils/utils.h"

#include "about-dialog.h"

#ifdef HAVE_SPARKLE_SUPPORT
#include "auto-update-service.h"
#endif

namespace {

} // namespace

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);
    setWindowTitle(tr("About %1").arg(getBrand()));
    setWindowIcon(QIcon(":/images/seafile.png"));
    setWindowFlags((windowFlags() & ~Qt::WindowContextHelpButtonHint) |
                   Qt::WindowStaysOnTopHint);

    version_text_ = tr("<h2>%1 Client %2</h2>")
                       .arg(getBrand())
	               .arg(STRINGIZE(SEAFILE_CLIENT_VERSION))
#ifdef SEAFILE_CLIENT_REVISION
                       .append(tr("<h5> REV %1 </h5>"))
                       .arg(STRINGIZE(SEAFILE_CLIENT_REVISION))
#endif
		       ;
    mVersionText->setText(version_text_);

    connect(mOKBtn, SIGNAL(clicked()), this, SLOT(close()));

    mCheckUpdateBtn->setVisible(false);
#ifdef HAVE_SPARKLE_SUPPORT
    if (AutoUpdateService::instance()->shouldSupportAutoUpdate()) {
        mCheckUpdateBtn->setVisible(true);
        connect(mCheckUpdateBtn, SIGNAL(clicked()), this, SLOT(checkUpdate()));
    }
#endif
}

#ifdef HAVE_SPARKLE_SUPPORT
void AboutDialog::checkUpdate()
{
    AutoUpdateService::instance()->checkUpdate();
    close();
}
#endif
