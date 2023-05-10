#include "onenetonejson.h"
#include "onenettokencpp.h"
#include <chrono>

OneNETOneJson::OneNETOneJson()
{

}

OneNETOneJson::~OneNETOneJson()
{

}

void OneNETOneJson::SetDev(std::string _ProductID,std::string _DeviceName)
{
    ProductID=_ProductID;
    DeviceName=_DeviceName;
}

bool OneNETOneJson::OnMQTTMessage(std::string topic,std::string payload)
{
    if(topic.empty() || payload.empty())
    {
        return false;
    }

    //请求的json
    Json::Value reqjson;
    {
        Json::Reader reader;
        if(!reader.parse(payload,reqjson))
        {
            //Json解析失败
            return false;
        }
    }

    auto TopicToPlies=[](std::string topic)->std::vector<std::string>
    {
        std::vector<std::string> ret;
        while(!topic.empty())
        {
            std::string::size_type pos=topic.find("/");
            if(pos!=std::string::npos)
            {
                ret.push_back(topic.substr(0,pos));
                topic=topic.substr(pos+1);
            }
            else
            {
                ret.push_back(topic);
                topic=std::string();
            }
        }

        return ret;
    };

    std::vector<std::string> plies=TopicToPlies(topic);

    if(plies.size()>4 && plies[0]=="$sys" && plies[1]==ProductID && plies[2]==DeviceName && plies[3]=="thing")
    {
        //是合法的OneNET物联网消息

        if(plies.size()>5 && plies[4]=="property")
        {
            //属性消息

            if(plies.size()==7 && plies[5]=="post" && plies[6]=="reply")
            {
                //上报属性消息平台回复
                if(reqjson["id"].isString() && reqjson["code"].isInt() && reqjson["msg"].isString())
                {
                    if(OnPropertyPostReply!=NULL)
                    {
                        OnPropertyPostReply(reqjson["id"].asString(),reqjson["code"].asInt(),reqjson["msg"].asString());
                        return true;
                    }
                }

            }

            if(plies.size()==6 && plies[5] == "set")
            {
                //设置属性消息
                if(OnPropertySet==NULL)
                {
                    //不支持设置
                    return false;
                }
                bool isallok=true;
                Json::Value paramsjson = reqjson["params"];
                if(paramsjson.isObject())
                {
                    Json::Value::Members members=paramsjson.getMemberNames();
                    for(auto member:members)
                    {
                        if(!OnPropertySet(member,paramsjson[member]))
                        {
                            isallok=false;
                        }
                    }
                }

                Json::Value replyjson;
                replyjson["id"]=reqjson["id"];
                if(isallok)
                {
                    replyjson["code"]=200;
                    replyjson["msg"]="ok";
                }
                else
                {
                    replyjson["code"]=500;
                    replyjson["msg"]="internal error";
                }

                std::string topic=std::string()+"$sys/"+ProductID+"/"+DeviceName+"/thing/property/set_reply";
                Json::FastWriter writer;
                std::string payload=writer.write(replyjson);
                if(MQTTPublish!=NULL)
                {
                    return MQTTPublish(topic,payload);
                }

            }

            if(plies.size()==6 && plies[5] == "get")
            {
                //获取属性消息
                if(OnPropertyGet==NULL)
                {
                    //不支持获取
                    return false;
                }
                Json::Value replyjson;
                replyjson["id"]=reqjson["id"];
                replyjson["code"]=200;
                replyjson["msg"]="ok";
                Json::Value paramsjson = reqjson["params"];
                if(paramsjson.isArray())
                {
                    Json::Value datajson;
                    for(auto it=paramsjson.begin(); it!=paramsjson.end(); it++)
                    {
                        Json::Value &key=(*it);
                        Json::Value value;
                        if(key.isString())
                        {
                            if(OnPropertyGet(key.asString(),value))
                            {
                                datajson[key.asString()]=value;
                            }
                        }
                    }
                    replyjson["data"]=datajson;
                }

                std::string topic=std::string()+"$sys/"+ProductID+"/"+DeviceName+"/thing/property/get_reply";
                Json::FastWriter writer;
                std::string payload=writer.write(replyjson);
                if(MQTTPublish!=NULL)
                {
                    return MQTTPublish(topic,payload);
                }
            }

            if(plies.size()==8 && plies[5]=="desired" && plies[6]=="get" && plies[7]=="reply")
            {
                //属性期望值获取返回
                if(reqjson["id"].isString() && reqjson["code"].isInt() && reqjson["msg"].isString()  && reqjson["data"].isObject())
                {
                    if(OnPropertyDesiredGetReply!=NULL)
                    {
                        std::map<std::string,PropertyDesiredValue> data;
                        {
                            Json::Value datajson=reqjson["data"];
                            if(datajson.isObject())
                            {
                                Json::Value::Members members=datajson.getMemberNames();
                                for(std::string member:members)
                                {
                                    PropertyDesiredValue desiredvalue;
                                    Json::Value valuejson=datajson[member];
                                    if(valuejson["version"].isInt())
                                    {
                                        desiredvalue.version=valuejson["version"].asInt();
                                        desiredvalue.value=valuejson["value"];
                                        data[member]=desiredvalue;
                                    }
                                }
                            }
                        }
                        OnPropertyDesiredGetReply(data,reqjson["id"].asString(),reqjson["code"].asInt(),reqjson["msg"].asString());
                        return true;
                    }
                }
            }

            if(plies.size()==8 && plies[5]=="desired" && plies[6]=="delete" && plies[7]=="reply")
            {
                // 期望值删除回复
                if(reqjson["id"].isString() && reqjson["code"].isInt() && reqjson["msg"].isString())
                {
                    if(OnPropertyDesiredDeleteReply!=NULL)
                    {
                        OnPropertyDesiredDeleteReply(reqjson["id"].asString(),reqjson["code"].asInt(),reqjson["msg"].asString());
                        return true;
                    }
                }
            }
        }

        if(plies.size()>5 && plies[4]=="event")
        {
            //事件消息
            if(plies.size()==7 && plies[5]=="post" && plies[6]=="reply")
            {
                //事件上报回复
                if(reqjson["id"].isString() && reqjson["code"].isInt() && reqjson["msg"].isString())
                {
                    if(OnEventPostReply!=NULL)
                    {
                        OnEventPostReply(reqjson["id"].asString(),reqjson["code"].asInt(),reqjson["msg"].asString());
                        return true;
                    }
                }
            }
        }

        if(plies.size()>5 && plies[4]=="pack")
        {
            //打包消息
            if(plies.size()==7 && plies[5]=="post" && plies[6]=="reply")
            {
                //打包上报回复
                if(reqjson["id"].isString() && reqjson["code"].isInt() && reqjson["msg"].isString())
                {
                    if(OnPackPostReply!=NULL)
                    {
                        OnPackPostReply(reqjson["id"].asString(),reqjson["code"].asInt(),reqjson["msg"].asString());
                        return true;
                    }
                }
            }
        }

        if(plies.size()>5 && plies[4]=="history")
        {
            //历史消息
            if(plies.size()==7 && plies[5]=="post" && plies[6]=="reply")
            {
                //历史上报回复
                if(reqjson["id"].isString() && reqjson["code"].isInt() && reqjson["msg"].isString())
                {
                    if(OnHistoryPostReply!=NULL)
                    {
                        OnHistoryPostReply(reqjson["id"].asString(),reqjson["code"].asInt(),reqjson["msg"].asString());
                        return true;
                    }
                }
            }
        }

        if(plies.size()>5 && plies[4]=="service")
        {
            //设备服务调用
            if(plies.size()==7 && plies[6]=="invoke")
            {
                std::string identifier=plies[5];
                std::string id;
                if(reqjson["id"].isString())
                {
                    id=reqjson["id"].asString();
                }
                if(OnServiceInvoke==NULL)
                {
                    //不支持服务调用
                    return false;
                }
                ServiceParam param;
                Json::Value paramsjson = reqjson["params"];
                if(paramsjson.isObject())
                {
                    Json::Value::Members members=paramsjson.getMemberNames();
                    for(std::string member:members)
                    {
                        if(member.empty())
                        {
                            continue;
                        }
                        param[member]=paramsjson[member];
                    }
                }

                ServiceData data=OnServiceInvoke(param,identifier,id);

                Json::Value replyjson;
                replyjson["id"]=reqjson["id"];
                replyjson["code"]=200;
                replyjson["msg"]="ok";
                if(!data.empty())
                {
                    Json::Value datajson;
                    for(const std::pair<std::string,Json::Value> &sdata:data)
                    {
                        datajson[sdata.first]=sdata.second;
                    }
                    replyjson["data"]=datajson;
                }
                std::string topic=std::string()+"$sys/"+ProductID+"/"+DeviceName+"/thing/service/"+identifier+"/invoke_reply";
                Json::FastWriter writer;
                std::string payload=writer.write(replyjson);
                if(MQTTPublish!=NULL)
                {
                    return MQTTPublish(topic,payload);
                }
            }
        }

        if(plies.size()>5 && plies[4]=="sub")
        {
            //子设备消息
            if(plies.size()==8 && plies[5]=="topo" && plies[6]=="add" && plies[7]=="reply")
            {
                //子设备绑定回复
                if(reqjson["id"].isString() && reqjson["code"].isInt() && reqjson["msg"].isString())
                {
                    if(OnSubTopoAddReply!=NULL)
                    {
                        OnSubTopoAddReply(reqjson["id"].asString(),reqjson["code"].asInt(),reqjson["msg"].asString());
                        return true;
                    }
                }
            }

            if(plies.size()==8 && plies[5]=="topo" && plies[6]=="delete" && plies[7]=="reply")
            {
                //子设备解除绑定回复
                if(reqjson["id"].isString() && reqjson["code"].isInt() && reqjson["msg"].isString())
                {
                    if(OnSubTopoDeleteReply!=NULL)
                    {
                        OnSubTopoDeleteReply(reqjson["id"].asString(),reqjson["code"].asInt(),reqjson["msg"].asString());
                        return true;
                    }
                }
            }

            if(plies.size()==8 && plies[5]=="topo" && plies[6]=="get" && plies[7]=="reply")
            {
                //拓扑获取回复
                if(reqjson["id"].isString() && reqjson["code"].isInt() && reqjson["msg"].isString())
                {
                    if(OnSubTopoGetReply!=NULL)
                    {
                        std::vector<SubTopoDevInfo> data;
                        {
                            Json::Value datajson=reqjson["data"];
                            if(datajson.isArray())
                            {
                                for(Json::Value::iterator it=datajson.begin(); it!=datajson.end(); it++)
                                {
                                    Json::Value itemjson=(*it);
                                    SubTopoDevInfo subdev;
                                    if(itemjson["deviceName"].isString())
                                    {
                                        subdev.devicename=itemjson["deviceName"].asString();
                                    }
                                    if(itemjson["productID"].isString())
                                    {
                                        subdev.productid=itemjson["productID"].asString();
                                    }
                                    if(!subdev.devicename.empty() && !subdev.productid.empty())
                                    {
                                        data.push_back(subdev);
                                    }
                                }
                            }
                        }
                        OnSubTopoGetReply(data,reqjson["id"].asString(),reqjson["code"].asInt(),reqjson["msg"].asString());
                        return true;
                    }
                }
            }

            if(plies.size()==8 && plies[5]=="topo" && plies[6]=="get" && plies[7]=="result")
            {
                //拓扑获取回复(同步)
                if(reqjson["id"].isString() && reqjson["code"].isInt() && reqjson["msg"].isString())
                {
                    if(OnSubTopoGetResult!=NULL)
                    {
                        OnSubTopoGetResult(reqjson["id"].asString(),reqjson["code"].asInt(),reqjson["msg"].asString());
                        return true;
                    }
                }
            }

            if(plies.size()==7 && plies[5]=="topo" && plies[6]=="change")
            {
                //拓扑变化
                if(OnSubTopoChange==NULL)
                {
                    //不支持
                    return false;
                }
                {
                    Json::Value paramsjson=reqjson["params"];
                    if(paramsjson.isObject())
                    {
                        Json::Value subListJson=paramsjson["subList"];
                        if(subListJson.isArray())
                        {
                            std::vector<SubTopoDevInfo> data;
                            for(Json::Value::iterator it=subListJson.begin(); it!=subListJson.end(); it++)
                            {
                                Json::Value itemjson=(*it);
                                SubTopoDevInfo subdev;
                                if(itemjson["deviceName"].isString())
                                {
                                    subdev.devicename=itemjson["deviceName"].asString();
                                }
                                if(itemjson["productID"].isString())
                                {
                                    subdev.productid=itemjson["productID"].asString();
                                }
                                if(!subdev.devicename.empty() && !subdev.productid.empty())
                                {
                                    data.push_back(subdev);
                                }
                            }
                            OnSubTopoChange(data);
                        }
                    }
                }
                Json::Value replyjson;
                replyjson["id"]=reqjson["id"];
                replyjson["code"]=200;
                replyjson["msg"]="ok";
                std::string topic=std::string()+"$sys/"+ProductID+"/"+DeviceName+"/thing/sub/topo/change_reply";
                Json::FastWriter writer;
                std::string payload=writer.write(replyjson);
                if(MQTTPublish!=NULL)
                {
                    return MQTTPublish(topic,payload);
                }
            }

            if(plies.size()==7 && plies[5]=="login" &&  plies[6]=="reply")
            {
                //子设备上线回复
                if(reqjson["id"].isString() && reqjson["code"].isInt() && reqjson["msg"].isString())
                {
                    if(OnSubLoginReply!=NULL)
                    {
                        OnSubLoginReply(reqjson["id"].asString(),reqjson["code"].asInt(),reqjson["msg"].asString());
                        return true;
                    }
                }
            }

            if(plies.size()==7 && plies[5]=="logout" &&  plies[6]=="reply")
            {
                //子设备下线回复
                if(reqjson["id"].isString() && reqjson["code"].isInt() && reqjson["msg"].isString())
                {
                    if(OnSubLogoutReply!=NULL)
                    {
                        OnSubLogoutReply(reqjson["id"].asString(),reqjson["code"].asInt(),reqjson["msg"].asString());
                        return true;
                    }
                }
            }

            if(plies.size()==7 && plies[5]=="property" &&  plies[6]=="get")
            {
                //子设备属性获取
                if(OnSubPropertyGet==NULL)
                {
                    return false;
                }
                Json::Value replyjson;
                replyjson["id"]=reqjson["id"];
                replyjson["code"]=200;
                replyjson["msg"]="ok";
                Json::Value paramsoutjson = reqjson["params"];
                if(paramsoutjson.isObject())
                {
                    std::string productID;
                    if(paramsoutjson["productID"].isString())
                    {
                        productID=paramsoutjson["productID"].asString();
                    }
                    std::string deviceName;
                    if(paramsoutjson["deviceName"].isString())
                    {
                        deviceName=paramsoutjson["deviceName"].asString();
                    }
                    Json::Value paramsjson= paramsoutjson["params"];
                    if(paramsjson.isArray())
                    {
                        Json::Value datajson;
                        for(auto it=paramsjson.begin(); it!=paramsjson.end(); it++)
                        {
                            Json::Value &key=(*it);
                            Json::Value value;
                            if(key.isString())
                            {
                                if(OnSubPropertyGet(productID,deviceName,key.asString(),value))
                                {
                                    datajson[key.asString()]=value;
                                }
                            }
                        }
                        replyjson["data"]=datajson;
                    }
                }

                std::string topic=std::string()+"$sys/"+ProductID+"/"+DeviceName+"/thing/sub/property/get_reply";
                Json::FastWriter writer;
                std::string payload=writer.write(replyjson);
                if(MQTTPublish!=NULL)
                {
                    return MQTTPublish(topic,payload);
                }

            }

            if(plies.size()==7 && plies[5]=="property" &&  plies[6]=="set")
            {
                //子设备属性设置
                if(OnSubPropertySet==NULL)
                {
                    return false;
                }

                bool isallok=true;
                Json::Value paramsoutjson = reqjson["params"];
                if(paramsoutjson.isObject())
                {
                    std::string productID;
                    if(paramsoutjson["productID"].isString())
                    {
                        productID=paramsoutjson["productID"].asString();
                    }
                    std::string deviceName;
                    if(paramsoutjson["deviceName"].isString())
                    {
                        deviceName=paramsoutjson["deviceName"].asString();
                    }
                    Json::Value paramsjson= paramsoutjson["params"];
                    if(paramsjson.isObject())
                    {
                        Json::Value::Members members=paramsjson.getMemberNames();
                        for(auto member:members)
                        {
                            if(!OnSubPropertySet(productID,deviceName,member,paramsjson[member]))
                            {
                                isallok=false;
                            }
                        }
                    }
                }

                Json::Value replyjson;
                replyjson["id"]=reqjson["id"];
                if(isallok)
                {
                    replyjson["code"]=200;
                    replyjson["msg"]="ok";
                }
                else
                {
                    replyjson["code"]=500;
                    replyjson["msg"]="internal error";
                }

                std::string topic=std::string()+"$sys/"+ProductID+"/"+DeviceName+"/thing/sub/property/set_reply";
                Json::FastWriter writer;
                std::string payload=writer.write(replyjson);
                if(MQTTPublish!=NULL)
                {
                    return MQTTPublish(topic,payload);
                }

            }

            if(plies.size()==7 && plies[5]=="service" &&  plies[6]=="invoke")
            {
                //子设备服务调用
                if(OnSubServiceInvoke==NULL)
                {
                    return false;
                }
                Json::Value paramsoutjson=reqjson["params"];
                std::string id;
                if(reqjson["id"].isString())
                {
                    id=reqjson["id"].asString();
                }
                if(paramsoutjson.isObject())
                {
                    std::string identifier;
                    if(paramsoutjson["identifier"].isString())
                    {
                        identifier=paramsoutjson["identifier"].asString();
                    }
                    std::string productID;
                    if(paramsoutjson["productID"].isString())
                    {
                        productID=paramsoutjson["productID"].asString();
                    }
                    std::string deviceName;
                    if(paramsoutjson["deviceName"].isString())
                    {
                        deviceName=paramsoutjson["deviceName"].asString();
                    }
                    ServiceParam param;
                    Json::Value inputjson= paramsoutjson["input"];
                    if(inputjson.isObject())
                    {
                        Json::Value::Members members=inputjson.getMemberNames();
                        for(std::string member:members)
                        {
                            if(member.empty())
                            {
                                continue;
                            }
                            param[member]=inputjson[member];
                        }
                    }

                    ServiceData output=OnSubServiceInvoke(productID,deviceName,param,identifier,id);

                    Json::Value replyjson;
                    replyjson["id"]=reqjson["id"];
                    replyjson["code"]=200;
                    replyjson["msg"]="ok";
                    {
                        Json::Value datajson;
                        datajson["deviceName"]=deviceName;
                        datajson["productID"]=productID;
                        datajson["identifier"]=identifier;
                        {
                            Json::Value outputjson;
                            for(const std::pair<std::string,Json::Value> &soutput:output)
                            {
                                outputjson[soutput.first]=soutput.second;
                            }
                            datajson["output"]=outputjson;
                        }
                        replyjson["data"]=datajson;
                    }
                    std::string topic=std::string()+"$sys/"+ProductID+"/"+DeviceName+"/thing/sub/service/invoke_reply";
                    Json::FastWriter writer;
                    std::string payload=writer.write(replyjson);
                    if(MQTTPublish!=NULL)
                    {
                        return MQTTPublish(topic,payload);
                    }
                }

            }

        }

    }

    return false;
}

