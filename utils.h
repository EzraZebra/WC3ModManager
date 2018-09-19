#ifndef UTILS_H
#define UTILS_H

#include <windows.h>
#include <fstream>

class Utils
{
    bool regOpenKey(REGSAM, HKEY*);
    std::ifstream txtReader;
    std::ofstream error_log;

public:
    Utils();
    std::string narrow(const std::wstring&);
    std::string int2string(int);
    std::pair<std::string, std::string> line2setting(std::string);
    void valueCorrect(std::string, std::string*);

    bool txtReaderStart(std::string);
    bool txtReaderNext();
    std::string txtReaderLine;

    std::string regGet(const wchar_t*, DWORD);
    bool regSet(const wchar_t*, DWORD);

    void error(std::string, std::string);

    std::string exePath;
};

#endif // UTILS_H
