
#ifndef SEAFILE_CELINT_TRANSLATE_COMMIT_DESC_H
#define SEAFILE_CELINT_TRANSLATE_COMMIT_DESC_H

#include <QString>

QString
translateCommitDesc (const QString& input);

void
translateCommitDescV2(const QString& path, const QString& file_name, const QString& repo_name,
                      const QString& obj_type, const QString& op_type, const QString& old_repo_name,
                      const QString& old_path, const QString& old_name, int clean_trash_days,
                      QString *out_obj_desc, QString *out_op_desc);

#endif  // SEAFILE_CELINT_TRANSLATE_COMMIT_DESC_H
