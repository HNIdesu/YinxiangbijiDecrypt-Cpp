#ifndef __UTILITY_H__
#define __UTILITY_H__
#include "common.hpp"
#include <cstring>

class memory_stream{
private:
    Byte* inner_buffer;
    bool leave_open;
    int r_position;
    int size;
public:
    memory_stream(Byte* buffer,uint32_t size,bool leave_open){
        inner_buffer=buffer;
        r_position=0;
        this->size=size;
        this->leave_open=leave_open;
    }
    int rtell() const{
        return r_position;
    }
    ~memory_stream(){
        if(!leave_open)
            delete[] inner_buffer;
    }

    int rseek(int offset,int where);
    const Byte* read(uint32_t size,uint32_t& bytes_read);
    int get_size() const{
        return size;
    }
    Byte* get_buffer() const{
        return inner_buffer;
    }
};
void print_hex(const char* label,const Byte* arr,uint32_t length);
#endif