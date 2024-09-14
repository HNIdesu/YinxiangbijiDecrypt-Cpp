#ifdef _WIN32
#include <windows.h>
#endif
#include "Path.hpp"
namespace fs=std::filesystem;
using namespace std;


Path::Path(wstring u16path)
{
#ifdef _WIN32
    path=u16path;
#else
    wstring_convert<codecvt_utf8_utf16<wchar_t>,wchar_t> convert;
    path=convert.to_bytes(u16path);
#endif
}

Path::Path(string u8path)
{
#ifdef _WIN32
    wstring_convert<codecvt_utf8_utf16<wchar_t>,wchar_t> convert;
    path=convert.from_bytes(u8path);
#else
    path=u8path;
#endif
    
}

bool Path::HasExtension(string u8ext) const
{
#ifdef _WIN32
    wstring_convert<codecvt_utf8_utf16<wchar_t>,wchar_t> convert;
    return convert.to_bytes(path.extension()) == u8ext;
#else
    return path.extension().string()==u8ext;
#endif
}

Path Path::ReplaceExtension(string u8ext)
{
#ifdef _WIN32
    wstring_convert<codecvt_utf8_utf16<wchar_t>,wchar_t> convert;
    path.replace_extension(convert.from_bytes(u8ext));
#else
    path.replace_extension(u8ext);
#endif
    return *this;
}
const std::string Path::ToUtf8String() const
{
#ifdef _WIN32
    wstring_convert<codecvt_utf8_utf16<wchar_t>,wchar_t> convert;
    return convert.to_bytes(path.wstring());
#else
    return path.string();
#endif
}

bool Path::IsDirectory() const
{
    return fs::is_directory(path);
}

bool Path::Exists() const
{
    return fs::exists(path);
}

std::filesystem::directory_iterator Path::EnumerateFiles() const
{
    return std::filesystem::directory_iterator(path);
}
