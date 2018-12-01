#ifndef MODDATA_H
#define MODDATA_H

#include "_uo_map_qs.h"

namespace md {
enum ModData       { Row, Busy, Size };
typedef std::tuple < int, bool, qint64 > data;
typedef std::unordered_map<QString, data> modData;

inline data newData(const int row, const bool busy=true)
{ return { row, busy, 0 }; }

inline void setRow(data &data, const int row)
{ std::get<int(md::Row)>(data) = row; }

inline bool exists(const modData &modData, const QString &modName)
{ return modData.find(modName) != modData.end(); }

inline bool busy(const data &data)
{ return std::get<int(md::Busy)>(data); }

inline qint64 size(const data &data)
{ return std::get<int(md::Size)>(data); }
}

#endif // MODDATA_H
