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
/**
 * @brief Describes the abstract interface to be used in order to use the IMqttClient implementation. It hides the
 * underlying MQTT library.
 *
 */
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

    /**
     * @brief Structure of (connection-) parameters handed over to IMqttClient at object instantiation.
     *
     */
    struct InitializeParameters final {
        std::string hostAddress{"localhost"}; /*!< the broker's address */
        int         port{1883u};              /*!< the broker's port */
        std::string clientId{"clientId"};     /*!< the MQTT client ID used to connect to the broker */
        std::string mqttUsername{""};   /*!< username for MQTT auth against the broker, not used if empty string */
        std::string mqttPassword{""};   /*!< passwortd for MQTT auth against the broker */
        bool        cleanSession{true}; /*!< set the clean session flag, when connecting to the broker */
        int         keepAliveInterval{10 /*seconds*/}; /*!< ping repetition intervall in seconds */
        int reconnectDelayMin{1 /*seconds*/}; /*!< minimum timeout that is waited, before a reconnect-attempt is done */
        int reconnectDelayMinLower{0 /*seconds*/}; /*!< when set > 0, a random amount of reconnectDelayMinLower <= time
                                                      <= reconnectDelayMinUpper is added to reconnectDelayMin */
        int reconnectDelayMinUpper{0 /*seconds*/}; /*!< when set > 0, a random amount of reconnectDelayMinLower <= time
                                                     <= reconnectDelayMinUpper is added to reconnectDelayMin */
        int reconnectDelayMax{30 /*seconds*/}; /*!< maximum timeout that is waited, before a reconnect-attempt is done
                                                  (in case expinential backoff is enabled)*/
        bool allowLocalTopics{false}; /*!< when enabled, the client may receive its own messages, when subscribed to the
                                         topic published to */
#ifdef IMQTT_WITH_TLS
        std::string caFilePath{""};         /*!< path to a file containing a CA certificate */
        std::string caDirPath{""};          /*!< path to a directory containing CA certificates */
        std::string clientCertFilePath{""}; /*!< path to a file containing the client certificate */
        std::string privateKeyFilePath{""}; /*!< path to a file containting the clients private key */
        std::string privateKeyPassword{
            ""}; /*!< password to encrpyt the private key, if not encrypted may be an empty string */
#ifdef IMQTT_EXPERIMENTAL
        std::string clientCert{""}; /*!< the clients certificate as string */
        std::string privateKey{""}; /*!< the clients private key as string */
#endif
#endif
#ifdef IMQTT_USE_PAHO
        bool disableDefaultCaStore{false}; /*!< if true, the systems default CA directory will not be considered */
        bool autoReconnect{true};          /*!< if true, after connection loss, the MQTT library will try to reconnect
                                              automatically (true for Mosquitto always) */
        std::string httpProxy{
            ""}; /*!< the http proxy address for MQTTWS connections (only on Paho),empty string means no proxy */
        std::string httpsProxy{
            ""}; /*!< the https proxy address for MQTTWSS connections (only on Paho), empty string means no proxy */
#endif
#ifdef IMQTT_USE_MOSQ
        bool exponentialBackoff{false}; /*true on PAHO*/
#endif
    };

    /**
     * @brief Converts an IMqttClient reason code into a pair of long and short string representations.
     *
     * @param rc IMqttClient reason code
     * @return ReasonCodeRepr_t
     */
    static ReasonCodeRepr_t ReasonCodeToStringRepr(ReasonCode rc);

    /**
     * @brief Converts an MQTTv3 reason code into a pair of long and short string representations.
     *
     * @param mqttRc MQTTv3 reason code
     * @return MqttReasonCodeRepr_t
     */
    static MqttReasonCodeRepr_t MqttReasonCodeToStringRepr(MqttReasonCode mqttRc);

    /**
     * @brief Converts an MQTTv3 reason code into a pair of long and short string representations.
     *
     * @param mqttRc MQTTv3 reason code
     * @return MqttReasonCodeRepr_t
     */
    static MqttReasonCodeRepr_t MqttReasonCodeToStringRepr(int mqttRc);

    /**
     * @brief Converts an MQTTv5 reason code into a pair of long and short string representations.
     *
     * @param mqttRc MQTTv5 reason code
     * @return Mqtt5ReasonCodeRepr_t
     */
    static Mqtt5ReasonCodeRepr_t Mqtt5ReasonCodeToStringRepr(Mqtt5ReasonCode mqttRc);

    /**
     * @brief Converts an MQTTv5 reason code into a pair of long and short string representations.
     *
     * @param mqttRc MQTTv5 reason code
     * @return Mqtt5ReasonCodeRepr_t
     */
    static Mqtt5ReasonCodeRepr_t Mqtt5ReasonCodeToStringRepr(int mqttRc);

    /**
     * @brief Once an IMqttClient object has been created, the already set callbacks can be changed at runtime. In case
     * a callback shall be disabled, nullptr can be used.
     *
     * @tparam TPtr one of the callback classes defined in IMqttClientCallbacks.h
     * @param ptr pointer to object implementing the callback, user is responsible for object lifetimes
     */
    template <class TPtr>
    void SetCallbacks(TPtr const* ptr = nullptr) noexcept;

    /**
     * @brief Returns the version of the underlying MQTT library
     *
     * @return string representation of the version
     */
    std::string GetLibVersion(void) const noexcept;

    /**
     * @brief Starts an attempt to connect to the broker. The connection attempt is done asynchronously. A return of
     * OKAY means the attempt was started, not that the connection was established. Use
     * IMqttConnectionCallbacks::OnConnectionStatusChanged callbacks to obtain connection information.
     * In case the automatic re-connect feature is enabled, the underlying MQTT libary will continue with connection
     * attempts according to what is set via IMqttClient::InitializeParameters
     * @return IMqttClient ReasonCode
     */
    virtual ReasonCode ConnectAsync(void) = 0;

    /**
     * @brief Starts an attempt to disconnect from the broker. The connection attempt is done asynchronously. A return
     * of OKAY means the attempt was started, not that the connection was closed. Use
     * IMqttConnectionCallbacks::OnConnectionStatusChanged callbacks to obtain connection information.
     *
     * @return IMqttClient ReasonCode
     */
    virtual ReasonCode DisconnectAsync(Mqtt5ReasonCode rc = Mqtt5ReasonCode::SUCCESS) = 0;

    /**
     * @brief Starts an attempt to Subscribe to a given topic. The subscription is done asynchronously, the method
     * returns immediately. A return of OKAY means the attempt was started, not that the subscription finished. Use
     * IMqttCommandCallbacks::OnSubscribe callbacks to obtain further information.
     *
     * @param topic the topic to subscribe to
     * @param qos the Quality of Service used to subscribe
     * @param pToken a token that is set after the method returned, can be used in order to correlate callbacks of
     * IMqttCommandCallbacks::OnSubscribe, may be set to nullptr, if not needed
     * @warning For Paho AND QOS0 the token is always set to 0 (fire and forget strategy)
     * @param getRetained if set true, messages retained at the broker will be received
     * @return the IMqttClient ReasonCode
     */
    virtual ReasonCode SubscribeAsync(std::string const& topic,
                                      IMqttMessage::QOS  qos,
                                      int*               pToken      = nullptr,
                                      bool               getRetained = true) = 0;

    /**
     * @brief Starts an attempt to Unsubscribe from a given topic. The unsubscription is done asynchronously, the method
     * returns immediately. A return of OKAY means the attempt was started, not that the unsubscription finished. Use
     * IMqttCommandCallbacks::OnUnSubscribe callbacks to obtain further information.
     *
     * @param topic the topic to unsubscribe from
     * @param pToken a token that is set after the method returned, can be used in order to correlate callbacks of
     * IMqttCommandCallbacks::OnUnSubscribe, may be set to nullptr, if not needed
     * @warning For Paho AND QOS0 the token is always set to 0 (fire and forget strategy)
     * @return the IMqttClient ReasonCode
     */
    virtual ReasonCode UnSubscribeAsync(std::string const& topic, int* pToken = nullptr) = 0;

    /**
     * @brief Starts an attempt to Publish a message to a given topic. The publish is done asynchronously, the method
     * returns immediately. A return of OKAY means the attempt was started, not that the publish finished. Use
     * IMqttMessageCallbacks::OnPublish callbacks to obtain further information.
     *
     * @param mqttMessage the message to publish
     * @param pToken a token that is set after the method returned, can be used in order to correlate callbacks of
     * IMqttMessageCallbacks::OnPublish, may be set to nullptr, if not needed
     * @warning For Paho AND QOS0 the token is always set to 0 (fire and forget strategy)
     * @return the IMqttClient ReasonCode
     */
    virtual ReasonCode PublishAsync(i_mqtt_client::upMqttMessage_t mqttMessage, int* pToken = nullptr) = 0;
    virtual bool       IsConnected(void) const noexcept                                                = 0;
};

/**
 * @brief Used to instantiate an MqttClient object behind an IMqttClient interface.
 *
 */
class MqttClientFactory final {
public:
    /**
     * @brief Generates an MqttClient object behind an IMqttClient interface. The user is responsible for object lifetime
     * management.
     *
     * @param msg pointer to an object providing a message callback
     * @param log pointer to an object providing a log callback for the IMqtt implementation, may be nullptr if not
     * needed
     * @param cmd pointer to an object providing a command callback for the IMqtt implementation, may be nullptr if not
     * needed
     * @param con pointer to an object providing a connection status callback for the IMqtt implementation, may be
     * nullptr if not needed
     * @return unique pointer to an MqttClient object hidden by an abstract IMqttClient interface
     */
    static std::unique_ptr<IMqttClient> Create(IMqttClient::InitializeParameters const&,
                                               IMqttMessageCallbacks const*    msg,
                                               IMqttLogCallbacks const*        log = nullptr,
                                               IMqttCommandCallbacks const*    cmd = nullptr,
                                               IMqttConnectionCallbacks const* con = nullptr);
    MqttClientFactory() = delete;
};
}  // namespace i_mqtt_client
