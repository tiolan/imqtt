/**
 * @file IMqttClient.h
 * @author Timo Lange
 * @brief
 * @date 2020
 * @copyright Timo Lange
 */

#include "MosquittoClient.h"

#include <mqtt_protocol.h>
#ifdef IMQTT_WITH_TLS
#include "openssl/ssl.h"
#endif

#include <iostream>
#include <stdexcept>

using namespace std;

namespace mqttclient {
std::atomic_uint MosquittoClient::counter{0ul};
std::string      MosquittoClient::libVersion;
std::mutex       MosquittoClient::libMutex;

MosquittoClient::MosquittoClient(IMqttClient::InitializeParameters const& parameters)
  : params(parameters)
  , messageDispatcherThread(&MosquittoClient::messageDispatcherWorker, this)
{
    setCallbacks(params.callbackProvider);
    bool initError{false};
    {
        lock_guard<mutex> lock(libMutex);
        // Init lib, if nobody ever did
        if (counter.fetch_add(1) == 0) {
            cbs.log->Log(LogLevel::Info, "Initializing mosquitto lib");
            initError |= mosquitto_lib_init();
            int major = 0, minor = 0, patch = 0;
            (void)mosquitto_lib_version(&major, &minor, &patch);
            libVersion = "libmosquitto " + to_string(major) + "." + to_string(minor) + "." + to_string(patch);
        };
    }  // unlock mutex, from here everything is instance specific

    // Init instance
    cbs.log->Log(LogLevel::Info, "Initializing mosquitto instance");
    cbs.log->Log(LogLevel::Info, "Broker-Address: " + params.hostAddress + ":" + to_string(params.port));
    pClient = mosquitto_new(params.clientId.c_str(), params.cleanSession, this);

    if (!params.mqttUsername.empty()) {
        initError |= mosquitto_username_pw_set(pClient, params.mqttUsername.c_str(), params.mqttPassword.c_str());
    }

    auto reconnMin{
        params.reconnectDelayMin +
        uniform_int_distribution<int>(params.reconnectDelayMinLower, params.reconnectDelayMinUpper)(rndGenerator)};
    cbs.log->Log(LogLevel::Debug,
                 "Reconnect delay min: " + to_string(reconnMin) + "," + " max: " + to_string(params.reconnectDelayMax));
    initError |= mosquitto_reconnect_delay_set(pClient, reconnMin, params.reconnectDelayMax, params.exponentialBackoff);
    initError |= mosquitto_int_option(pClient, MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);
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
    initError |= mosquitto_tls_set(
        pClient,
        params.caFilePath.empty() ? nullptr : params.caFilePath.c_str(),
        params.caDirPath.empty() ? nullptr : params.caDirPath.c_str(),
        params.clientCertFilePath.empty() ? nullptr : params.clientCertFilePath.c_str(),
        params.privateKeyFilePath.empty() ? nullptr : params.privateKeyFilePath.c_str(),
        [](char* buf, int size, int rwflag, void* pClient) -> int {
            if (rwflag == 1) {
                return static_cast<int>(
                    static_cast<MosquittoClient*>(mosquitto_userdata(static_cast<mosquitto*>(pClient)))
                        ->params.privateKeyPassword.copy(buf, size));
            }
            return 0;
        });
    initError |= mosquitto_tls_opts_set(pClient, SSL_VERIFY_PEER, nullptr, nullptr);
#endif
    cbs.log->Log(LogLevel::Info, "Starting mosquitto instance");
    initError |= mosquitto_loop_start(pClient);
    if (initError) {
        throw runtime_error("Was not able to initialize mosquitto lib");
    }
}

void
MosquittoClient::messageDispatcherWorker(void)
{
    cbs.log->Log(LogLevel::Debug, "Starting MQTT message dispatcher");
    unique_lock<mutex> lock(messageDispatcherMutex);
    while (!messageDispatcherExit) {
        cbs.log->Log(
            LogLevel::Debug,
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
    cbs.log->Log(LogLevel::Info, "Exiting MQTT message dispatcher");
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
    cbs.log->Log(LogLevel::Info, "Deinitializing mosquitto instance");
    Disconnect();
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
        cbs.log->Log(LogLevel::Info, "Deinitializing mosquitto library");
        mosquitto_lib_cleanup();
    }
}

void
MosquittoClient::onConnectCb(struct mosquitto* pClient, int rc, int flags, const mosquitto_property* pProps)
{
    (void)pClient;
    (void)flags;
    (void)pProps;
    cbs.log->Log(LogLevel::Info, "Mosquitto connection to broker: " + string(mosquitto_strerror(rc)));
    if (rc == MOSQ_ERR_SUCCESS) {
        connected = true;
        cbs.con->OnConnectionStatusChanged(IMqttConnectionCallbacks::ConnectionStatus::Connected);
    }
}

void
MosquittoClient::onDisconnectCb(struct mosquitto* pClient, int rc, const mosquitto_property* pProps)
{
    (void)pClient;
    (void)pProps;
    auto loglvl{IMqttClientCallbacks::LogLevel::Warning};
    if (rc == MOSQ_ERR_SUCCESS) {
        loglvl = IMqttClientCallbacks::LogLevel::Info;
    }
    cbs.log->Log(loglvl, "Mosquitto Disconnected from broker: " + string(mosquitto_strerror(rc)));
    if (rc == MOSQ_ERR_SUCCESS) {
        connected = false;
        cbs.con->OnConnectionStatusChanged(IMqttConnectionCallbacks::ConnectionStatus::Disconnected);
    }
}

void
MosquittoClient::onPublishCb(struct mosquitto* pClient, int messageId, int rc, const mosquitto_property* pProps)
{
    (void)pClient;
    (void)pProps;
    cbs.log->Log(LogLevel::Debug, "Mosquitto published: " + to_string(rc));
    cbs.msg->OnPublish(messageId);
}

void
MosquittoClient::onMessageCb(struct mosquitto*               pClient,
                             const struct mosquitto_message* pMsg,
                             const mosquitto_property*       pProps)
{
    (void)pClient;

    cbs.log->Log(LogLevel::Debug, "Mosquitto received message");

    auto mqttMessage{MqttMessageFactory::create(
        pMsg->topic,
        IMqttMessage::payload_t(static_cast<IMqttMessage::payloadRaw_t*>(pMsg->payload),
                                static_cast<IMqttMessage::payloadRaw_t*>(pMsg->payload) + pMsg->payloadlen),
        IMqttMessage::intToQos(pMsg->qos),
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
                    cbs.log->Log(LogLevel::Error, "Was not able to add user props - ignoring");
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
            formatIndicator == 1u ? IMqttMessage::FormatIndicator::UTF8 : IMqttMessage::FormatIndicator::Unspecified;
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
        cbs.log->Log(LogLevel::Debug, "Mosquitto Subscribed with QOS: " + to_string(*(pGrantedQos + i)));
    }
    cbs.msg->OnSubscribe(messageId);
}

void
MosquittoClient::onUnSubscribeCb(struct mosquitto* pClient, int messageId, const mosquitto_property* pProps)
{
    (void)pClient;
    (void)pProps;
    cbs.log->Log(LogLevel::Debug, "Mosquitto UnSubscribed");
    cbs.msg->OnUnSubscribe(messageId);
}

void
MosquittoClient::onLog(struct mosquitto* pClient, int logLevel, const char* pTxt)
{
    (void)pClient;
    auto logLvl{LogLevel::Info};
    switch (logLevel) {
    case MOSQ_LOG_WARNING:
        logLvl = LogLevel::Warning;
        break;
    case MOSQ_LOG_ERR:
        logLvl = LogLevel::Error;
        break;
    case MOSQ_LOG_SUBSCRIBE:
        [[fallthrough]];
    case MOSQ_LOG_UNSUBSCRIBE:
        [[fallthrough]];
    case MOSQ_LOG_WEBSOCKETS:
        [[fallthrough]];
    case MOSQ_LOG_DEBUG:
        logLvl = LogLevel::Debug;
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

void
MosquittoClient::ConnectAsync(void)
{
    cbs.log->Log(LogLevel::Info, "Connecting to broker async: " + params.hostAddress + ":" + to_string(params.port));
    int rc{mosquitto_connect_async(pClient, params.hostAddress.c_str(), params.port, params.keepAliveInterval)};
    if (MOSQ_ERR_SUCCESS != rc) {
        cbs.log->Log(LogLevel::Fatal,
                     "Mosquitto_connect_async could not be called: " + string(mosquitto_strerror(rc)) + " (" +
                         to_string(rc) + ")");
        throw runtime_error("mosquitto_connect_async could not be called");
    }
}

void
MosquittoClient::Disconnect(void)
{
    cbs.log->Log(LogLevel::Info, "Disconnecting from broker");
    int rc{mosquitto_disconnect(pClient)};
    if (MOSQ_ERR_SUCCESS != rc && MOSQ_ERR_NO_CONN != rc) {
        cbs.log->Log(
            LogLevel::Fatal,
            "Mosquitto_Disconnect could not be called: " + string(mosquitto_strerror(rc)) + " (" + to_string(rc) + ")");
        throw runtime_error("mosquitto_disconnect could not be called");
    }
}

IMqttClient::RetCodes
MosquittoClient::SubscribeAsync(string const& topic, IMqttMessage::QOS qos, int* token, bool getRetained)
{
    cbs.log->Log(LogLevel::Trace, "Subscribing to topic: \"" + topic + "\"");
    int options{0};
    if (!params.allowLocalTopics) {
        options |= mqtt5_sub_options::MQTT_SUB_OPT_NO_LOCAL;
    };
    if (getRetained == false) {
        options |= mqtt5_sub_options::MQTT_SUB_OPT_SEND_RETAIN_NEVER;
    }
    switch (mosquitto_subscribe_v5(pClient, token, topic.c_str(), IMqttMessage::qosToInt(qos), options, NULL)) {
    case MOSQ_ERR_SUCCESS:
        return IMqttClient::RetCodes::OKAY;
    case MOSQ_ERR_NO_CONN:
        return IMqttClient::RetCodes::ERROR_TEMPORARY;
    default:
        break;
    }

    return IMqttClient::RetCodes::ERROR_PERMANENT;
}

IMqttClient::RetCodes
MosquittoClient::UnSubscribeAsync(string const& topic, int* token)
{
    cbs.log->Log(LogLevel::Trace, "Unsubscribing from topic: \"" + topic + "\"");
    switch (mosquitto_unsubscribe_v5(pClient, token, topic.c_str(), NULL)) {
    case MOSQ_ERR_SUCCESS:
        return IMqttClient::RetCodes::OKAY;
    case MOSQ_ERR_NO_CONN:
        return IMqttClient::RetCodes::ERROR_TEMPORARY;
    default:
        break;
    }

    return IMqttClient::RetCodes::ERROR_PERMANENT;
}

IMqttClient::RetCodes
MosquittoClient::PublishAsync(upMqttMessage_t mqttMsg, int* token)
{
    cbs.log->Log(LogLevel::Trace, "Publishing to topic: \"" + mqttMsg->getTopic() + "\"");

    auto propertiesOkay{true};
    auto status{IMqttClient::RetCodes::ERROR_PERMANENT};

    mosquitto_property* pProps{nullptr};
    for (auto const& prop : mqttMsg->userProps) {
        if (MOSQ_ERR_SUCCESS != mosquitto_property_add_string_pair(
                                    &pProps, MQTT_PROP_USER_PROPERTY, prop.first.c_str(), prop.second.c_str())) {
            cbs.log->Log(LogLevel::Error, "Invalid MQTT user property - ignoring message");
            propertiesOkay = false;
            break;
        }
    }

    if (MOSQ_ERR_SUCCESS != mosquitto_property_add_binary(&pProps,
                                                          MQTT_PROP_CORRELATION_DATA,
                                                          mqttMsg->correlationDataProps.data(),
                                                          mqttMsg->correlationDataProps.size())) {
        cbs.log->Log(LogLevel::Error, "Invalid MQTT correlation data property - ignoring message");
        propertiesOkay = false;
    }

    if (MOSQ_ERR_SUCCESS !=
        mosquitto_property_add_string(&pProps, MQTT_PROP_RESPONSE_TOPIC, mqttMsg->responseTopic.c_str())) {
        cbs.log->Log(LogLevel::Error, "Invalid MQTT response topic - ignoring message");
        propertiesOkay = false;
    }

    if (MOSQ_ERR_SUCCESS !=
        mosquitto_property_add_string(&pProps, MQTT_PROP_CONTENT_TYPE, mqttMsg->payloadContentType.c_str())) {
        cbs.log->Log(LogLevel::Error, "Invalid MQTT content type - ignoring message");
        propertiesOkay = false;
    }

    if (MOSQ_ERR_SUCCESS !=
        mosquitto_property_add_byte(&pProps,
                                    MQTT_PROP_PAYLOAD_FORMAT_INDICATOR,
                                    mqttMsg->payloadFormatIndicator == IMqttMessage::FormatIndicator::UTF8 ? 1 : 0)) {
        cbs.log->Log(LogLevel::Error, "Invalid MQTT format indicator - ignoring message");
        propertiesOkay = false;
    }

    if (propertiesOkay) {
        switch (mosquitto_publish_v5(pClient,
                                     token,
                                     mqttMsg->getTopic().c_str(),
                                     mqttMsg->getPayload().size(),
                                     mqttMsg->getPayload().data(),
                                     IMqttMessage::qosToInt(mqttMsg->getQos()),
                                     mqttMsg->getRetained(),
                                     pProps)) {
        case MOSQ_ERR_SUCCESS:
            status = IMqttClient::RetCodes::OKAY;
            break;
        case MOSQ_ERR_NO_CONN:
            status = IMqttClient::RetCodes::ERROR_TEMPORARY;
            break;
        default:
            status = IMqttClient::RetCodes::ERROR_PERMANENT;
            break;
        }
    }
    if (status != IMqttClient::RetCodes::OKAY) {
        cbs.log->Log(LogLevel::Error, "PublishAsync failed - will not retry");
    }

    mosquitto_property_free_all(&pProps);
    return status;
}

IMqttConnectionCallbacks::ConnectionStatus
MosquittoClient::GetConnectionStatus(void) const noexcept
{
    return connected ? ConnectionStatus::Connected : ConnectionStatus::Disconnected;
}

unique_ptr<IMqttClient>
MqttClientFactory::create(IMqttClient::InitializeParameters const& params)
{
    return unique_ptr<MosquittoClient>(new MosquittoClient(params));
}

}  // namespace mqttclient