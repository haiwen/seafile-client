#ifndef _SEAF_I18N_H
#define _SEAF_I18N_H
#include "utils/stl_extra.h"
#include <QList>
#include <QLocale>
#include <QStringList>
#include <QScopedPointer>

class QTranslator;
class I18NHelper {
public:
    static I18NHelper* getInstance() {
      if (!instance_) {
          static I18NHelper i18n;
          instance_ = &i18n;
      }
      return instance_;
    }

    void init();

    QStringList getLanguages() {
        QStringList languages;
        Q_FOREACH(const QLocale& locale, getInstalledLocales())
        {
            languages.push_back(
                QString("%1 - %2").arg(QLocale::languageToString(locale.language()))
                                  .arg(QLocale::countryToString(locale.country())));
        }
        languages.front() = "-- System --";
        return languages;
    }

    // return between 0 and numOfLangs -1
    int preferredLanguage();
    // accept index between 0 and numOfLangs -1
    void setPreferredLanguage(int langIndex);
private:
    I18NHelper();
    ~I18NHelper();
    I18NHelper(const I18NHelper&) SEAFILE_DELETED;

    const QList<QLocale> &getInstalledLocales();
    bool setLanguage(int langIndex);
    QScopedPointer<QTranslator> qt_translator_;
    QScopedPointer<QTranslator> my_translator_;

    static I18NHelper *instance_;
};


#endif // _SEAF_I18N_H
