#include "utils.h"
#include <sstream>
#include <ctime>
#include <iostream>
#include <ostream>
#include <QFileInfo>

std::string utils::str_narrow(const std::wstring& str)
{
    std::ostringstream stm;
    const std::ctype<char>& ctfacet = std::use_facet< std::ctype<char> >(stm.getloc());
    for(size_t i=0; i<str.size(); ++i) stm << ctfacet.narrow(char(str[i]), 0);
    return stm.str();
}

std::string utils::int2string(int i)
{
    std::ostringstream stream;
    stream << i;
    return stream.str();
}

void utils::valueCorrect(std::string field, std::string *value)
{
    if(field == "GamePath" || field == "exePath") replace(value->begin(), value->end(), '\\', '/');
}

std::pair<std::string, std::string> utils::line2setting(std::string line)
{
    size_t pos = line.find_first_of('=');

    std::string field = line.substr(0, pos),
                value = line.substr(pos+1);
    valueCorrect(field, &value);

    return { field, value };
}

bool utils::regOpenKey(REGSAM samDesired, HKEY* hKey)
{
    if(RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Blizzard Entertainment\\Warcraft III", 0, samDesired, hKey) == ERROR_SUCCESS)
        return true;
    else return false;
}

std::string utils::regGet(const wchar_t* key, DWORD type)
{
    HKEY hKey;
    std::string sResult = "";
    if(regOpenKey(KEY_READ, &hKey))
    {
        DWORD size=1024;

        if(type == REG_SZ)
        {
            wchar_t result[MAX_PATH];
            if(RegQueryValueEx(hKey, key, nullptr, &type, LPBYTE(&result), &size) == ERROR_SUCCESS)
                sResult = str_narrow(result);
        }
        else if(type == REG_DWORD)
        {
            DWORD result;
            if(RegQueryValueEx(hKey, key, nullptr, &type, LPBYTE(&result), &size) == ERROR_SUCCESS)
                sResult = int2string(static_cast<int>(result));
        }
    }

    RegCloseKey(hKey);
    valueCorrect(str_narrow(key), &sResult);

    return sResult;
}

bool utils::regSet(const wchar_t* key, DWORD value)
{
    HKEY hKey;
    bool success = false;

    if(regOpenKey(KEY_ALL_ACCESS, &hKey))
        success = (RegSetValueEx(hKey, key, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&value), sizeof(value)) == ERROR_SUCCESS);

    RegCloseKey(hKey);

    return success;
}



TxtReader::TxtReader(std::string path)
{
    QFileInfo fiPath(QString::fromStdString(path));

    if(fiPath.exists() && fiPath.isFile())
    {
        txtReader.open(path.c_str());
    }
}

bool TxtReader::next()
{
    if(!txtReader)
    {
        txtReader.close();
        return false;
    }
    else
    {
        getline(txtReader, line);
        return true;
    }
}
