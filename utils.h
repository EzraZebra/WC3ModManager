#ifndef UTILS_H
#define UTILS_H

#include <windows.h>
#include <fstream>

namespace utils
{
    std::string int2string(int);
    void valueCorrect(std::string, std::string*);
    std::string narrow_str(const std::wstring&);

    std::string regGet(const wchar_t*, DWORD);
    bool regSet(const wchar_t*, DWORD);
    std::string regGetPF86();

    class TxtReader
    {
        std::ifstream txtReader;
        bool invalidPath = false;

        public:
            TxtReader(std::string);
            ~TxtReader();
            std::string line;
            bool next();
            std::pair<std::string, std::string> line2setting();
    };
};

#endif // UTILS_H