void OneNETOneJson::SetMQTTPublish(std::function<bool(std::string,std::string)> _MQTTPublish)
{
    MQTTPublish=_MQTTPublish;
}

bool OneNETOneJson::PropertyPost(std::vector<PropertyPostParam> params,std::string id,OneJsonType type)
{
    if(ProductID.empty() || DeviceName.empty())
    {
        return false;
    }
    if(type==ONENET_ONEJSON_TYPE_MQTT)
    {
        Json::Value postjson;
        postjson["version"]="1.0";
        postjson["id"]=id;
        {
            Json::Value paramsjson;
            for(PropertyPostParam & param:params)
            {
                if(param.key.empty())
                {
                    //key不能为空
                    continue;
                }
                Json::Value paramjson;
                paramjson["value"]=param.value;
                paramjson["time"]=(Json::UInt64)param.timestamp;
                paramsjson[param.key]=paramjson;
            }
            postjson["params"]=paramsjson;
        }

        std::string topic=std::string()+"$sys/"+ProductID+"/"+DeviceName+"/thing/property/post";
        Json::FastWriter writer;
        std::string payload=writer.write(postjson);
        if(MQTTPublish!=NULL)
        {
            return MQTTPublish(topic,payload);
        }
    }
    return false;
}

void OneNETOneJson::SetOnPropertyPostReply(std::function<void(std::string,int,std::string)> _OnPropertyPostReply)
{
    OnPropertyPostReply=_OnPropertyPostReply;
}

