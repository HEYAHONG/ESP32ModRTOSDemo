#ifndef APPCONFIG_H_INCLUDED
#define APPCONFIG_H_INCLUDED

#include "sdkconfig.h"

/*
APPCONFIG 用于定义一些通用的宏定义
*/

//常用FreeRTOS头文件
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

//定义默认全局json配置文件
#define DEFAULT_GOLBAL_CONFIG_JSON "{}"


#endif // APPCONFIG_H_INCLUDED
