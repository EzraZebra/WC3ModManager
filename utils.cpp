#include "utils.h"
#include <sstream>
#include <ctime>
#include <iostream>
#include <ostream>
#include <QFileInfo>

using namespace std;

Utils::Utils()
{
    wchar_t exePathBuffer[MAX_PATH];
    GetModuleFileName(NULL, exePathBuffer, MAX_PATH);
    wstring ws(exePathBuffer);
    string exePathBufferStr(ws.begin(), ws.end());
    string::size_type pos = exePathBufferStr.find_last_of("\\/");
    exePathBufferStr = exePathBufferStr.substr(0, pos);
    valueCorrect("exePath", &exePathBufferStr);
    exePath = exePathBufferStr;
}

string Utils::narrow(const wstring& str)
{
    ostringstream stm ;
    const ctype<char>& ctfacet = use_facet< ctype<char> >(stm.getloc());
    for(size_t i=0 ; i<str.size() ; ++i) stm << ctfacet.narrow(str[i], 0);
    return stm.str();
}

string Utils::int2string(int i)
{
    ostringstream stream;
    stream << i;
    return stream.str();
}

void Utils::valueCorrect(string field, string* value)
{
    if(field == "GamePath" || field == "exePath") replace(value->begin(), value->end(), '\\', '/');
}

bool Utils::txtReaderStart(string path)
{
    QFileInfo qfTxtFile(QString::fromStdString(path));

    if(qfTxtFile.exists() && qfTxtFile.isFile())
    {
        if(txtReader.is_open()) txtReader.close();
        txtReader.open(path.c_str());
        return true;
    }
    else return false;
}

bool Utils::txtReaderNext()
{
    if(!txtReader)
    {
        txtReader.close();
        return false;
    }
    else
    {
        char line[255];
        txtReader.getline(line, 255);
        txtReaderLine = string(line);
        return true;
    }
}

pair<string, string> Utils::line2setting(string line)
{
    size_t pos = line.find_first_of('=');

    string  field = line.substr(0, pos),
            value = line.substr(pos+1);
    valueCorrect(field, &value);

    return { field, value };
}

bool Utils::regOpenKey(REGSAM samDesired, HKEY* hKey)
{
    if(RegOpenKeyEx(HKEY_CURRENT_USER,L"Software\\Blizzard Entertainment\\Warcraft III",0,samDesired,hKey) == ERROR_SUCCESS)
        return true;
    else
    {
        error("Utils::regOpenKey", "Failed to open registry key.");
        return false;
    }
}

string Utils::regGet(const wchar_t* key, DWORD type)
{
    HKEY hKey;
    string sResult = "";
    if(regOpenKey(KEY_READ, &hKey))
    {
        DWORD size=1024;

        if(type == REG_SZ)
        {
            wchar_t result[MAX_PATH];
            if(RegQueryValueEx(hKey,key,NULL,&type,(LPBYTE)&result,&size) == ERROR_SUCCESS)
                sResult = narrow(result);
            else error("Utils::regGet", "Failed to read registry value.");
        }
        else if(type == REG_DWORD)
        {
            DWORD result;
            if(RegQueryValueEx(hKey,key,NULL,&type,(LPBYTE)&result,&size) == ERROR_SUCCESS)
                sResult = int2string(result);
            else error("Utils::regGet", "Failed to read registry value.");
        }
        else error("Utils::regGet", "Invalid type");
    }

    RegCloseKey(hKey);
    valueCorrect(narrow(key), &sResult);

    return sResult;
}

bool Utils::regSet(const wchar_t* key, DWORD value)
{
    HKEY hKey;
    bool success = false;
    if(regOpenKey(KEY_ALL_ACCESS, &hKey))
        success = (RegSetValueEx(hKey, key, 0, REG_DWORD, (const BYTE*)&value, sizeof(value)) == ERROR_SUCCESS);
    if(!success) error("Utils::regSet()", "Failed to set registry value.");
    RegCloseKey(hKey);
    return success;
}

void Utils::error(string info, string msg)
{
    static char timestamp[20];
    time_t now = time(0);
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", localtime(&now));

    string timestampStr(timestamp);

    if(!error_log.is_open()) error_log.open((exePath+"/error_log_"+timestampStr+".txt").c_str());

    const char* output = ("["+timestampStr+"] "+info+" - "+msg).c_str();
    error_log << output << endl;
    cout << output << endl;
}
