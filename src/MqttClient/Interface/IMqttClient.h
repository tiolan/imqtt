/**
 * @file IMqttClient.h
 * @author Timo Lange
 * @brief Abstract interface definition for an MqttClient
 * @date 2020
 * @copyright    Copyright 2020 Timo Lange

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#pragma once

#include <memory>
#include <random>
#include <string>

#include "IMqttClientCallbacks.h"

namespace i_mqtt_client {
class IMqttClient : public IMqttClientCallbacks {
private:
    // TODO: Return bool in order to indicate accept / reject message?
    virtual void OnMqttMessage(upMqttMessage_t) const override
    {
        logCb->Log(LogLevel::WARNING, "Got MQTT message, but no handler installed");
    }

protected:
    static std::string              libVersion;
    std::default_random_engine      rndGenerator{std::random_device()()};
    IMqttLogCallbacks const*        logCb;
    IMqttCommandCallbacks const*    cmdCb;
    IMqttMessageCallbacks const*    msgCb;
    IMqttConnectionCallbacks const* conCb;
    IMqttClient(IMqttLogCallbacks const*        log,
                IMqttCommandCallbacks const*    cmd,
                IMqttMessageCallbacks const*    msg,
                IMqttConnectionCallbacks const* con);

public:
    IMqttClient(const IMqttClient&) = delete;
    IMqttClient(IMqttClient&&)      = delete;
    IMqttClient& operator=(const IMqttClient&) = delete;
    IMqttClient& operator=(IMqttClient&&) = delete;
    void*        operator new[](size_t)   = delete;

    virtual ~IMqttClient() noexcept = default;

    struct InitializeParameters final {
        std::string hostAddress{"localhost"};
        int         port{1883u};
        std::string clientId{"clientId"};
        std::string mqttUsername{""};
        std::string mqttPassword{""};
        bool        cleanSession{true};
        int         keepAliveInterval{10 /*seconds*/};
        int         reconnectDelayMin{1 /*seconds*/};
        int         reconnectDelayMinLower{0 /*seconds*/};
        int         reconnectDelayMinUpper{0 /*seconds*/};
        int         reconnectDelayMax{30 /*seconds*/};
        bool        allowLocalTopics{false};
#ifdef IMQTT_WITH_TLS
        std::string caFilePath{""};
        std::string caDirPath{""};
        std::string clientCert{""};
        std::string clientCertFilePath{""};
        std::string privateKey{""};
        std::string privateKeyFilePath{""};
        std::string privateKeyPassword{""};
#endif
#ifdef IMQTT_USE_PAHO
        bool        disableDefaultCaStore{false};
        bool        autoReconnect{true}; /*true on MOSQ*/
        std::string httpProxy{""};       /*n/a on MOSQ*/
        std::string httpsProxy{""};      /*n/a on MOSQ*/
#endif
#ifdef IMQTT_USE_MOSQ
        bool exponentialBackoff{false}; /*true on PAHO*/
#endif
    };
    /*Interface definition*/
    static ReasonCodeRepr_t      ReasonCodeToStringRepr(ReasonCode);
    static MqttReasonCodeRepr_t  MqttReasonCodeToStringRepr(MqttReasonCode);
    static MqttReasonCodeRepr_t  MqttReasonCodeToStringRepr(int);
    static Mqtt5ReasonCodeRepr_t Mqtt5ReasonCodeToStringRepr(Mqtt5ReasonCode);
    static Mqtt5ReasonCodeRepr_t Mqtt5ReasonCodeToStringRepr(int);

    template <class TPtr>
    void               SetCallbacks(TPtr const* ptr = nullptr) noexcept;
    std::string        GetLibVersion(void) const noexcept;
    virtual ReasonCode ConnectAsync(void)                                                             = 0;
    virtual ReasonCode DisconnectAsync(Mqtt5ReasonCode rc = Mqtt5ReasonCode::SUCCESS)                 = 0;
    virtual ReasonCode SubscribeAsync(std::string const& topic,
                                      IMqttMessage::QOS  qos,
                                      int*               token       = nullptr,
                                      bool               getRetained = true)                                        = 0;
    virtual ReasonCode UnSubscribeAsync(std::string const& topic, int* token = nullptr)               = 0;
    virtual ReasonCode PublishAsync(i_mqtt_client::upMqttMessage_t mqttMessage, int* token = nullptr) = 0;
    virtual bool       IsConnected(void) const noexcept                                               = 0;
};

class MqttClientFactory final {
public:
    static std::unique_ptr<IMqttClient> Create(IMqttClient::InitializeParameters const&,
                                               IMqttMessageCallbacks const*    msg,
                                               IMqttLogCallbacks const*        log = nullptr,
                                               IMqttCommandCallbacks const*    cmd = nullptr,
                                               IMqttConnectionCallbacks const* con = nullptr);
    MqttClientFactory() = delete;
};
}  // namespace i_mqtt_client
