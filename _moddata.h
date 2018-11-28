#ifndef MODDATA_H
#define MODDATA_H

#include <QString>
#include <map>

enum class ModData { Row, Busy };
typedef std::tuple < int, bool > mod_t;
typedef std::map<QString, mod_t> mod_m;

#endif // MODDATA_H
