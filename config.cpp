#include "config.h"

using namespace std;

Config::Config(Utils* newUtils)
{
    utils = newUtils;

    //Set Paths
    cfgPath = utils->exePath+"/config.cfg";
    modPath = utils->exePath+"/mods";
    outFilesPath = utils->exePath+"/out_files.txt";
    backupFilesPath = utils->exePath+"/backup_files.txt";

    //Load config file
    if(utils->txtReaderStart(cfgPath))
        while(utils->txtReaderNext())
        {
            pair<string, string> setting = utils->line2setting(utils->txtReaderLine);
            saveSetting(setting.first, setting.second);
        }

    //Load defaults
    bool configChanged = false;

    if(loadSetting("GamePath") == "")
    {
        string gamePath = utils->regGet(L"GamePath", REG_SZ);
        if(gamePath == "") gamePath = utils->regGet(L"InstallPath", REG_SZ);
        if(gamePath == "") gamePath = "C:/Program Files (x86)/Warcraft III";

        saveSetting("GamePath", gamePath);
        configChanged = true;
    }
    if(loadSetting("hideEmptyMods") == "")
    {
        saveSetting("hideEmptyMods", "1");
        configChanged = true;
    }

    if(configChanged) saveConfig();
}

void Config::saveSetting(string key, string value)
{
    if(key != "")
    {
        if(value == "") deleteSetting(key);
        else
        {
            utils->valueCorrect(key, &value);
            if(settings.find(key) == settings.end()) settings.insert({ key, value });
            else settings.find(key)->second = value;
        }
     }
}

void Config::deleteSetting(string key)
{
    if(settings.find(key) != settings.end())
        settings.erase(key);
}

string Config::loadSetting(string key)
{
    if(settings.find(key) == settings.end())
        return "";
    else return settings.find(key)->second;
}

void Config::saveConfig()
{
    ofstream cfg(cfgPath.c_str());

    for(map<string, string>::const_iterator it = settings.begin(); it != settings.end(); ++it)
        cfg << it->first+"="+it->second << endl;

    cfg.close();
}
