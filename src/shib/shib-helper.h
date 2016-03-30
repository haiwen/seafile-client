#ifndef SEAFILE_CLIENT_SHIB_HELPER_H
#define SEAFILE_CLIENT_SHIB_HELPER_H

#include <QWebEnginePage>

class QWebEngineCertificateError;
class SeafileQWebEnginePage : public QWebEnginePage
{
    Q_OBJECT
public:
    SeafileQWebEnginePage(QObject *parent = 0);

protected:
    bool certificateError(
        const QWebEngineCertificateError &certificateError);
};

#endif /* SEAFILE_CLIENT_SHIB_HELPER_H */
