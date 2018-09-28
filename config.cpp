#include "config.h"
#include "utils.h"

Config::Config()
{
    //exe path
    wchar_t exePathBuffer[MAX_PATH];
    GetModuleFileName(nullptr, exePathBuffer, MAX_PATH);
    std::string exePath = utils::narrow_str(exePathBuffer);
    exePath = exePath.substr(0, exePath.find_last_of("\\/"));
    utils::valueCorrect("path", &exePath);

    //Set Paths
    cfgPath = exePath+"/config.cfg";
    modPath = exePath.substr(0, exePath.find_last_of("/"))+"/mods"; //parent folder of "bin" in current deploy structure
    outFilesPath = exePath+"/out_files.txt";
    backupFilesPath = exePath+"/backup_files.txt";

    //Load config file
    utils::TxtReader txtReader(cfgPath);
    while(txtReader.next())
    {
        std::pair<std::string, std::string> setting = txtReader.line2setting();
        setSetting(setting.first, setting.second);
    }

    //Load defaults
    bool configChanged = false;

    if(getSetting("GamePath") == "")
    {
        std::string gamePath = "";
        for(int i=0; gamePath == ""; i++)
            switch(i)
            {
                case(0): gamePath = utils::regGet(L"GamePath", REG_SZ);
                         break;
                case(1): gamePath = utils::regGet(L"InstallPath", REG_SZ);
                         break;
                case(2): gamePath = utils::regGetPF86();
                         if(gamePath != "") gamePath += "/Warcraft III";
                         break;
                default: gamePath = "C:/Program Files (x86)/Warcraft III";
            }

        setSetting("GamePath", gamePath);
        configChanged = true;
    }
    if(getSetting("hideEmptyMods") == "")
    {
        setSetting("hideEmptyMods", "1");
        configChanged = true;
    }

    if(configChanged) saveConfig();
}

void Config::setSetting(std::string key, std::string value)
{
    if(key != "")
    {
        if(value == "") deleteSetting(key);
        else
        {
            utils::valueCorrect(key, &value);
            if(settings.find(key) == settings.end()) settings.insert({ key, value });
            else settings.find(key)->second = value;
        }
     }
}

void Config::deleteSetting(std::string key)
{
    if(settings.find(key) != settings.end())
        settings.erase(key);
}

std::string Config::getSetting(std::string key)
{
    if(settings.find(key) == settings.end())
        return "";
    else return settings.find(key)->second;
}

void Config::saveConfig()
{
    std::ofstream cfg(cfgPath.c_str());

    for(std::map<std::string, std::string>::const_iterator it = settings.begin(); it != settings.end(); ++it)
        cfg << it->first+"="+it->second << std::endl;

    cfg.close();
}
