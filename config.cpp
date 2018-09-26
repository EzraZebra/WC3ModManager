#include "config.h"

using namespace std;

Config::Config()
{
    //Set Paths
        //exe path
        wchar_t exePathBuffer[MAX_PATH];
        GetModuleFileName(nullptr, exePathBuffer, MAX_PATH);
        std::wstring ws(exePathBuffer);
        std::string exePathBufferStr(ws.begin(), ws.end());
        std::string::size_type pos = exePathBufferStr.find_last_of("\\/");
        exePathBufferStr = exePathBufferStr.substr(0, pos);
        utils::valueCorrect("exePath", &exePathBufferStr);
    cfgPath = exePathBufferStr+"/config.cfg";
    modPath = exePathBufferStr+"/mods";
    outFilesPath = exePathBufferStr+"/out_files.txt";
    backupFilesPath = exePathBufferStr+"/backup_files.txt";

    //Load config file
    TxtReader txtReader(cfgPath);
    while(txtReader.next())
    {
        pair<string, string> setting = utils::line2setting(txtReader.line);
        setSetting(setting.first, setting.second);
    }

    //Load defaults
    bool configChanged = false;

    if(getSetting("GamePath") == "")
    {
        string gamePath = utils::regGet(L"GamePath", REG_SZ);
        if(gamePath == "") gamePath = utils::regGet(L"InstallPath", REG_SZ);
        if(gamePath == "") gamePath = "C:/Program Files (x86)/Warcraft III";

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

void Config::setSetting(string key, string value)
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

void Config::deleteSetting(string key)
{
    if(settings.find(key) != settings.end())
        settings.erase(key);
}

string Config::getSetting(string key)
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
