#ifndef __PATH_H__
#define __PATH_H__

#include <string>
#include <filesystem>

class Path{
private:
    std::filesystem::path path;
public:
    Path(std::wstring u16path);
    Path(std::string u8path);
    bool HasExtension(std::string u8ext) const;
    Path ReplaceExtension(std::string u8ext);
    const std::string ToUtf8String() const;
    bool IsDirectory() const;
    bool Exists() const;
    std::filesystem::directory_iterator EnumerateFiles() const;
};
#endif