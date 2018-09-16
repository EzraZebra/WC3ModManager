#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <windows.h>
#include <fstream>
#include <QtOpenGL/qgl.h>

class Utils
{
public:
    Utils();
    std::string narrow(const std::wstring&);
    std::string int2string(int);
    void valueCorrect(std::string, std::string*);
    bool txtReaderStart(std::string);
    bool txtReaderNext();
    std::pair<std::string, std::string> line2setting(std::string);
    bool regOpenKey(REGSAM, HKEY*);
    std::string regGet(const wchar_t*, DWORD);
    bool regSet(const wchar_t*, DWORD);
    void error(std::string, std::string);
    std::string exePath;
    std::ifstream txtReader;
    std::string txtReaderLine;
    std::ofstream error_log;
};

#endif // UTILS_H
