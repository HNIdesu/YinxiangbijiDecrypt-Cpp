#include "logger.hpp"
#include "chrono"
#include <iostream>

using namespace std;

namespace Logger{
    static LogLevel logLevel=(LogLevel)(LogLevel::Log|LogLevel::Error|LogLevel::Warn);
    static chrono::system_clock clock;

    void set_log_level(LogLevel level)
    {
        logLevel=level;
    }

    static string get_timestamp()
    {
        auto now = chrono::system_clock::now();
        time_t now_time = chrono::system_clock::to_time_t(now);
        tm* local_time = localtime(&now_time);
        char buffer[20];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", local_time);
        return string(buffer);
    }

    void log(const char *tag, const char* diveder,const char* end,initializer_list<const char*> msg)
    {
        if(logLevel&LogLevel::Log){
            cout << "[" << get_timestamp() << "] ";
            cout << "[Log] ";
            cout << "[" << tag << "] ";
            for(auto const arg:msg)
                cout << arg << diveder;
            cout<<end;
            cout.flush();
        }
    }

    

    void debug(const char *tag, const char* diveder,const char* end,initializer_list<const char*> msg)
    {
        if(logLevel&LogLevel::Debug){
            cout << "[" << get_timestamp() << "] ";
            cout << "[Debug] ";
            cout << "[" << tag << "] ";
            for(auto const arg:msg)
                cout << arg << diveder;
            cout<<end;
            cout.flush();
        }
    }

    void warn(const char *tag, const char *diveder, const char *end, std::initializer_list<const char *> msg)
    {
        if(logLevel&LogLevel::Warn){
            cout << "[" << get_timestamp() << "] ";
            cout << "[Warn] ";
            cout << "[" << tag << "] ";
            for(auto const arg:msg)
                cout << arg << diveder;
            cout<<end;
            cout.flush();
        }
    }
    void error(const char *tag, const char *diveder, const char *end, std::initializer_list<const char *> msg,exception* ex)
    {
        if(logLevel&LogLevel::Error){
            cout << "[" << get_timestamp() << "] ";
            cout << "[Error] ";
            cout << "[" << tag << "] ";
            for(auto const arg:msg)
                cout << arg << diveder;
            if(ex)
                cout<<"\nException:"<<ex->what();
            cout<<end;
            cout.flush();
        }
    }
}