void OneNETOneJson::SetOnPropertySet(std::function<bool(std::string,Json::Value)> _OnPropertySet)
{
    OnPropertySet=_OnPropertySet;
}

void OneNETOneJson::SetOnPropertyGet(std::function<bool(std::string,Json::Value&)> _OnPropertyGet)
{
    OnPropertyGet=_OnPropertyGet;
}

bool OneNETOneJson::EventPost(std::vector<EventPostParam> params,std::string id,OneJsonType type)
{
    if(ProductID.empty() || DeviceName.empty())
    {
        return false;
    }
    if(type==ONENET_ONEJSON_TYPE_MQTT)
    {
        Json::Value postjson;
        postjson["version"]="1.0";
        postjson["id"]=id;
        {
            Json::Value paramsjson;
            for(EventPostParam & param:params)
            {
                if(param.identifier.empty())
                {
                    //identifier不能为空
                    continue;
                }
                Json::Value paramjson;
                Json::Value valuejson;
                for(PropertyPostParam &property:param.value)
                {
                    if(property.key.empty())
                    {
                        //key不能为空
                        continue;
                    }
                    valuejson[property.key]=property.value;
                }
                paramjson["value"]=valuejson;
                paramjson["time"]=(Json::UInt64)param.timestamp;
                paramsjson[param.identifier]=paramjson;
            }
            postjson["params"]=paramsjson;
        }

        std::string topic=std::string()+"$sys/"+ProductID+"/"+DeviceName+"/thing/event/post";
        Json::FastWriter writer;
        std::string payload=writer.write(postjson);
        if(MQTTPublish!=NULL)
        {
            return MQTTPublish(topic,payload);
        }
    }
    return false;
}

