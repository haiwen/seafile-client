
#ifndef SEAFILE_CELINT_TRANSLATE_COMMIT_DESC_H
#define SEAFILE_CELINT_TRANSLATE_COMMIT_DESC_H

#include <QString>

QString
translateCommitDesc (const QString& input);

QString
translateComitActivitiesDesc(QString path, QString file_name, QString repo_name, QString obj_type, QString op_type);

#endif  // SEAFILE_CELINT_TRANSLATE_COMMIT_DESC_H
