#include "config.h"
#include <QDir>
#include <fstream>
#include <winerror.h>

const QChar   Config::CFG_SEP       = '=';
const QString Config::vOn           = "1",
              Config::vOff          = "0",
              Config::kGamePath     = "GamePath",
              Config::kHideEmpty    = "HideEmptyMods";/*,
              Config::kMounted      = "Mounted",
              Config::kMountedError = "MountedError";*/

Config::Config()
{
 // LOAD CONFIG FILE
    std::ifstream cfgReader(pathCfg);
    for(std::string line; std::getline(cfgReader, line); )
    {
        const QStringList &list = QString::fromStdString(line).split(CFG_SEP);
        if(list.size() >= 2) saveSetting(list[0], list[1]);
    }
    cfgReader.close();

 // LOAD DEFAULTS
    bool configChanged = false;

    if(getSetting(kGamePath).isEmpty())
    {
        HKEY hKey;
        if(regOpenWC3(KEY_READ, hKey))
        {
            QString gamePath;
            DWORD type = REG_SZ, size = 1024;
            wchar_t result[MAX_PATH];

            if(RegQueryValueEx(hKey, L"GamePath", nullptr, &type, LPBYTE(&result), &size)
                    == ERROR_SUCCESS)
                gamePath = QString::fromWCharArray(result);

            if(gamePath.isEmpty()  && RegQueryValueEx(hKey, L"InstallPath", nullptr, &type, LPBYTE(&result), &size)
                    == ERROR_SUCCESS)
                gamePath = QString::fromWCharArray(result);

            if(!gamePath.isEmpty())
            {
                saveSetting(kGamePath, gamePath);
                configChanged = true;
            }
        }
        RegCloseKey(hKey);
    }
    if(getSetting(kHideEmpty).isEmpty())
    {
        saveSetting(kHideEmpty, vOn);
        configChanged = true;
    }

    if(configChanged) saveConfig();
}

void Config::saveSetting(const QString &key, QString value)
{
    if(!key.isEmpty())
    {
        if(value.isEmpty()) deleteSetting(key);
        else
        {
            if(key == kGamePath) value = QDir::fromNativeSeparators(value);
            if(settings.find(key) == settings.end()) settings.insert({ key, value });
            else settings[key] = value;
        }
    }
}

void Config::saveConfig() const
{
    std::ofstream cfgWriter(pathCfg);

    for(const std::pair<const QString, const QString> &setting : settings)
        cfgWriter << QString("%0%1%2").arg(setting.first, CFG_SEP, setting.second).toStdString() << std::endl;

    cfgWriter.close();
}

bool Config::regOpenWC3(const REGSAM &accessMode, HKEY &hKey)
{
    return RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Blizzard Entertainment\\Warcraft III", 0, accessMode, &hKey)
                == ERROR_SUCCESS;
}