void OneNETOneJson::SetOnEventPostReply(std::function<void(std::string,int,std::string)> _OnEventPostReply)
{
    OnEventPostReply=_OnEventPostReply;
}

bool OneNETOneJson::PackPost(std::vector<PackPostParam> params,std::string id,OneJsonType type)
{
    if(ProductID.empty() || DeviceName.empty())
    {
        return false;
    }
    if(type==ONENET_ONEJSON_TYPE_MQTT)
    {
        Json::Value postjson;
        postjson["version"]="1.0";
        postjson["id"]=id;
        {
            Json::Value paramsjson;
            for(PackPostParam & param:params)
            {
                Json::Value paramjson;
                {
                    //identity
                    Json::Value identity;
                    identity["productID"]=param.identity.productid;
                    identity["deviceName"]=param.identity.devicename;
                    paramjson["identity"]=identity;
                }

                {
                    //properties
                    Json::Value properties;
                    for(auto property:param.properties)
                    {
                        if(property.key.empty())
                        {
                            continue;
                        }
                        Json::Value value;
                        value["value"]=property.value;
                        value["time"]=(Json::UInt64)property.timestamp;
                        properties[property.key]=value;
                    }
                    paramjson["properties"]=properties;
                }
                {
                    //events
                    Json::Value events;
                    for(EventPostParam & event:param.events)
                    {
                        if(event.identifier.empty())
                        {
                            //identifier不能为空
                            continue;
                        }
                        Json::Value eventjson;
                        Json::Value eventvaluejson;
                        for(PropertyPostParam &eventproperty:event.value)
                        {
                            if(eventproperty.key.empty())
                            {
                                //key不能为空
                                continue;
                            }
                            eventvaluejson[eventproperty.key]=eventproperty.value;
                        }
                        eventjson["value"]=eventvaluejson;
                        eventjson["time"]=(Json::UInt64)event.timestamp;
                        events[event.identifier]=eventjson;
                    }
                    paramjson["events"]=events;
                }

                paramsjson.append(paramjson);
            }
            postjson["params"]=paramsjson;
        }

        std::string topic=std::string()+"$sys/"+ProductID+"/"+DeviceName+"/thing/pack/post";
        Json::FastWriter writer;
        std::string payload=writer.write(postjson);
        if(MQTTPublish!=NULL)
        {
            return MQTTPublish(topic,payload);
        }
    }
    return false;
}

