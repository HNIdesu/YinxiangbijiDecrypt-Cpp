#ifndef __LOGGER_H__
#define __LOGGER_H__
#include <initializer_list>
#include <stdexcept>
namespace Logger{
    enum LogLevel{
        Silent  =       0,
        Log     =       1<<0,
        Debug   =       1<<1,
        Warn    =       1<<2,
        Error   =       1<<3,
        Verbose =       0x1111
    };
    void set_log_level(LogLevel level);
    
    void log(const char *tag, const char* diveder,const char* end,std::initializer_list<const char*> msg);
    void debug(const char *tag, const char* diveder,const char* end,std::initializer_list<const char*> msg);
    void warn(const char *tag, const char* diveder,const char* end,std::initializer_list<const char*> msg);
    void error(const char *tag, const char* diveder,const char* end,std::initializer_list<const char*> msg,std::exception* ex=nullptr);
}
#endif