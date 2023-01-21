
/*
 * This file was automatically generated by sdbus-c++-xml2cpp; DO NOT EDIT!
 */

#ifndef __sdbuscpp__perftests_adaptor_h__adaptor__H__
#define __sdbuscpp__perftests_adaptor_h__adaptor__H__

#include <sdbus-c++/sdbus-c++.h>
#include <string>
#include <tuple>

namespace org {
namespace sdbuscpp {

class perftests_adaptor
{
public:
    static constexpr const char* INTERFACE_NAME = "org.sdbuscpp.perftests";

protected:
    perftests_adaptor(sdbus::IObject& object)
        : object_(&object)
    {
        object_->registerMethod("sendDataSignals").onInterface(INTERFACE_NAME).withInputParamNames("numberOfSignals", "signalMsgSize").implementedAs([this](const uint32_t& numberOfSignals, const uint32_t& signalMsgSize){ return this->sendDataSignals(numberOfSignals, signalMsgSize); });
        object_->registerMethod("concatenateTwoStrings").onInterface(INTERFACE_NAME).withInputParamNames("string1", "string2").withOutputParamNames("result").implementedAs([this](const std::string& string1, const std::string& string2){ return this->concatenateTwoStrings(string1, string2); });
        object_->registerSignal("dataSignal").onInterface(INTERFACE_NAME).withParameters<std::string>("data");
    }

    perftests_adaptor(const perftests_adaptor&) = delete;
    perftests_adaptor& operator=(const perftests_adaptor&) = delete;
    perftests_adaptor(perftests_adaptor&&) = default;
    perftests_adaptor& operator=(perftests_adaptor&&) = default;

    ~perftests_adaptor() = default;

public:
    void emitDataSignal(const std::string& data)
    {
        object_->emitSignal("dataSignal").onInterface(INTERFACE_NAME).withArguments(data);
    }

private:
    virtual void sendDataSignals(const uint32_t& numberOfSignals, const uint32_t& signalMsgSize) = 0;
    virtual std::string concatenateTwoStrings(const std::string& string1, const std::string& string2) = 0;

private:
    sdbus::IObject* object_;
};

}} // namespaces

#endif