void OneNETOneJson::SetOnPackPostReply(std::function<void(std::string,int,std::string)> _OnPackPostReply)
{
    OnPackPostReply=_OnPackPostReply;
}

bool OneNETOneJson::HistoryPost(std::vector<HistoryPostParam> params,std::string id,OneJsonType type)
{
    if(ProductID.empty() || DeviceName.empty())
    {
        return false;
    }
    if(type==ONENET_ONEJSON_TYPE_MQTT)
    {
        Json::Value postjson;
        postjson["version"]="1.0";
        postjson["id"]=id;
        {
            Json::Value paramsjson;
            for(HistoryPostParam & param:params)
            {
                Json::Value paramjson;
                {
                    //identity
                    Json::Value identity;
                    identity["productID"]=param.identity.productid;
                    identity["deviceName"]=param.identity.devicename;
                    paramjson["identity"]=identity;
                }

                {
                    //properties
                    Json::Value properties;
                    for(HistoryProperty& property:param.properties)
                    {
                        if(property.key.empty())
                        {
                            continue;
                        }
                        Json::Value value;
                        {
                            //历史数据
                            for(HistoryPropertyValue hisvalue:property.value)
                            {
                                Json::Value hisval;
                                hisval["value"]=hisvalue.value;
                                hisval["time"]=(Json::UInt64)hisvalue.timestamp;
                                value.append(hisval);
                            }
                        }
                        properties[property.key]=value;
                    }
                    paramjson["properties"]=properties;
                }
                {
                    //events
                    Json::Value events;
                    for(HistoryEvent & event:param.events)
                    {
                        if(event.identifier.empty())
                        {
                            //identifier不能为空
                            continue;
                        }
                        Json::Value eventjson;
                        {
                            for(HistoryEventValue &hisvalue:event.value)
                            {
                                Json::Value hiseventjson;
                                Json::Value eventvaluejson;
                                for(PropertyPostParam &eventproperty:hisvalue.value)
                                {
                                    if(eventproperty.key.empty())
                                    {
                                        //key不能为空
                                        continue;
                                    }
                                    eventvaluejson[eventproperty.key]=eventproperty.value;
                                }
                                hiseventjson["value"]=eventvaluejson;
                                hiseventjson["time"]=(Json::UInt64)hisvalue.timestamp;
                                eventjson.append(hiseventjson);
                            }
                        }
                        events[event.identifier]=eventjson;
                    }
                    paramjson["events"]=events;
                }

                paramsjson.append(paramjson);
            }
            postjson["params"]=paramsjson;
        }

        std::string topic=std::string()+"$sys/"+ProductID+"/"+DeviceName+"/thing/history/post";
        Json::FastWriter writer;
        std::string payload=writer.write(postjson);
        if(MQTTPublish!=NULL)
        {
            return MQTTPublish(topic,payload);
        }
    }
    return false;
}

