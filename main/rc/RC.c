
#include "RC.h"
#include "RC_internal.h"

//当待比较的数据相同时返回1
static unsigned char NameCmp(const char * Name1,const char *Name2)
{
    unsigned char ret=1;
    size_t i=0;
    while(1)
    {
        if(Name1[i]!=Name2[i])
            ret=0;
        if(Name1[i]=='\0' || Name2[i]=='\0')
            break;
        i++;
    }
    return ret;
}

//获取指定名称的资源的RC_Info,失败返回NULL
static const RC_Info_t * FindRCInfoByName(const char * Name)
{
    const RC_Info_t *ret=NULL;
    size_t i=0;
    for(i=0;i<RC_Info_Size;i++)
    {
        if(NameCmp(Name,(const char *)&RC_Name[RC_Info[i].name_offset]))
        {
            ret=&RC_Info[i];
            break;
        }
    }

    return ret;
}


//通过名称获取资源大小
size_t RCGetSize(const char * name)
{
    size_t ret=0;
    const RC_Info_t * info=FindRCInfoByName(name);
    if(info!=NULL)
    {
        ret=info->data_size;
    }
    return ret;
}

//通过名称获取资源指针
const unsigned char * RCGetHandle(const char * name)
{
    const unsigned char * ret=NULL;
    const RC_Info_t * info=FindRCInfoByName(name);
    if(info!=NULL)
    {
        ret=&RC_Data[info->data_offset];
    }
    return ret;
}


