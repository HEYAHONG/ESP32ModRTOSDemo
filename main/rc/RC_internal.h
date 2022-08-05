#ifndef RC_INTERNAL_H_INCLUDED
#define RC_INTERNAL_H_INCLUDED

#include "stdint.h"
#include "stdlib.h"

//资源信息结构体
typedef struct
{
size_t data_offset;
size_t data_size;
size_t name_offset;
size_t name_size;
} RC_Info_t;

extern const unsigned char RC_Data[];
extern const unsigned char RC_Name[];
extern const RC_Info_t RC_Info[];
extern const size_t    RC_Info_Size;

#endif // RC_INTERNAL_H_INCLUDED