void OneNETOneJson::SetOnHistoryPostReply(std::function<void(std::string,int,std::string)> _OnHistoryPostReply)
{
    OnHistoryPostReply=_OnHistoryPostReply;
}

bool OneNETOneJson::ServiceReturnAsync(ServiceData data,std::string identifier,std::string id,int code,std::string msg,OneJsonType type)
{
    if(ProductID.empty() || DeviceName.empty())
    {
        return false;
    }
    if(identifier.empty())
    {
        return false;
    }
    if(data.empty())
    {
        //必须包含数据
        return false;
    }
    if(type==ONENET_ONEJSON_TYPE_MQTT)
    {
        Json::Value postjson;
        postjson["id"]=id;
        postjson["code"]=code;
        postjson["msg"]=msg;
        Json::Value datajson;
        for(const std::pair<std::string,Json::Value> &sdata:data)
        {
            datajson[sdata.first]=sdata.second;
        }
        postjson["data"]=datajson;

        std::string topic=std::string()+"$sys/"+ProductID+"/"+DeviceName+"/thing/service/"+identifier+"/invoke_reply";
        Json::FastWriter writer;
        std::string payload=writer.write(postjson);
        if(MQTTPublish!=NULL)
        {
            return MQTTPublish(topic,payload);
        }
    }
    return false;
}

void OneNETOneJson::SetOnServiceInvoke(std::function<ServiceData(ServiceParam,std::string,std::string)> _OnServiceInvoke)
{
    OnServiceInvoke=_OnServiceInvoke;
}

bool OneNETOneJson::PropertyDesiredGet(std::vector<std::string> params,std::string id,OneJsonType type)
{
    if(ProductID.empty() || DeviceName.empty())
    {
        return false;
    }

    if(params.empty())
    {
        //不支持空参数
        return false;
    }

    if(type==ONENET_ONEJSON_TYPE_MQTT)
    {
        Json::Value postjson;
        postjson["version"]="1.0";
        postjson["id"]=id;
        {
            Json::Value paramsjson;
            for(std::string &key:params)
            {
                paramsjson.append(key);
            }
            postjson["params"]=paramsjson;
        }

        std::string topic=std::string()+"$sys/"+ProductID+"/"+DeviceName+"/thing/property/desired/get";
        Json::FastWriter writer;
        std::string payload=writer.write(postjson);
        if(MQTTPublish!=NULL)
        {
            return MQTTPublish(topic,payload);
        }
    }
    return false;
}

void OneNETOneJson::SetOnPropertyDesiredGetReply(std::function<void(std::map<std::string,PropertyDesiredValue>,std::string,int,std::string)> _OnPropertyDesiredGetReply)
{
    OnPropertyDesiredGetReply=OnPropertyDesiredGetReply;
}

