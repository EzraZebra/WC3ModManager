#ifndef UTILS_H
#define UTILS_H

#include <windows.h>
#include <fstream>

class utils
{
    static bool regOpenKey(REGSAM, HKEY*);
    static std::string str_narrow(const std::wstring&);

public:
    static std::string int2string(int);
    static std::pair<std::string, std::string> line2setting(std::string);
    static void valueCorrect(std::string, std::string*);

    static std::string regGet(const wchar_t*, DWORD);
    static bool regSet(const wchar_t*, DWORD);
};

class TxtReader
{
    std::ifstream txtReader;

public:
    TxtReader(std::string);
    std::string line;
    bool next();
};

#endif // UTILS_H
