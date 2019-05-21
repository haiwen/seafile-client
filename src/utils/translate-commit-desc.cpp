#include <stdlib.h>
#include <stdio.h>
#include <QHash>
#include <QObject>
#include <QApplication>
#include <QRegExp>
#include <QStringList>

#include "utils/utils.h"
#include "translate-commit-desc.h"
#include "utils/file-utils.h"

namespace {

//const char *kTranslateContext = "MessageListener";

QHash<QString, QString> *verbsMap = NULL;

QHash<QString, QString>*
getVerbsMap()
{
    if (!verbsMap) {
        verbsMap = new QHash<QString, QString>;
        verbsMap->insert("Added", QObject::tr("Added"));
        verbsMap->insert("Added or modified", QObject::tr("Added or modified"));
        verbsMap->insert("Deleted", QObject::tr("Deleted"));
        verbsMap->insert("Removed", QObject::tr("Removed"));
        verbsMap->insert("Modified", QObject::tr("Modified"));
        verbsMap->insert("Renamed", QObject::tr("Renamed"));
        verbsMap->insert("Moved", QObject::tr("Moved"));
        verbsMap->insert("Added directory", QObject::tr("Added directory"));
        verbsMap->insert("Removed directory", QObject::tr("Removed directory"));
        verbsMap->insert("Renamed directory", QObject::tr("Renamed directory"));
        verbsMap->insert("Moved directory", QObject::tr("Moved directory"));
    }

    return verbsMap;
}

QString translateLine(const QString line)
{
    QString operations = ((QStringList)getVerbsMap()->keys()).join("|");
    QString pattern = QString("(%1) \"(.*)\"\\s?(and ([0-9]+) more (files|directories))?").arg(operations);

    QRegExp regex(pattern);

    if (regex.indexIn(line) < 0) {
        return line;
    }

    QString op = regex.cap(1);
    QString file_name = regex.cap(2);
    QString has_more = regex.cap(3);
    QString n_more = regex.cap(4);
    QString more_type = regex.cap(5);

    QString op_trans = getVerbsMap()->value(op, op);

    QString type, ret;
    if (has_more.length() > 0) {
        if (more_type == "files") {
            type = QObject::tr("files");
        } else {
            type = QObject::tr("directories");
        }

        QString more = QObject::tr("and %1 more").arg(n_more);
        ret = QString("%1 \"%2\" %3 %4.").arg(op_trans).arg(file_name).arg(more).arg(type);
    } else {
        ret = QString("%1 \"%2\".").arg(op_trans).arg(file_name);
    }

    return ret;
}

} // namespace


QString
translateCommitDesc(const QString& input)
{
    QString value = input;
    if (value.startsWith("Reverted repo")) {
        value.replace("repo", "library");
    }

    if (value.startsWith("Reverted library")) {
        return value.replace("Reverted library to status at", QObject::tr("Reverted library to status at"));
    } else if (value.startsWith("Reverted file")) {
        QRegExp regex("Reverted file \"(.*)\" to status at (.*)");

        if (regex.indexIn(value) >= 0) {
            QString name = regex.cap(1);
            QString time = regex.cap(2);
            return QObject::tr("Reverted file \"%1\" to status at %2.").arg(name).arg(time);
        }

    } else if (value.startsWith("Recovered deleted directory")) {
        return value.replace("Recovered deleted directory", QObject::tr("Recovered deleted directory"));
    } else if (value.startsWith("Changed library")) {
        return value.replace("Changed library name or description", QObject::tr("Changed library name or description"));
    } else if (value.startsWith("Merged") || value.startsWith("Auto merge")) {
        return QObject::tr("Auto merge by %1 system").arg(getBrand());
    }

    QStringList lines = value.split("\n");
    QStringList out;

    for (int i = 0; i < lines.size(); i++) {
        out << translateLine(lines.at(i));
    }

    return out.join("\n");
}

// path: the path of activity file or folder
// file_name: the name of activity file item
// repo_name: the name of activity repository item
// obj_type: the object of activity item include (file, path, repository)
// op_type: the operation type of obj_name
// old_repo_name: when rename repo the origin repo name
// old_path: when rename path the origin  path name
// old_name: when rename file the origin file name
// clean_trash_days: the days since clean the trash
// out_obj_desc: the description of obj_name
// out out_op_desc: the description of opt_type
void
translateCommitDescV2(const QString& path, const QString& file_name, const QString& repo_name,
                      const QString& obj_type, const QString& op_type, const QString& old_repo_name,
                      const QString& old_path, const QString& old_name, int clean_trash_days,
                      QString *out_obj_desc, QString *out_op_desc)
{
    if (obj_type == "repo") {
        if (op_type == "create") {
            *out_op_desc = QObject::tr("Created libraray");
        } else if (op_type == "rename") {
            *out_op_desc = QObject::tr("Renamed libraray");
        } else if (op_type == "delete") {
            *out_op_desc = QObject::tr("Deleted libraray");
            *out_obj_desc = repo_name;
        } else if (op_type == "recover") {
            *out_op_desc = QObject::tr("Restored libraray");
        } else if (op_type == "clean_up_trash") {
            if (clean_trash_days == 0) {
                *out_op_desc = QObject::tr("Removed all items from trash");
            } else {
                *out_op_desc = QObject::tr("Removed items older than days %1 from trash").arg(clean_trash_days);
            }
        }

        if (op_type == "rename") {
            *out_obj_desc = old_repo_name + " => " + repo_name;
        } else {
            *out_obj_desc = repo_name;
        }

    } else if (obj_type == "draft") {
            *out_op_desc = QObject::tr("Published draft");
            *out_obj_desc = file_name;
    } else if (obj_type == "file") {
        if (op_type == "create") {
            if (file_name.endsWith("(draft).md")) {
                *out_op_desc = QObject::tr("Created draft");
            } else {
                *out_op_desc = QObject::tr("Created file");
            }
        } else if (op_type == "rename") {
            *out_op_desc = QObject::tr("Renamed file");
        } else if (op_type == "delete") {
            if (file_name.endsWith("(draft).md")) {
                *out_op_desc = QObject::tr("Deleted draft");
            } else {
                *out_op_desc = QObject::tr("Deleted file");
            }
        } else if (op_type == "recover") {
            *out_op_desc = QObject::tr("Restored file");
        } else if (op_type == "move") {
            *out_op_desc = QObject::tr("Moved file");
        } else if (op_type == "edit") {
            *out_op_desc = QObject::tr("Updated file");
        }

        if (op_type == "rename") {
            *out_obj_desc = old_name + " => " + file_name;
        } else {
            *out_obj_desc = file_name;
        }

    } else { //dir
        if (op_type == "create") {
            *out_op_desc = QObject::tr("Created folder");
         } else if (op_type == "rename") {
            *out_op_desc = QObject::tr("Renamed folder");
         } else if (op_type == "delete") {
            *out_op_desc = QObject::tr("Deleted folder");
         } else if (op_type == "recover") {
            *out_op_desc = QObject::tr("Restored folder");
         } else if (op_type == "move") {
            *out_op_desc = QObject::tr("Moved folder");
        }
        if (op_type == "rename") {
            *out_obj_desc = old_path + " => " + path;
        } else {
            *out_obj_desc = path;
        }
    }

}
