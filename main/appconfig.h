#ifndef APPCONFIG_H_INCLUDED
#define APPCONFIG_H_INCLUDED

#include "sdkconfig.h"

/*
APPCONFIG ���ڶ���һЩͨ�õĺ궨��
*/

//����FreeRTOSͷ�ļ�
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

//����Ĭ��ȫ��json�����ļ�
#define DEFAULT_GOLBAL_CONFIG_JSON "{}"


#endif // APPCONFIG_H_INCLUDED
