#include "utils.h"
#include <map>

#ifndef CONFIG_H
#define CONFIG_H

class Config
{

public:
    Config(Utils*);
    void saveSetting(std::string, std::string);
    void deleteSetting(std::string);
    std::string getSetting(std::string);
    void saveConfig();
    Utils* utils;
    std::string cfgPath;
    std::string modPath;
    std::string outFilesPath;
    std::string backupFilesPath;
    std::map<std::string, std::string> settings;
};

#endif // CONFIG_H
