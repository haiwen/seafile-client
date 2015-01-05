
#include "starred-file-item.h"

StarredFileItem::StarredFileItem(const StarredFile& file)
    : file_(file)
{
    setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
}
