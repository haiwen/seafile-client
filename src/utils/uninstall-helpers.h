#ifndef SEAFILE_CLIENT_UTILS_UNINSTALL_HELPERS_H
#define SEAFILE_CLIENT_UTILS_UNINSTALL_HELPERS_H

#include <QString>

void do_ping();
/**
 * Stop running seafile client.
 */
void do_stop();

/**
 * Remove ccnet and seafile-data
 */
void do_remove_user_data();

int get_ccnet_dir(QString *ret);
int get_seafile_data_dir(const QString& ccnet_dir, QString *ret);
int delete_dir_recursively(const QString& path_in);

#endif // SEAFILE_CLIENT_UTILS_UNINSTALL_HELPERS_H
