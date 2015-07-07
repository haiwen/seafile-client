#include <jansson.h>
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

bool Json::getBool(const char *key) const
{
    json_t *value = json_object_get(json_, key);
    if (json_is_false(value))
        return false;
    return json_is_true(value) || json_integer_value(value);
}
