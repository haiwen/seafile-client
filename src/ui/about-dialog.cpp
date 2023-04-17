#include <QtWidgets>

#include "seafile-applet.h"
#include "utils/utils.h"

#include "about-dialog.h"

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
}
