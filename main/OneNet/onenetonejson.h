#ifndef ONENETONEJSON_H
#define ONENETONEJSON_H
#ifdef __cplusplus
#include <json/value.h>
#include <json/reader.h>
#include <json/writer.h>
#include <string>
#include <functional>
#include <vector>
#include <map>
#include <time.h>

class OneNETOneJson
{
public:
    OneNETOneJson();
    virtual ~OneNETOneJson();

    typedef enum
    {
        ONENET_ONEJSON_TYPE_MQTT=1,
        ONENET_ONEJSON_TYPE_DEFAULT=ONENET_ONEJSON_TYPE_MQTT
    } OneJsonType;

    //设置产品ID和设备名称,注意：不符合产品ID与设备名称的消息将忽略
    void SetDev(std::string _ProductID,std::string _DeviceName);

    //MQTT接口(一般不由用户调用，初始化MQTT栈时使用)
    bool OnMQTTMessage(std::string topic,std::string payload);
    void SetMQTTPublish(std::function<bool(std::string,std::string)> _MQTTPublish);

    //OneJson设备属性/事件接口
    typedef struct
    {
        std::string key;
        Json::Value value;
        uint64_t timestamp;
    } PropertyPostParam;
    bool PropertyPost(std::vector<PropertyPostParam> params,std::string id,OneJsonType type=ONENET_ONEJSON_TYPE_DEFAULT);
    void SetOnPropertyPostReply(std::function<void(std::string,int,std::string)> _OnPropertyPostReply);
    void SetOnPropertySet(std::function<bool(std::string,Json::Value)> _OnPropertySet);
    void SetOnPropertyGet(std::function<bool(std::string,Json::Value&)> _OnPropertyGet);
    typedef struct
    {
        std::string identifier;
        std::vector<PropertyPostParam> value;
        uint64_t timestamp;
    } EventPostParam;
    bool EventPost(std::vector<EventPostParam> params,std::string id,OneJsonType type=ONENET_ONEJSON_TYPE_DEFAULT);
    void SetOnEventPostReply(std::function<void(std::string,int,std::string)> _OnEventPostReply);
    typedef struct
    {
        struct
        {
            std::string productid;
            std::string devicename;
        } identity;
        std::vector<PropertyPostParam> properties;
        std::vector<EventPostParam> events;
    } PackPostParam;
    bool PackPost(std::vector<PackPostParam> params,std::string id,OneJsonType type=ONENET_ONEJSON_TYPE_DEFAULT);
    void SetOnPackPostReply(std::function<void(std::string,int,std::string)> _OnPackPostReply);
    typedef struct
    {
        Json::Value value;
        uint64_t timestamp;
    } HistoryPropertyValue;
    typedef struct
    {
        std::string key;
        std::vector<HistoryPropertyValue> value;
    } HistoryProperty;
    typedef struct
    {
        std::vector<PropertyPostParam> value;
        uint64_t timestamp;
    } HistoryEventValue;
    typedef struct
    {
        std::string identifier;
        std::vector<HistoryEventValue> value;
    } HistoryEvent;
    typedef struct
    {
        struct
        {
            std::string productid;
            std::string devicename;
        } identity;
        std::vector<HistoryProperty> properties;
        std::vector<HistoryEvent> events;
    } HistoryPostParam;
    bool HistoryPost(std::vector<HistoryPostParam> params,std::string id,OneJsonType type=ONENET_ONEJSON_TYPE_DEFAULT);
    void SetOnHistoryPostReply(std::function<void(std::string,int,std::string)> _OnHistoryPostReply);

    //OneJson设备服务调用接口
    typedef std::map< std::string,Json::Value> ServiceParam;
    typedef std::map< std::string,Json::Value> ServiceData;
    bool ServiceReturnAsync(ServiceData data,std::string identifier,std::string id,int code,std::string msg,OneJsonType type=ONENET_ONEJSON_TYPE_DEFAULT);
    void SetOnServiceInvoke(std::function<ServiceData(ServiceParam,std::string,std::string)> _OnServiceInvoke);

    //OneJson属性期望值接口
    bool PropertyDesiredGet(std::vector<std::string> params,std::string id,OneJsonType type=ONENET_ONEJSON_TYPE_DEFAULT);
    typedef struct
    {
        Json::Value value;
        int version;
    } PropertyDesiredValue;
    void SetOnPropertyDesiredGetReply(std::function<void(std::map<std::string,PropertyDesiredValue>,std::string,int,std::string)> _OnPropertyDesiredGetReply);
    bool PropertyDesiredDelete(std::vector<std::string> params,std::string id,OneJsonType type=ONENET_ONEJSON_TYPE_DEFAULT);
    bool PropertyDesiredDelete(std::map<std::string,int> params,std::string id,OneJsonType type=ONENET_ONEJSON_TYPE_DEFAULT);
    void SetOnPropertyDesiredDeleteReply(std::function<void(std::string,int,std::string)> _OnPropertyDesiredDeleteReply);

