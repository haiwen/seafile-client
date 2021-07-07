#ifndef _SEAF_I18N_H
#define _SEAF_I18N_H
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
            QString str_location = QLocale::countryToString(locale.country());
            if (str_location == "China") {
                str_location = "Simplified";
            } else if (str_location == "Taiwan") {
                str_location = "Traditional";
            }
            languages.push_back(
                QString("%1 - %2").arg(QLocale::languageToString(locale.language()))
                                  .arg(str_location));
        }
        languages.front() = "-- System --";
        return languages;
    }

    bool isChinese();
    int preferredLanguage();
    void setPreferredLanguage(int langIndex);
private:
    I18NHelper();
    ~I18NHelper();
    I18NHelper(const I18NHelper&); // = delete

    const QList<QLocale> &getInstalledLocales();
    bool setLanguage(int langIndex);
    QScopedPointer<QTranslator> qt_translator_;
    QScopedPointer<QTranslator> my_translator_;

    static I18NHelper *instance_;
};


#endif // _SEAF_I18N_H
