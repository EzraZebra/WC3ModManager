#ifndef UTILS_H
#define UTILS_H

#include <windows.h>
#include <fstream>
#include <sstream>
#include <ctime>
#include <iostream>
#include <ostream>
#include <QFileInfo>

namespace utils
{
    std::string str_narrow(const std::wstring&);

    std::string int2string(int);

    void valueCorrect(std::string, std::string *);

    std::pair<std::string, std::string> line2setting(std::string);

    bool regOpenKey(REGSAM, HKEY*);

    std::string regGet(const wchar_t*, DWORD);

    bool regSet(const wchar_t*, DWORD);

    class TxtReader
    {
        std::ifstream txtReader;

        public:
            TxtReader(std::string);
            std::string line;
            bool next();
    };
};

#endif // UTILS_H
