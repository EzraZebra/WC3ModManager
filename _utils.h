#ifndef UTILS_H
#define UTILS_H

#include <QIcon>

//std QString hash template
namespace std {
template<> struct hash<QString> {
    size_t operator()(const QString& s) const {
        return qHash(s);
    }
};
}

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
