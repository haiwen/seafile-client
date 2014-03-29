#ifndef SEAFILE_CLIENT_CERTS_MANAGER_H
#define SEAFILE_CLIENT_CERTS_MANAGER_H

#include <QHash>
#include <QSslCertificate>

struct sqlite3;
struct sqlite3_stmt;

class QUrl;

class CertsManager {
public:
    CertsManager();
    ~CertsManager();

    int start();

    void saveCertificate(const QUrl& url, const QSslCertificate& cert);
    QSslCertificate getCertificate(const QUrl& url);

private:
    void loadCertificates();
    static bool loadCertificatesCB(sqlite3_stmt *stmt, void *data);

    QHash<QString, QSslCertificate> certs_;

    struct sqlite3 *db;
};

#endif // SEAFILE_CLIENT_CERTS_MANAGER_H
