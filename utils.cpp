#include "utils.h"
#include <sstream>
#include <QFileInfo>

namespace utils {
    std::string int2string(int i)
    {
        std::ostringstream stream;
        stream << i;
        return stream.str();
    }

    void valueCorrect(std::string field, std::string *value)
    {
        if(field == "GamePath" || field == "path") replace(value->begin(), value->end(), '\\', '/');
    }

    std::string narrow_str(const std::wstring& str)
    {
        std::ostringstream stm;
        const std::ctype<char>& ctfacet = std::use_facet< std::ctype<char> >(stm.getloc());
        for(size_t i=0; i<str.size(); ++i) stm << ctfacet.narrow(char(str[i]), 0);
        return stm.str();
    }

    bool regOpenKey(REGSAM samDesired, HKEY* hKey)
    {
        if(RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Blizzard Entertainment\\Warcraft III", 0, samDesired, hKey) == ERROR_SUCCESS)
            return true;
        else return false;
    }

    std::string regGet(const wchar_t* key, DWORD type)
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
                    sResult = narrow_str(result);
            }
            else if(type == REG_DWORD)
            {
                DWORD result;
                if(RegQueryValueEx(hKey, key, nullptr, &type, LPBYTE(&result), &size) == ERROR_SUCCESS)
                    sResult = int2string(static_cast<int>(result));
            }
        }

        RegCloseKey(hKey);
        valueCorrect(narrow_str(key), &sResult);

        return sResult;
    }

    bool regSet(const wchar_t* key, DWORD value)
    {
        HKEY hKey;
        bool success = false;

        if(regOpenKey(KEY_ALL_ACCESS, &hKey))
            success = (RegSetValueEx(hKey, key, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&value), sizeof(value)) == ERROR_SUCCESS);

        RegCloseKey(hKey);

        return success;
    }

    std::string regGetPF86()
    {
        HKEY hKey;
        std::string sResult = "";
        if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Winwdows\\CurrentVersion", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            wchar_t result[MAX_PATH];
            DWORD size = 1024, type = REG_SZ;
            if(RegQueryValueEx(hKey, L"ProgramFilesDir (x86)", nullptr, &type, LPBYTE(&result), &size) == ERROR_SUCCESS)
                sResult = narrow_str(result);
        }

        RegCloseKey(hKey);
        valueCorrect("path", &sResult);

        return sResult;
    }

    TxtReader::TxtReader(std::string path)
    {
        QFileInfo fiPath(QString::fromStdString(path));

        if(fiPath.exists() && fiPath.isFile())
        {
            txtReader.open(path.c_str());
        }
        else invalidPath = true;
    }

    bool TxtReader::next()
    {
        if(invalidPath || !txtReader)
        {
            if(!invalidPath) txtReader.close();
            return false;
        }
        else
        {
            getline(txtReader, line);
            return true;
        }
    }

    std::pair<std::string, std::string> TxtReader::line2setting()
    {
        size_t pos = line.find_first_of('=');

        std::string field = line.substr(0, pos),
                    value = line.substr(pos+1);

        return { field, value };
    }

    TxtReader::~TxtReader()
    {
        if(txtReader.is_open()) txtReader.close();
    }
}
