#ifndef UTILS_H
#define UTILS_H

#include <QIcon>

namespace u {
inline QIcon largestIcon(const QIcon &icon)
{
    QSize max(0, 0);
    for(QSize &size : icon.availableSizes())
        if(size.width()*size.height() >= max.width()*max.height()) max = size;

    return icon.pixmap(max);
}

inline bool isValidFileName(const QString &fileName)
{
    return !fileName.contains('<') && !fileName.contains('>') && !fileName.contains(':')
        && !fileName.contains('"') && !fileName.contains('/') && !fileName.contains('\\')
        && !fileName.contains('|') && !fileName.contains('?') && !fileName.contains('*');
}
}

#endif // UTILS_H
