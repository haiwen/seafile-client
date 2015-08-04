#ifndef SEAFILE_CLIENT_UTILS_JSON_UTILS_H

#include <jansson.h>
#include <QString>

// A convenient class to access jasson `json_t` struct
class Json {
public:
    Json(const json_t *root);

    QString getString(const char *name) const;
    qint64 getLong(const char *name) const;
    bool getBool(const char *name) const;

private:
    const json_t *json_;
};

#endif  // SEAFILE_CLIENT_UTILS_JSON_UTILS_H
