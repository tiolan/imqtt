/**
 * @file IMqttClient.h
 * @author Timo Lange
 * @brief
 * @date 2020
 * @copyright Timo Lange
 */

#pragma once

#include <memory>
#include <string>

#include "IMqttClientCallbacks.h"

namespace mqttclient {
class IMqttClient : public IMqttClientCallbacks {
protected:
    MqttClientCallbacks cbs;
    IMqttClient(void)
      : cbs({this, this, this})
    {
    }

    virtual void OnMqttMessage(upMqttMessage_t) const override
    {
        cbs.log->Log(IMqttLogCallbacks::LogLevel::Warning, "Got MQTT message, but no handler installed");
    }

public:
    IMqttClient(const IMqttClient&) = delete;
    IMqttClient(IMqttClient&&)      = delete;
    IMqttClient& operator=(const IMqttClient&) = delete;
    IMqttClient& operator=(IMqttClient&&) = delete;
    void*        operator new[](size_t)   = delete;

    virtual ~IMqttClient() noexcept = default;

    enum class RetCodes { OKAY, ERROR_PERMANENT, ERROR_TEMPORARY };

    virtual void
    setCallbacks(MqttClientCallbacks const& callbacks) noexcept
    {
        cbs.log = callbacks.log ? callbacks.log : this;
        cbs.msg = callbacks.msg ? callbacks.msg : this;
        cbs.con = callbacks.con ? callbacks.con : this;
    }

    /*Interface definition*/
    virtual std::string      GetLibVersion(void) const noexcept                                                  = 0;
    virtual void             ConnectAsync(void)                                                                  = 0;
    virtual void             Disconnect(void)                                                                    = 0;
    virtual RetCodes         Subscribe(std::string const& topic, IMqttMessage::QOS qos, bool getRetained = true) = 0;
    virtual RetCodes         UnSubscribe(std::string const& topic)                                               = 0;
    virtual RetCodes         Publish(mqttclient::upMqttMessage_t mqttMessage)                                    = 0;
    virtual ConnectionStatus GetConnectionStatus(void) const noexcept                                            = 0;
};

class MqttClientFactory final {
public:
    static std::unique_ptr<IMqttClient> create(std::string                host,
                                               int                        port,
                                               std::string                clientId,
                                               MqttClientCallbacks const& callbacks = MqttClientCallbacks(nullptr,
                                                                                                          nullptr,
                                                                                                          nullptr));
    MqttClientFactory() = delete;
};
}  // namespace mqttclient
