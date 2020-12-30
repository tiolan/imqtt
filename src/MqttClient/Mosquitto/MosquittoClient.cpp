/**
 * @file MosquittoClient.cpp
 * @author Timo Lange
 * @brief Implementation of wrapper for mosquitto library
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

#include "MosquittoClient.h"

#include <mqtt_protocol.h>
#ifdef IMQTT_WITH_TLS
#include "openssl/ssl.h"
#endif

#include <iostream>
#include <stdexcept>

using namespace std;

namespace i_mqtt_client {
atomic_uint MosquittoClient::counter{0ul};
string      MosquittoClient::libVersion;
mutex       MosquittoClient::libMutex;

MosquittoClient::MosquittoClient(IMqttClient::InitializeParameters const& parameters)
  : params(parameters)
  , messageDispatcherThread(&MosquittoClient::messageDispatcherWorker, this)
{
    auto rc{static_cast<int>(MOSQ_ERR_SUCCESS)};
    setCallbacks(params.callbackProvider);
    {
        lock_guard<mutex> lock(libMutex);
        // Init lib, if nobody ever did
        if (counter.fetch_add(1) == 0) {
            cbs.log->Log(LogLevel::INFO, "Initializing mosquitto lib");
            rc = mosquitto_lib_init();
            if (MOSQ_ERR_SUCCESS != rc) {
                throw runtime_error("Was not able to initialize mosquitto lib: " + string(mosquitto_strerror(rc)));
            }
            int major = 0, minor = 0, patch = 0;
            int vers   = mosquitto_lib_version(&major, &minor, &patch);
            libVersion = "libmosquitto " + to_string(major) + "." + to_string(minor) + "." + to_string(patch) + " (" +
                         to_string(vers) + ")";
        };
    }  // unlock mutex, from here everything is instance specific

    // Init instance
    cbs.log->Log(LogLevel::INFO, "Initializing mosquitto instance");
    cbs.log->Log(LogLevel::INFO, "Broker-Address: " + params.hostAddress + ":" + to_string(params.port));
    pClient = mosquitto_new(params.clientId.c_str(), params.cleanSession, this);

    if (!params.mqttUsername.empty()) {
        rc = mosquitto_username_pw_set(pClient, params.mqttUsername.c_str(), params.mqttPassword.c_str());
        if (MOSQ_ERR_SUCCESS != rc) {
            throw runtime_error("Was not able to set MQTT credentials: " + string(mosquitto_strerror(rc)));
        }
    }

    auto reconnMin{
        params.reconnectDelayMin +
        uniform_int_distribution<int>(params.reconnectDelayMinLower, params.reconnectDelayMinUpper)(rndGenerator)};
    cbs.log->Log(LogLevel::DEBUG,
                 "Reconnect delay min: " + to_string(reconnMin) + "," + " max: " + to_string(params.reconnectDelayMax));
    rc = mosquitto_reconnect_delay_set(pClient, reconnMin, params.reconnectDelayMax, params.exponentialBackoff);
    if (MOSQ_ERR_SUCCESS != rc) {
        throw runtime_error("Was not able to set reconnect delay: " + string(mosquitto_strerror(rc)));
    }
    rc = mosquitto_int_option(pClient, MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);
    if (MOSQ_ERR_SUCCESS != rc) {
        throw runtime_error("Was not able to set MQTT version: " + string(mosquitto_strerror(rc)));
    }
    mosquitto_connect_v5_callback_set(
        pClient, [](struct mosquitto* pClient, void* pThis, int rc, int flags, const mosquitto_property* pProps) {
            static_cast<MosquittoClient*>(pThis)->onConnectCb(pClient, rc, flags, pProps);
        });
    mosquitto_disconnect_v5_callback_set(
        pClient, [](struct mosquitto* pClient, void* pThis, int rc, const mosquitto_property* pProps) {
            static_cast<MosquittoClient*>(pThis)->onDisconnectCb(pClient, rc, pProps);
        });
    mosquitto_publish_v5_callback_set(
        pClient, [](struct mosquitto* pClient, void* pThis, int messageId, int rc, const mosquitto_property* pProps) {
            static_cast<MosquittoClient*>(pThis)->onPublishCb(pClient, messageId, rc, pProps);
        });
    mosquitto_message_v5_callback_set(pClient,
                                      [](struct mosquitto*               pClient,
                                         void*                           pThis,
                                         const struct mosquitto_message* pMsg,
                                         const mosquitto_property*       pProps) {
                                          static_cast<MosquittoClient*>(pThis)->onMessageCb(pClient, pMsg, pProps);
                                      });
    mosquitto_subscribe_v5_callback_set(pClient,
                                        [](struct mosquitto*         pClient,
                                           void*                     pThis,
                                           int                       messageId,
                                           int                       grantedQosCount,
                                           const int*                pGrantedQos,
                                           const mosquitto_property* pProps) {
                                            static_cast<MosquittoClient*>(pThis)->onSubscribeCb(
                                                pClient, messageId, grantedQosCount, pGrantedQos, pProps);
                                        });
    mosquitto_unsubscribe_v5_callback_set(
        pClient, [](struct mosquitto* pClient, void* pThis, int messageId, const mosquitto_property* pProps) {
            static_cast<MosquittoClient*>(pThis)->onUnSubscribeCb(pClient, messageId, pProps);
        });
    mosquitto_log_callback_set(pClient, [](struct mosquitto* pClient, void* pThis, int logLevel, const char* pTxt) {
        static_cast<MosquittoClient*>(pThis)->onLog(pClient, logLevel, pTxt);
    });
#ifdef IMQTT_WITH_TLS
    rc = mosquitto_tls_set(pClient,
                           params.caFilePath.empty() ? nullptr : params.caFilePath.c_str(),
                           params.caDirPath.empty() ? nullptr : params.caDirPath.c_str(),
                           params.clientCertFilePath.empty() ? nullptr : params.clientCertFilePath.c_str(),
                           params.privateKeyFilePath.empty() ? nullptr : params.privateKeyFilePath.c_str(),
                           [](char* buf, int size, int rwflag, void* pClient) -> int {
                               if (rwflag != 0) {
                                   return static_cast<int>(static_cast<MosquittoClient*>(
                                                               mosquitto_userdata(static_cast<mosquitto*>(pClient)))
                                                               ->params.privateKeyPassword.copy(buf, size));
                               }
                               return 0;
                           });
    if (MOSQ_ERR_SUCCESS != rc) {
        throw runtime_error("Was not able to set TLS settings: " + string(mosquitto_strerror(rc)));
    }
    rc = mosquitto_tls_opts_set(pClient, SSL_VERIFY_PEER, nullptr, nullptr);
    if (MOSQ_ERR_SUCCESS != rc) {
        throw runtime_error("Was not able to set TLS options: " + string(mosquitto_strerror(rc)));
    }
#endif
    cbs.log->Log(LogLevel::INFO, "Starting mosquitto instance");
    rc = mosquitto_loop_start(pClient);
    if (MOSQ_ERR_SUCCESS != rc) {
        throw runtime_error("Was not able to start mosquitto loop: " + string(mosquitto_strerror(rc)));
    }
}

void
MosquittoClient::messageDispatcherWorker(void)
{
    cbs.log->Log(LogLevel::DEBUG, "Starting MQTT message dispatcher");
    unique_lock<mutex> lock(messageDispatcherMutex);
    while (!messageDispatcherExit) {
        cbs.log->Log(
            LogLevel::DEBUG,
            "Number of MQTT messages to be processed: " + to_string(messageDispatcherQueue.size()) + " messages");
        messageDispatcherAwaiter.wait(lock,
                                      [this] { return (messageDispatcherQueue.size() || messageDispatcherExit); });
        if (!messageDispatcherExit && messageDispatcherQueue.size()) {
            lock.unlock();
            cbs.msg->OnMqttMessage(move(messageDispatcherQueue.front()));
            messageDispatcherQueue.pop();
            lock.lock();
        }
    }
    cbs.log->Log(LogLevel::INFO, "Exiting MQTT message dispatcher");
}

void
MosquittoClient::dispatchMessage(upMqttMessage_t&& msg)
{
    unique_lock<mutex> lock(messageDispatcherMutex);
    messageDispatcherQueue.push(move(msg));
    lock.unlock();
    messageDispatcherAwaiter.notify_one();
}

MosquittoClient::~MosquittoClient() noexcept
{
    cbs.log->Log(LogLevel::INFO, "Deinitializing mosquitto instance");
    if (IsConnected()) {
        Disconnect(Mqtt5ReasonCode::SUCCESS);
    }
    mosquitto_loop_stop(pClient, false);
    messageDispatcherExit = true;
    messageDispatcherMutex.unlock();
    messageDispatcherAwaiter.notify_all();
    if (messageDispatcherThread.joinable()) {
        messageDispatcherThread.join();
    }
    mosquitto_destroy(pClient);
    // If no users are left, clean the lib
    lock_guard<mutex> l(libMutex);
    if (counter.fetch_sub(1) == 1) {
        cbs.log->Log(LogLevel::INFO, "Deinitializing mosquitto library");
        mosquitto_lib_cleanup();
    }
}

void
MosquittoClient::onConnectCb(struct mosquitto* pClient, int mqttRc, int flags, const mosquitto_property* pProps)
{
    (void)pClient;
    (void)flags;
    (void)pProps;
    auto logLvl{LogLevel::WARNING};
    if (Mqtt5ReasonCode::SUCCESS == static_cast<Mqtt5ReasonCode>(mqttRc)) {
        connected = true;
        logLvl    = LogLevel::INFO;
    }
    cbs.log->Log(logLvl, "Mosquitto connected to broker, rc: " + Mqtt5ReasonCodeToStringRepr(mqttRc).first);
    cbs.con->OnConnectionStatusChanged(IMqttConnectionCallbacks::ConnectionType::CONNECT,
                                       static_cast<Mqtt5ReasonCode>(mqttRc));
}

void
MosquittoClient::onDisconnectCb(struct mosquitto* pClient, int mqttRc, const mosquitto_property* pProps)
{
    (void)pClient;
    (void)pProps;
    connected = false;
    cbs.log->Log(LogLevel::WARNING,
                 "Mosquitto disconnected from broker, rc: " + Mqtt5ReasonCodeToStringRepr(mqttRc).first);
    cbs.con->OnConnectionStatusChanged(IMqttConnectionCallbacks::ConnectionType::DISCONNECT,
                                       static_cast<Mqtt5ReasonCode>(mqttRc));
}

void
MosquittoClient::onPublishCb(struct mosquitto* pClient, int messageId, int mqttRc, const mosquitto_property* pProps)
{
    (void)pClient;
    (void)pProps;
    cbs.log->Log(LogLevel::DEBUG,
                 "Mosquitto publish completed for token: " + to_string(messageId) +
                     ", rc: " + Mqtt5ReasonCodeToStringRepr(mqttRc).first);
    cbs.msg->OnPublish(messageId, static_cast<Mqtt5ReasonCode>(mqttRc));
}

void
MosquittoClient::onMessageCb(struct mosquitto*               pClient,
                             const struct mosquitto_message* pMsg,
                             const mosquitto_property*       pProps)
{
    (void)pClient;

    cbs.log->Log(LogLevel::DEBUG, "Mosquitto received message");

    auto mqttMessage{MqttMessageFactory::create(
        pMsg->topic,
        IMqttMessage::payload_t(static_cast<IMqttMessage::payloadRaw_t*>(pMsg->payload),
                                static_cast<IMqttMessage::payloadRaw_t*>(pMsg->payload) + pMsg->payloadlen),
        static_cast<IMqttMessage::QOS>(pMsg->qos),
        pMsg->retain)};
    mqttMessage->messageId = pMsg->mid;

    {
        const mosquitto_property* pUserProps{pProps};
        bool                      skipFirst{false};
        do {
            char* key{nullptr};
            char* val{nullptr};
            pUserProps =
                mosquitto_property_read_string_pair(pUserProps, MQTT_PROP_USER_PROPERTY, &key, &val, skipFirst);
            skipFirst = true;
            if (key) {
                auto keyStr = string(key);
                if (!mqttMessage->userProps.insert(make_pair(keyStr, val ? string(val) : string())).second) {
                    cbs.log->Log(LogLevel::ERROR, "Was not able to add user props - ignoring");
                }
            }
        } while (pUserProps);
    }

    {
        uint16_t corellationDataSize{0};
        void*    pCorrelationData{nullptr};
        (void)mosquitto_property_read_binary(
            pProps, MQTT_PROP_CORRELATION_DATA, &pCorrelationData, &corellationDataSize, false);
        if (pCorrelationData) {
            mqttMessage->correlationDataProps = IMqttMessage::correlationDataProps_t(
                static_cast<unsigned char*>(pCorrelationData),
                static_cast<unsigned char*>(pCorrelationData) + corellationDataSize);
        }
    }

    {
        char* pResponseTopic{nullptr};
        (void)mosquitto_property_read_string(pProps, MQTT_PROP_RESPONSE_TOPIC, &pResponseTopic, false);
        if (pResponseTopic) {
            mqttMessage->responseTopic = string(pResponseTopic);
        }
    }

    {
        char* pContentType{nullptr};
        (void)mosquitto_property_read_string(pProps, MQTT_PROP_CONTENT_TYPE, &pContentType, false);
        if (pContentType) {
            mqttMessage->responseTopic = string(pContentType);
        }
    }

    {
        uint8_t formatIndicator{0u};
        (void)mosquitto_property_read_byte(pProps, MQTT_PROP_PAYLOAD_FORMAT_INDICATOR, &formatIndicator, false);
        mqttMessage->payloadFormatIndicator =
            formatIndicator == 1u ? IMqttMessage::FormatIndicator::UTF8 : IMqttMessage::FormatIndicator::UNSPECIFIED;
    }

    dispatchMessage(move(mqttMessage));
}

void
MosquittoClient::onSubscribeCb(struct mosquitto*         pClient,
                               int                       messageId,
                               int                       grantedQosCount,
                               const int*                pGrantedQos,
                               const mosquitto_property* pProps)
{
    (void)pClient;
    (void)pProps;
    for (int i{0}; i < grantedQosCount; i++) {
        cbs.log->Log(LogLevel::DEBUG, "Mosquitto Subscribe completed with QOS: " + to_string(*(pGrantedQos + i)));
    }
    // TODO: How to get the Mqtt5ReasonCode in order to hand it over to the user
    cbs.msg->OnSubscribe(messageId);
}

void
MosquittoClient::onUnSubscribeCb(struct mosquitto* pClient, int messageId, const mosquitto_property* pProps)
{
    (void)pClient;
    (void)pProps;
    cbs.log->Log(LogLevel::DEBUG, "Mosquitto UnSubscribe completed");
    // TODO: How to get the Mqtt5ReasonCode in order to hand it over to the user
    cbs.msg->OnUnSubscribe(messageId);
}

void
MosquittoClient::onLog(struct mosquitto* pClient, int logLevel, const char* pTxt)
{
    (void)pClient;
    auto logLvl{LogLevel::INFO};
    switch (logLevel) {
    case MOSQ_LOG_WARNING:
        logLvl = LogLevel::WARNING;
        break;
    case MOSQ_LOG_ERR:
        logLvl = LogLevel::ERROR;
        break;
    case MOSQ_LOG_SUBSCRIBE:
        [[fallthrough]];
    case MOSQ_LOG_UNSUBSCRIBE:
        [[fallthrough]];
    case MOSQ_LOG_WEBSOCKETS:
        [[fallthrough]];
    case MOSQ_LOG_DEBUG:
        logLvl = LogLevel::DEBUG;
        break;
    case MOSQ_LOG_NOTICE:
        [[fallthrough]];
    case MOSQ_LOG_INFO:
        [[fallthrough]];
    default:
        cbs.log->Log(logLvl, string(pTxt));
        break;
    }
}

std::string
MosquittoClient::GetLibVersion(void) const noexcept
{
    return libVersion;
}

ReasonCode
MosquittoClient::ConnectAsync(void)
{
    cbs.log->Log(LogLevel::INFO, "Connecting to broker async: " + params.hostAddress + ":" + to_string(params.port));
    return mosqRcToReasonCode(
        mosquitto_connect_async(pClient, params.hostAddress.c_str(), params.port, params.keepAliveInterval),
        "mosquitto_connect_async");
}

ReasonCode
MosquittoClient::Disconnect(Mqtt5ReasonCode rc)
{
    cbs.log->Log(LogLevel::INFO, "Disconnecting from broker");
    return mosqRcToReasonCode(mosquitto_disconnect_v5(pClient, static_cast<int>(rc), NULL), "mosquitto_disconnect_v5");
}

ReasonCode
MosquittoClient::SubscribeAsync(string const& topic, IMqttMessage::QOS qos, int* token, bool getRetained)
{
    cbs.log->Log(LogLevel::DEBUG, "Subscribing to topic: \"" + topic + "\"");
    int options{0};
    if (!params.allowLocalTopics) {
        options |= mqtt5_sub_options::MQTT_SUB_OPT_NO_LOCAL;
    };
    if (getRetained == false) {
        options |= mqtt5_sub_options::MQTT_SUB_OPT_SEND_RETAIN_NEVER;
    }
    return mosqRcToReasonCode(
        mosquitto_subscribe_v5(pClient, token, topic.c_str(), static_cast<int>(qos), options, NULL),
        "mosquitto_subscribe_v5");
}

ReasonCode
MosquittoClient::UnSubscribeAsync(string const& topic, int* token)
{
    cbs.log->Log(LogLevel::DEBUG, "Unsubscribing from topic: \"" + topic + "\"");
    return mosqRcToReasonCode(mosquitto_unsubscribe_v5(pClient, token, topic.c_str(), NULL),
                              "mosquitto_unsubscribe_v5");
}

ReasonCode
MosquittoClient::PublishAsync(upMqttMessage_t mqttMsg, int* token)
{
    cbs.log->Log(LogLevel::DEBUG, "Publishing to topic: \"" + mqttMsg->topic + "\"");

    auto propertiesOkay{true};

    mosquitto_property* pProps{nullptr};
    for (auto const& prop : mqttMsg->userProps) {
        if (MOSQ_ERR_SUCCESS != mosquitto_property_add_string_pair(
                                    &pProps, MQTT_PROP_USER_PROPERTY, prop.first.c_str(), prop.second.c_str())) {
            cbs.log->Log(LogLevel::ERROR, "Invalid MQTT user property - ignoring message");
            propertiesOkay = false;
            break;
        }
    }

    if (MOSQ_ERR_SUCCESS != mosquitto_property_add_binary(&pProps,
                                                          MQTT_PROP_CORRELATION_DATA,
                                                          mqttMsg->correlationDataProps.data(),
                                                          mqttMsg->correlationDataProps.size())) {
        cbs.log->Log(LogLevel::ERROR, "Invalid MQTT correlation data property - ignoring message");
        propertiesOkay = false;
    }

    if (MOSQ_ERR_SUCCESS !=
        mosquitto_property_add_string(&pProps, MQTT_PROP_RESPONSE_TOPIC, mqttMsg->responseTopic.c_str())) {
        cbs.log->Log(LogLevel::ERROR, "Invalid MQTT response topic - ignoring message");
        propertiesOkay = false;
    }

    if (MOSQ_ERR_SUCCESS !=
        mosquitto_property_add_string(&pProps, MQTT_PROP_CONTENT_TYPE, mqttMsg->payloadContentType.c_str())) {
        cbs.log->Log(LogLevel::ERROR, "Invalid MQTT content type - ignoring message");
        propertiesOkay = false;
    }

    if (MOSQ_ERR_SUCCESS !=
        mosquitto_property_add_byte(&pProps,
                                    MQTT_PROP_PAYLOAD_FORMAT_INDICATOR,
                                    mqttMsg->payloadFormatIndicator == IMqttMessage::FormatIndicator::UTF8 ? 1 : 0)) {
        cbs.log->Log(LogLevel::ERROR, "Invalid MQTT format indicator - ignoring message");
        propertiesOkay = false;
    }

    auto status{ReasonCode::ERROR_GENERAL};
    if (propertiesOkay) {
        status = mosqRcToReasonCode(mosquitto_publish_v5(pClient,
                                                         token,
                                                         mqttMsg->topic.c_str(),
                                                         mqttMsg->payload.size(),
                                                         mqttMsg->payload.data(),
                                                         static_cast<int>(mqttMsg->qos),
                                                         mqttMsg->retain,
                                                         pProps),
                                    "mosquitto_publish_v5");
    }
    if (ReasonCode::OKAY != status) {
        cbs.log->Log(LogLevel::ERROR, "PublishAsync failed - will not retry");
    }
    mosquitto_property_free_all(&pProps);
    return status;
}

bool
MosquittoClient::IsConnected(void) const noexcept
{
    return connected;
}

ReasonCode
MosquittoClient::mosqRcToReasonCode(int rc, string const& details) const
{
    auto status{ReasonCode::ERROR_GENERAL};
    auto logLvl{LogLevel::ERROR};
    switch (rc) {
    case MOSQ_ERR_SUCCESS:
        logLvl = LogLevel::DEBUG;
        status = ReasonCode::OKAY;
        break;
    case MOSQ_ERR_TLS:
        [[fallthrough]];
    case MOSQ_ERR_TLS_HANDSHAKE:
        logLvl = LogLevel::ERROR;
        status = ReasonCode::ERROR_TLS;
        break;
    case MOSQ_ERR_CONN_LOST:
        [[fallthrough]];
    case MOSQ_ERR_NO_CONN:
        logLvl = LogLevel::WARNING;
        status = ReasonCode::ERROR_NO_CONNECTION;
        break;
    case MOSQ_ERR_AUTH:
        logLvl = LogLevel::ERROR;
        status = ReasonCode::NOT_ALLOWED;
        break;
    default:
        break;
    }
    cbs.log->Log(logLvl,
                 details + ": " + ReasonCodeToStringRepr(status).first + ", Mosq: " + string(mosquitto_strerror(rc)));
    return status;
}

unique_ptr<IMqttClient>
MqttClientFactory::create(IMqttClient::InitializeParameters const& params)
{
    return unique_ptr<MosquittoClient>(new MosquittoClient(params));
}

}  // namespace i_mqtt_client