bool OneNETOneJson::PropertyDesiredDelete(std::vector<std::string> params,std::string id,OneJsonType type)
{
    if(ProductID.empty() || DeviceName.empty())
    {
        return false;
    }

    if(params.empty())
    {
        //不支持空参数
        return false;
    }

    if(type==ONENET_ONEJSON_TYPE_MQTT)
    {
        Json::Value postjson;
        postjson["version"]="1.0";
        postjson["id"]=id;
        {
            Json::Value paramsjson;
            for(std::string &key:params)
            {
                paramsjson[key]=Json::Value(Json::ValueType::objectValue);
            }
            postjson["params"]=paramsjson;
        }

        std::string topic=std::string()+"$sys/"+ProductID+"/"+DeviceName+"/thing/property/desired/delete";
        Json::FastWriter writer;
        std::string payload=writer.write(postjson);
        if(MQTTPublish!=NULL)
        {
            return MQTTPublish(topic,payload);
        }
    }
    return false;
}

bool OneNETOneJson::PropertyDesiredDelete(std::map<std::string,int> params,std::string id,OneJsonType type)
{
    if(ProductID.empty() || DeviceName.empty())
    {
        return false;
    }

    if(params.empty())
    {
        //不支持空参数
        return false;
    }

    if(type==ONENET_ONEJSON_TYPE_MQTT)
    {
        Json::Value postjson;
        postjson["version"]="1.0";
        postjson["id"]=id;
        {
            Json::Value paramsjson;
            for(std::pair<std::string,int> param:params)
            {
                Json::Value valuejson;
                valuejson["version"]=param.second;
                paramsjson[param.first]=valuejson;
            }
            postjson["params"]=paramsjson;
        }

        std::string topic=std::string()+"$sys/"+ProductID+"/"+DeviceName+"/thing/property/desired/delete";
        Json::FastWriter writer;
        std::string payload=writer.write(postjson);
        if(MQTTPublish!=NULL)
        {
            return MQTTPublish(topic,payload);
        }
    }
    return false;
}

void OneNETOneJson::SetOnPropertyDesiredDeleteReply(std::function<void(std::string,int,std::string)> _OnPropertyDesiredDeleteReply)
{
    OnPropertyDesiredDeleteReply=_OnPropertyDesiredDeleteReply;
}

bool OneNETOneJson::SubTopoAdd(std::string productID,std::string deviceName,std::string accesskey,std::string id,OneJsonType type)
{
    if(ProductID.empty() || DeviceName.empty())
    {
        return false;
    }

    if(productID.empty() || deviceName.empty() || accesskey.empty())
    {
        //不支持空参数
        return false;
    }

    if(type==ONENET_ONEJSON_TYPE_MQTT)
    {
        Json::Value postjson;
        postjson["version"]="1.0";
        postjson["id"]=id;
        {
            Json::Value paramsjson;
            paramsjson["productID"]=productID;
            paramsjson["deviceName"]=deviceName;
            {
                std::string res=OneNETTokenGetDeviceRes(productID,deviceName);
                std::string sign=OneNETTokenGetSign(0x7FFFFFFF,accesskey,ONENETCPP_DEFAULT_METHOD,ONENETCPP_DEFAULT_VERSION);
                //计算token
                std::string token=OneNETTokenGenerateURLToken(0x7FFFFFFF,res,sign,ONENETCPP_DEFAULT_METHOD,ONENETCPP_DEFAULT_VERSION);
                paramsjson["sasToken"]=token;
            }
            postjson["params"]=paramsjson;
        }

        std::string topic=std::string()+"$sys/"+ProductID+"/"+DeviceName+"/thing/sub/topo/add";
        Json::FastWriter writer;
        std::string payload=writer.write(postjson);
        if(MQTTPublish!=NULL)
        {
            return MQTTPublish(topic,payload);
        }
    }
    return false;
}

void OneNETOneJson::SetOnSubTopoAddReply(std::function<void(std::string,int,std::string)> _OnSubTopoAddReply)
{
    OnSubTopoAddReply=_OnSubTopoAddReply;
}

bool OneNETOneJson::SubTopoDelete(std::string productID,std::string deviceName,std::string accesskey,std::string id,OneJsonType type)
{
    if(ProductID.empty() || DeviceName.empty())
    {
        return false;
    }

    if(productID.empty() || deviceName.empty() || accesskey.empty())
    {
        //不支持空参数
        return false;
    }

    if(type==ONENET_ONEJSON_TYPE_MQTT)
    {
        Json::Value postjson;
        postjson["version"]="1.0";
        postjson["id"]=id;
        {
            Json::Value paramsjson;
            paramsjson["productID"]=productID;
            paramsjson["deviceName"]=deviceName;
            {
                std::string res=OneNETTokenGetDeviceRes(productID,deviceName);
                std::string sign=OneNETTokenGetSign(0x7FFFFFFF,accesskey,ONENETCPP_DEFAULT_METHOD,ONENETCPP_DEFAULT_VERSION);
                //计算token
                std::string token=OneNETTokenGenerateURLToken(0x7FFFFFFF,res,sign,ONENETCPP_DEFAULT_METHOD,ONENETCPP_DEFAULT_VERSION);
                paramsjson["sasToken"]=token;
            }
            postjson["params"]=paramsjson;
        }

        std::string topic=std::string()+"$sys/"+ProductID+"/"+DeviceName+"/thing/sub/topo/delete";
        Json::FastWriter writer;
        std::string payload=writer.write(postjson);
        if(MQTTPublish!=NULL)
        {
            return MQTTPublish(topic,payload);
        }
    }
    return false;
}