    //OneJson管理拓扑关系接口
    bool SubTopoAdd(std::string productID,std::string deviceName,std::string accesskey,std::string id,OneJsonType type=ONENET_ONEJSON_TYPE_DEFAULT);
    void SetOnSubTopoAddReply(std::function<void(std::string,int,std::string)> _OnSubTopoAddReply);
    bool SubTopoDelete(std::string productID,std::string deviceName,std::string accesskey,std::string id,OneJsonType type=ONENET_ONEJSON_TYPE_DEFAULT);
    void SetOnSubTopoDeleteReply(std::function<void(std::string,int,std::string)> _OnSubTopoDeleteReply);
    bool SubTopoGet(std::string id,OneJsonType type=ONENET_ONEJSON_TYPE_DEFAULT);
    typedef struct
    {
        std::string productid;
        std::string devicename;
    } SubTopoDevInfo;
    void SetOnSubTopoGetReply(std::function<void(std::vector<SubTopoDevInfo>,std::string,int,std::string)> _OnSubTopoGetReply);
    void SetOnSubTopoGetResult(std::function<void(std::string,int,std::string)> _OnSubTopoGetResult);
    void SetOnSubTopoChange(std::function<void(std::vector<SubTopoDevInfo>)> _OnSubTopoChange);

    //OneJson子设备上下线接口
    bool SubLogin(std::string productID,std::string deviceName,std::string id,OneJsonType type=ONENET_ONEJSON_TYPE_DEFAULT);
    void SetOnSubLoginReply(std::function<void(std::string,int,std::string)> _OnSubLoginReply);
    bool SubLogout(std::string productID,std::string deviceName,std::string id,OneJsonType type=ONENET_ONEJSON_TYPE_DEFAULT);
    void SetOnSubLogoutReply(std::function<void(std::string,int,std::string)> _OnSubLogoutReply);

    //OneJson子设备数据交互接口
    void SetOnSubPropertyGet(std::function<bool(std::string,std::string,std::string,Json::Value&)> _OnSubPropertyGet);
    void SetOnSubPropertySet(std::function<bool(std::string,std::string,std::string,Json::Value)> _OnSubPropertySet);
    void SetOnSubServiceInvoke(std::function<ServiceData(std::string,std::string,ServiceParam,std::string,std::string)> _OnSubServiceInvoke);


private:
    //MQTT发送函数
    std::function<bool(std::string,std::string)> MQTTPublish;

    //设备信息
    std::string ProductID;
    std::string DeviceName;

    //OneJson设备属性/事件接口
    std::function<void(std::string,int,std::string)> OnPropertyPostReply;
    std::function<bool(std::string,Json::Value)> OnPropertySet;
    std::function<bool(std::string,Json::Value&)> OnPropertyGet;
    std::function<void(std::string,int,std::string)> OnEventPostReply;
    std::function<void(std::string,int,std::string)> OnPackPostReply;
    std::function<void(std::string,int,std::string)> OnHistoryPostReply;

    //OneJson设备服务调用接口
    std::function<ServiceData(ServiceParam,std::string,std::string)> OnServiceInvoke;

    //OneJson属性期望值接口
    std::function<void(std::map<std::string,PropertyDesiredValue>,std::string,int,std::string)> OnPropertyDesiredGetReply;
    std::function<void(std::string,int,std::string)> OnPropertyDesiredDeleteReply;

    //OneJson管理拓扑关系接口
    std::function<void(std::string,int,std::string)> OnSubTopoAddReply;
    std::function<void(std::string,int,std::string)> OnSubTopoDeleteReply;
    std::function<void(std::vector<SubTopoDevInfo>,std::string,int,std::string)> OnSubTopoGetReply;
    std::function<void(std::string,int,std::string)> OnSubTopoGetResult;
    std::function<void(std::vector<SubTopoDevInfo>)> OnSubTopoChange;

    //OneJson子设备上下线接口
    std::function<void(std::string,int,std::string)> OnSubLoginReply;
    std::function<void(std::string,int,std::string)> OnSubLogoutReply;

    //OneJson子设备数据交互接口
    std::function<bool(std::string,std::string,std::string,Json::Value&)> OnSubPropertyGet;
    std::function<bool(std::string,std::string,std::string,Json::Value)> OnSubPropertySet;
    std::function<ServiceData(std::string,std::string,ServiceParam,std::string,std::string)> OnSubServiceInvoke;

};


#endif // __cplusplus
#endif // ONENETONEJSON_H
