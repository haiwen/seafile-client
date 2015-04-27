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
    "nl_NL",
    "lv",
    "ja",
    "sv",
    "cs_CZ",
    "el_GR",
    "nb_NO"
    };
SEAFILE_CONSTEXPR int numOfLangs = arrayLengthOf(langs);

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

    // if it is empty, regard it as system locale
    if (current.isEmpty())
        return 0;

    int i;
    for (i = numOfLangs - 1; i != 0; --i)
        if (current == langs[i])
            break;
    // if it is invalid and not found,
    // i is 0 and then return 0 directly (system locale)
    return i;
}

} // anonymous namespace

I18NHelper *I18NHelper::instance_ = NULL;

I18NHelper::I18NHelper()
    : qt_translator_(new QTranslator),
      my_translator_(new QTranslator)
{
}

I18NHelper::~I18NHelper() {
}

void I18NHelper::init() {
    qApp->installTranslator(qt_translator_.data());
    qApp->installTranslator(my_translator_.data());
    setLanguage(preferredLanguage());
}

int I18NHelper::preferredLanguage() {
    return loadCurrentLanguage();
}

void I18NHelper::setPreferredLanguage(int langIndex) {
    if (langIndex < 0 || langIndex >= numOfLangs)
        return;
    saveCurrentLanguage(langIndex);
}

bool I18NHelper::setLanguage(int langIndex) {
    const QList<QLocale> &locales = getInstalledLocales();
    if (langIndex < 0 || langIndex >= numOfLangs)
        return false;

    const QLocale &locale = locales[langIndex];
#if defined(Q_OS_WIN32)
    qt_translator_->load("qt_" + locale.name());
#else
    qt_translator_->load("qt_" + locale.name(),
                      QLibraryInfo::location(QLibraryInfo::TranslationsPath));
#endif

#if QT_VERSION >= QT_VERSION_CHECK(4, 8, 0) && !defined(Q_OS_MAC)
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
        locales.reserve(numOfLangs);
        locales.push_back(QLocale::system());
        for (int i = 1; i != numOfLangs; ++i)
            locales.push_back(QLocale(langs[i]));
    }
    return locales;
}
