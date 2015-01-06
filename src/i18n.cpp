#include "i18n.h"
#include <QTranslator>
#include <QLibraryInfo>
#include <QApplication>
#include <QSettings>
#include <QDebug>

namespace {
const char* langs[] = {
    NULL, //reserved for system locale
    "ca",
    "de_DE",
    "en",
    "es",
    "es_AR",
    "es_MX",
    "fr_FR",
    "he_IL",
    "hu_HU",
    "is",
    "it",
    "ko_KR",
    "nl_BE",
    "pl_PL",
    "pt_BR",
    "pt_PT",
    "ru",
    "sk_SK",
    "uk",
    "zh_CN",
    "zh_TW",
    "tr",
    NULL
    };
void saveCurrentLanguage(int langIndex) {
    QSettings settings;

    settings.beginGroup("Language");
    settings.setValue("current", QString(langs[langIndex]));
    settings.endGroup();
}

int loadCurrentLanguage() {
    QSettings settings;

    settings.beginGroup("Language");
    QString current = settings.value("current").toString();
    settings.endGroup();

    // system locale
    if (current.isEmpty()) {
        return 0;
    }

    const char** pos = langs; /* skip first one*/
    while(*++pos != NULL) {
        if (*pos == current)
            break;
    }
    return pos - langs;
}

} // anonymous namespace

I18NHelper *I18NHelper::instance_ = NULL;

I18NHelper::I18NHelper()
    : qt_translator_(new QTranslator),
      my_translator_(new QTranslator)
{
    qApp->installTranslator(qt_translator_.data());
    qApp->installTranslator(my_translator_.data());
}

I18NHelper::~I18NHelper() {
}

void I18NHelper::init() {
    int pos = preferredLanguage();
    if (langs[pos] == NULL) // NULL is reserved for system locale
        setLanguage(0);
    else
        setLanguage(pos);
}

int I18NHelper::preferredLanguage() {
    return loadCurrentLanguage();
}

void I18NHelper::setPreferredLanguage(int langIndex) {
    const QList<QLocale> &locales = getInstalledLocales();
    if (langIndex < 0 || langIndex >= locales.size())
        return;
    saveCurrentLanguage(langIndex);
}

bool I18NHelper::setLanguage(int langIndex) {
    const QList<QLocale> &locales = getInstalledLocales();
    if (langIndex < 0 || langIndex >= locales.size())
        return false;

    const QLocale &locale = locales[langIndex];
    qDebug() << "[i18n] using language :" << (langIndex == 0 ? "system locale" : locale.name());
#if defined(Q_WS_WIN)
    qt_translator_->load("qt_" + locale.name());
#else
    qt_translator_->load("qt_" + locale.name(),
                      QLibraryInfo::location(QLibraryInfo::TranslationsPath));
#endif

#if QT_VERSION >= 0x040800 && !defined(Q_WS_MAC)
    QString lang = QLocale::languageToString(locale.language());

    if (lang != "en") {
        bool success;
        success = my_translator_->load(locale,            // locale
                                       "",                // file name
                                       "seafile_",        // prefix
                                       ":/i18n/",         // folder
                                       ".qm");            // suffix

        if (!success) {
            my_translator_->load(QString(":/i18n/seafile_%1.qm").arg(langs[langIndex]));
        }
    }
#else
    // note:
    //      locales[pos].name() != langs[pos]
    //      e.g. "tr_TR" != "tr"
    my_translator_->load(QString(":/i18n/seafile_%1.qm").arg(langs[langIndex]));
#endif

    return true;
}

const QList<QLocale> &I18NHelper::getInstalledLocales() {
    static QList<QLocale> locales;
    if (locales.empty()) {
        locales.push_back(QLocale::system());
        const char** next = langs; /* skip the first one*/
        while(*++next != NULL)
            locales.push_back(QLocale(*next));
    }
    return locales;
}
