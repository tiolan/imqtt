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
    struct InitializeParameters final {
        std::string         hostAddress{"localhost"};
        int                 port{1883u};
        std::string         clientId{"clientId"};
        MqttClientCallbacks callbackProvider{MqttClientCallbacks(nullptr, nullptr, nullptr)};
        std::string         mqttUsername{""};
        std::string         mqttPassword{""};
        bool                cleanSession{true};
        int                 keepAliveInterval{10 /*seconds*/};
        int                 reconnectDelayMin{1 /*seconds*/};
        int                 reconnectDelayMinLower{0 /*seconds*/};
        int                 reconnectDelayMinUpper{0 /*seconds*/};
        int                 reconnectDelayMax{30 /*seconds*/};
        bool                allowLocalTopics{false};
#ifdef USE_PAHO
        bool        autoReconnect{true}; /*true on MOSQ*/
        std::string httpProxy{""};       /*n/a on MOSQ*/
        std::string httpsProxy{""};      /*n/a on MOSQ*/
#endif
#ifdef USE_MOSQ
        bool exponentialBackoff{false}; /*true on PAHO*/
#endif
    };

    virtual void
    setCallbacks(MqttClientCallbacks const& callbacks) noexcept
    {
        cbs.log = callbacks.log ? callbacks.log : this;
        cbs.msg = callbacks.msg ? callbacks.msg : this;
        cbs.con = callbacks.con ? callbacks.con : this;
    }

    /*Interface definition*/
    virtual std::string      GetLibVersion(void) const noexcept                                          = 0;
    virtual void             ConnectAsync(void)                                                          = 0;
    virtual void             Disconnect(void)                                                            = 0;
    virtual RetCodes         SubscribeAsync(std::string const& topic,
                                            IMqttMessage::QOS  qos,
                                            int*               token       = nullptr,
                                            bool               getRetained = true)                                     = 0;
    virtual RetCodes         UnSubscribeAsync(std::string const& topic, int* token = nullptr)            = 0;
    virtual RetCodes         PublishAsync(mqttclient::upMqttMessage_t mqttMessage, int* token = nullptr) = 0;
    virtual ConnectionStatus GetConnectionStatus(void) const noexcept                                    = 0;
};

class MqttClientFactory final {
public:
    static std::unique_ptr<IMqttClient> create(IMqttClient::InitializeParameters const&);
    MqttClientFactory() = delete;
};
}  // namespace mqttclient
