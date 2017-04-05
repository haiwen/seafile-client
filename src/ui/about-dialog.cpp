#include <QtWidgets>

#include "seafile-applet.h"
#include "utils/utils.h"

#include "about-dialog.h"

#ifdef HAVE_SPARKLE_SUPPORT
#include "auto-update-service.h"

namespace {

} // namespace
#endif

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);
    setWindowTitle(tr("About %1").arg(getBrand()));
    setWindowIcon(QIcon(":/images/seafile.png"));
    setWindowFlags((windowFlags() & ~Qt::WindowContextHelpButtonHint) |
                   Qt::WindowStaysOnTopHint);

    version_text_ = tr("<h4>%1 Client %2</h4>")
                       .arg(getBrand())
	               .arg(STRINGIZE(SEAFILE_CLIENT_VERSION))
#ifdef SEAFILE_CLIENT_REVISION
                       .append(tr("<h5> REV %1 </h5>"))
                       .arg(STRINGIZE(SEAFILE_CLIENT_REVISION))
#endif
		       ;
    mVersionText->setText(version_text_);

    connect(mOKBtn, SIGNAL(clicked()), this, SLOT(close()));

#ifdef HAVE_SPARKLE_SUPPORT
    mCheckUpdateBtn->setVisible(true);
    connect(mCheckUpdateBtn, SIGNAL(clicked()), this, SLOT(checkUpdate()));
#else
    mCheckUpdateBtn->setVisible(false);
#endif
}

#ifdef HAVE_SPARKLE_SUPPORT
void AboutDialog::checkUpdate()
{
    AutoUpdateService::instance()->setRequestParams();
    AutoUpdateService::instance()->checkUpdate();
    close();
}
#endif
