#ifndef MODDATA_H
#define MODDATA_H

#include <QString>
#include <map>

enum class ModData { Row, Busy };
typedef std::tuple < int, bool > mod_t;
typedef std::map<QString, mod_t> mod_m;

inline mod_t newModT(const int row=0, const bool busy=false)
{ return { row, busy }; }

#endif // MODDATA_H
