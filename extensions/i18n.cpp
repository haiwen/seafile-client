#include "ext-common.h"
#include "ext-utils.h"

#include <string>
#include <map>
#include <memory>

using std::string;

namespace {

string getWin32Locale()
{
  LCID lcid;
  char iso639[10];

  lcid = GetThreadLocale ();

  if (!GetLocaleInfo (lcid, LOCALE_SISO639LANGNAME, iso639, sizeof (iso639)))
      return "C";

  return iso639;
}

class I18NHelper {
public:
    static I18NHelper *instance();

    string getString(const string& src) const;

private:
    I18NHelper();
    static I18NHelper *singleton_;

    void initChineseDict();
    void initGermanDict();

    std::map<string, string> lang_dict_;
};


I18NHelper* I18NHelper::singleton_;

I18NHelper *I18NHelper::instance() {
    if (!singleton_) {
        singleton_ = new I18NHelper();
    }
    return singleton_;
}

void I18NHelper::initChineseDict()
{
    lang_dict_["get seafile download link"] = "获取共享链接";
    lang_dict_["get seafile internal link"] = "获取内部链接";
    lang_dict_["lock this file"] = "锁定该文件";
    lang_dict_["unlock this file"] = "解锁该文件";
    lang_dict_["share to a user"] = "共享给其他用户";
    lang_dict_["share to a group"] = "共享给群组";
    lang_dict_["view file history"] = "查看文件历史";
}

void I18NHelper::initGermanDict()
{
    lang_dict_["get seafile download link"] = "Freigabelink erstellen";
    lang_dict_["get seafile internal link"] = "Internen Link erstellen";
    lang_dict_["lock this file"] = "Datei sperren";
    lang_dict_["unlock this file"] = "Datei entsperren";
    lang_dict_["share to a user"] = "Freigabe für Benutzer/in";
    lang_dict_["share to a group"] = "Freigabe für Gruppe";
    lang_dict_["view file history"] = "Vorgängerversionen";
}

I18NHelper::I18NHelper()
{
    string locale = getWin32Locale();
    if (locale == "zh") {
        initChineseDict();
    } else if (locale == "de") {
        initGermanDict();
    }
}

string I18NHelper::getString(const string& src) const
{
    auto value = lang_dict_.find(src);
    return value != lang_dict_.end() ? value->second : src;
}

} // namespace

namespace seafile {

string getString(const string& src)
{
    string value = I18NHelper::instance()->getString(src);
    std::unique_ptr<wchar_t[]> value_w(utils::utf8ToWString(value));
    return utils::wStringToLocale(value_w.get());
}

}