void OneNETOneJson::SetOnSubTopoDeleteReply(std::function<void(std::string,int,std::string)> _OnSubTopoDeleteReply)
{
    OnSubTopoDeleteReply=_OnSubTopoDeleteReply;
}

bool OneNETOneJson::SubTopoGet(std::string id,OneJsonType type)
{
    if(ProductID.empty() || DeviceName.empty())
    {
        return false;
    }

    if(type==ONENET_ONEJSON_TYPE_MQTT)
    {
        Json::Value postjson;
        postjson["version"]="1.0";
        postjson["id"]=id;

        std::string topic=std::string()+"$sys/"+ProductID+"/"+DeviceName+"/thing/sub/topo/get";
        Json::FastWriter writer;
        std::string payload=writer.write(postjson);
        if(MQTTPublish!=NULL)
        {
            return MQTTPublish(topic,payload);
        }
    }
    return false;
}

void OneNETOneJson::SetOnSubTopoGetReply(std::function<void(std::vector<SubTopoDevInfo>,std::string,int,std::string)> _OnSubTopoGetReply)
{
    OnSubTopoGetReply=_OnSubTopoGetReply;
}

void OneNETOneJson::SetOnSubTopoGetResult(std::function<void(std::string,int,std::string)> _OnSubTopoGetResult)
{
    OnSubTopoGetResult=_OnSubTopoGetResult;
}

void OneNETOneJson::SetOnSubTopoChange(std::function<void(std::vector<SubTopoDevInfo>)> _OnSubTopoChange)
{
    OnSubTopoChange=_OnSubTopoChange;
}

bool OneNETOneJson::SubLogin(std::string productID,std::string deviceName,std::string id,OneJsonType type)
{
    if(ProductID.empty() || DeviceName.empty())
    {
        return false;
    }

    if(productID.empty() || deviceName.empty())
    {
        //不支持空参数
        return false;
    }

    if(type==ONENET_ONEJSON_TYPE_MQTT)
    {
        Json::Value postjson;
        postjson["version"]="1.0";
        postjson["id"]=id;
        {
            Json::Value paramsjson;
            paramsjson["productID"]=productID;
            paramsjson["deviceName"]=deviceName;
            postjson["params"]=paramsjson;
        }

        std::string topic=std::string()+"$sys/"+ProductID+"/"+DeviceName+"/thing/sub/login";
        Json::FastWriter writer;
        std::string payload=writer.write(postjson);
        if(MQTTPublish!=NULL)
        {
            return MQTTPublish(topic,payload);
        }
    }
    return false;
}

void OneNETOneJson::SetOnSubLoginReply(std::function<void(std::string,int,std::string)> _OnSubLoginReply)
{
    OnSubLoginReply=_OnSubLoginReply;
}

bool OneNETOneJson::SubLogout(std::string productID,std::string deviceName,std::string id,OneJsonType type)
{
    if(ProductID.empty() || DeviceName.empty())
    {
        return false;
    }

    if(productID.empty() || deviceName.empty())
    {
        //不支持空参数
        return false;
    }

    if(type==ONENET_ONEJSON_TYPE_MQTT)
    {
        Json::Value postjson;
        postjson["version"]="1.0";
        postjson["id"]=id;
        {
            Json::Value paramsjson;
            paramsjson["productID"]=productID;
            paramsjson["deviceName"]=deviceName;
            postjson["params"]=paramsjson;
        }

        std::string topic=std::string()+"$sys/"+ProductID+"/"+DeviceName+"/thing/sub/logout";
        Json::FastWriter writer;
        std::string payload=writer.write(postjson);
        if(MQTTPublish!=NULL)
        {
            return MQTTPublish(topic,payload);
        }
    }
    return false;
}

void OneNETOneJson::SetOnSubLogoutReply(std::function<void(std::string,int,std::string)> _OnSubLogoutReply)
{
    OnSubLogoutReply=_OnSubLogoutReply;
}

void OneNETOneJson::SetOnSubPropertyGet(std::function<bool(std::string,std::string,std::string,Json::Value&)> _OnSubPropertyGet)
{
    OnSubPropertyGet=_OnSubPropertyGet;
}

void OneNETOneJson::SetOnSubPropertySet(std::function<bool(std::string,std::string,std::string,Json::Value)> _OnSubPropertySet)
{
    OnSubPropertySet=_OnSubPropertySet;
}

void OneNETOneJson::SetOnSubServiceInvoke(std::function<ServiceData(std::string,std::string,ServiceParam,std::string,std::string)> _OnSubServiceInvoke)
{
    OnSubServiceInvoke=_OnSubServiceInvoke;
}
