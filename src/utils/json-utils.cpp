#include "json-utils.h"

Json::Json(const json_t *root)
{
    json_ = root;
}

QString Json::getString(const char *key) const
{
    return QString::fromUtf8(json_string_value(json_object_get(json_, key)));
}

qint64 Json::getLong(const char *key) const
{
    return json_integer_value(json_object_get(json_, key));
}
