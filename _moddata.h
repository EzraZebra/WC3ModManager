#ifndef MODDATA_H
#define MODDATA_H

#include <QString>
#include <map>

namespace md {
enum ModData       { Row, Busy };
typedef std::tuple < int, bool > data;
typedef std::map<QString, data> modData;

inline data newData(const int row=0, const bool busy=false)
{ return { row, busy }; }

inline void setRow(data &data, const int row)
{ std::get<int(md::Row)>(data) = row; }

inline bool exists(const modData &modData, const QString &modName)
{ return modData.find(modName) != modData.end(); }
}

#endif // MODDATA_H
