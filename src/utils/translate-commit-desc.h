
#ifndef SEAFILE_CELINT_TRANSLATE_COMMIT_DESC_H
#define SEAFILE_CELINT_TRANSLATE_COMMIT_DESC_H

#include <QString>

QString
translateCommitDesc (const QString& input);

QString
translateCommitDescV2(const QString& path, const QString& file_name, const QString& repo_name,
                      const QString& obj_type, const QString& op_type);

#endif  // SEAFILE_CELINT_TRANSLATE_COMMIT_DESC_H
