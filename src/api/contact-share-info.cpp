#include <QHash>
#include "contact-share-info.h"

uint qHash(const SeafileUser& user, uint seed) {
    return qHash(user.email, seed);
}
