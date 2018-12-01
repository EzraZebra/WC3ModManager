#ifndef UO_MAP_QS_H
#define UO_MAP_QS_H

#include <QString>
#include <qhashfunctions.h>
#include <unordered_map>

//std QString hash template
namespace std {
template<> struct hash<QString> {
    size_t operator()(const QString& s) const {
        return qHash(s);
    }
};
}

#endif // UO_MAP_QS_H
