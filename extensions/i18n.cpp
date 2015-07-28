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
    lang_dict_["get share link"] = "获取共享链接";
    lang_dict_["lock this file"] = "锁定该文件";
    lang_dict_["unlock this file"] = "解锁该文件";
}

void I18NHelper::initGermanDict()
{
    lang_dict_["get share link"] = "Link zu dieser Datei freigeben und";
    lang_dict_["lock this file"] = "Diese Datei sperren";
    lang_dict_["unlock this file"] = "diese Datei zu entsperren";
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
