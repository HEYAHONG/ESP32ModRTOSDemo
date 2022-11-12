#ifndef INIT_H_INCLUDED
#define INIT_H_INCLUDED

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#include "cJSON.h"

/*
此文件提供基础功能的初始化以及相关API
*/

//初始化
void system_init();

/*
全局配置文件相关,只能保存少量数据，更多数据需要单独保存文件
*/

//保存配置
void system_config_save();

//添加配置item,添加后若原item不使用则需要用cJSON_Delete()释放原item。
void system_config_put_item(cJSON *item, const char *name);

//获取配置item,使用完成后，需要使用cJSON_Delete()释放指针
cJSON *system_config_get_item(const char *name);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // INIT_H_INCLUDED
