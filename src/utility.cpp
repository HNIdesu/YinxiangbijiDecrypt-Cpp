#include <iomanip>
#include <iostream>
#include "utility.hpp"
using namespace std;
const Byte* memory_stream::read(uint32_t size,uint32_t& bytes_read){
    if(r_position==this->size){
        bytes_read=0;
        return nullptr;
    }
    auto const start=r_position;
    auto const remaining=this->size-r_position;
    if(remaining>size){
        r_position+=size;
        bytes_read = size;
    }else{
        bytes_read= remaining;
        r_position=this->size;
    }
    return inner_buffer+start;
}

int memory_stream::rseek(int offset,int where){
    int new_pos=0;
    switch (where)
    {
    case 0:
        new_pos=offset;
        if(new_pos>size || new_pos<0)return -1;
        r_position=new_pos;
        break;
    case 1:
        new_pos=r_position+offset;
        if(new_pos>size || new_pos<0)return -1;
        r_position=new_pos;
        break;
    case 2:
        new_pos=size+offset;
        if(new_pos>size || new_pos<0)return -1;
        r_position=new_pos;
        break;
    default:
        new_pos= -1;
        break;
    }
    return new_pos;
}

void print_hex(const char* label,const Byte* arr,uint32_t length)
{
    cout<<label<<":\n";
    for(uint32_t i=0;i<length;i++)
        cout<<setw(2)<<setfill('0')<<hex<<(int)arr[i]<<" ";
    cout<<endl;
}
