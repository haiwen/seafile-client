
#include "starred-file-item.h"

StarredFileItem::StarredFileItem(const StarredItem& file)
    : file_(file)
{
    setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
}
