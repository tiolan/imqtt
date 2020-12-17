/**
 * @file IMqttClient.h
 * @author Timo Lange
 * @brief
 * @date 2020
 * @copyright Timo Lange
 */

#include "MosquittoClient.h"

#include <mqtt_protocol.h>

#include <iostream>
#include <stdexcept>
#include <string>

using namespace std;

namespace {
constexpr auto MOSQUITTO_KEEP_ALIVE_INTERVAL{15};
}  // namespace

namespace mqttclient {
std::atomic_uint MosquittoClient::counter{0ul};
std::string      MosquittoClient::libVersion;
std::mutex       MosquittoClient::libMutex;

MosquittoClient::MosquittoClient(std::string                address,
                                 int                        port,
                                 std::string                clientId,
                                 MqttClientCallbacks const& callbacks)
  : address(address)
  , port(port)
  , id(clientId)
  , messageDispatcherThread(&MosquittoClient::messageDispatcherWorker, this)
{
    setCallbacks(callbacks);
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
    pClient = mosquitto_new(id.c_str(), cleanSession, this);

    // initError |= mosquitto_threaded_set(pClient, true);
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
    (void)messageId;
    (void)pProps;
    cbs.log->Log(LogLevel::Debug, "Mosquitto published: " + to_string(rc));
}

void
MosquittoClient::onMessageCb(struct mosquitto*               pClient,
                             const struct mosquitto_message* pMsg,
                             const mosquitto_property*       pProps)
{
    (void)pClient;

    cbs.log->Log(LogLevel::Debug, "Mosquitto received message");

    IMqttMessage::userProps_t userProps;
    const mosquitto_property* pUserProps{pProps};
    bool                      skipFirst{false};
    do {
        char* key{nullptr};
        char* val{nullptr};
        pUserProps = mosquitto_property_read_string_pair(pUserProps, MQTT_PROP_USER_PROPERTY, &key, &val, skipFirst);
        skipFirst  = true;
        if (key) {
            auto keyStr       = string(key);
            userProps[keyStr] = val ? string(val) : string();
        }
    } while (pUserProps);

    uint16_t corellationDataSize{0};
    void*    pCorrelationData{nullptr};
    (void)mosquitto_property_read_binary(
        pProps, MQTT_PROP_CORRELATION_DATA, &pCorrelationData, &corellationDataSize, false);
    IMqttMessage::correlationDataProps_t correlationData(
        static_cast<unsigned char*>(pCorrelationData),
        static_cast<unsigned char*>(pCorrelationData) + corellationDataSize);

    dispatchMessage(MqttMessageFactory::create(pMsg->topic,
                                               pMsg->payload,
                                               pMsg->payloadlen,
                                               IMqttMessage::intToQos(pMsg->qos),
                                               pMsg->retain,
                                               userProps,
                                               correlationData,
                                               pMsg->mid));
}

void
MosquittoClient::onSubscribeCb(struct mosquitto*         pClient,
                               int                       messageId,
                               int                       grantedQosCount,
                               const int*                pGrantedQos,
                               const mosquitto_property* pProps)
{
    (void)pClient;
    (void)messageId;
    (void)pProps;
    for (int i{0}; i < grantedQosCount; i++) {
        cbs.log->Log(LogLevel::Debug, "Mosquitto Subscribed with QOS: " + to_string(*(pGrantedQos + i)));
    }
    cbs.msg->OnSubscribe();
}

void
MosquittoClient::onUnSubscribeCb(struct mosquitto* pClient, int messageId, const mosquitto_property* pProps)
{
    (void)pClient;
    (void)messageId;
    (void)pProps;
    cbs.log->Log(LogLevel::Debug, "Mosquitto UnSubscribed");
    cbs.msg->OnUnSubscribe();
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
    cbs.log->Log(LogLevel::Info, "Connecting to broker async: [" + address + "]:" + to_string(port));
    int rc{mosquitto_connect_async(pClient, address.c_str(), port, MOSQUITTO_KEEP_ALIVE_INTERVAL)};
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
MosquittoClient::Subscribe(string const& topic, IMqttMessage::QOS qos, bool getRetained)
{
    int options{mqtt5_sub_options::MQTT_SUB_OPT_NO_LOCAL};
    if (getRetained == false) {
        options |= mqtt5_sub_options::MQTT_SUB_OPT_SEND_RETAIN_NEVER;
    }
    switch (mosquitto_subscribe_v5(pClient, NULL, topic.c_str(), IMqttMessage::qosToInt(qos), options, NULL)) {
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
MosquittoClient::UnSubscribe(string const& topic)
{
    cbs.log->Log(LogLevel::Trace, "Unsubscribing from topic: \"" + topic + "\"");
    switch (mosquitto_unsubscribe_v5(pClient, NULL, topic.c_str(), NULL)) {
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
MosquittoClient::Publish(upMqttMessage_t mqttMsg)
{
    cbs.log->Log(LogLevel::Trace, "Publishing to topic: \"" + mqttMsg->getTopic() + "\"");

    auto propertiesOkay{true};
    auto status{IMqttClient::RetCodes::ERROR_PERMANENT};
    /* Note: freed at the end of this function */
    mosquitto_property* pProps{nullptr};
    for (auto const& prop : mqttMsg->getUserProps()) {
        if (MOSQ_ERR_SUCCESS != mosquitto_property_add_string_pair(
                                    &pProps, MQTT_PROP_USER_PROPERTY, prop.first.c_str(), prop.second.c_str())) {
            cbs.log->Log(LogLevel::Error, "Invalid MQTT user property - ignoring message");
            propertiesOkay = false;
            break;
        }
    }

    if (MOSQ_ERR_SUCCESS != mosquitto_property_add_binary(&pProps,
                                                          MQTT_PROP_CORRELATION_DATA,
                                                          mqttMsg->getCorrelationDataProps().data(),
                                                          mqttMsg->getCorrelationDataProps().size())) {
        cbs.log->Log(LogLevel::Error, "invalid MQTT correlation data property - ignoring message");
        propertiesOkay = false;
    }

    if (propertiesOkay) {
        switch (mosquitto_publish_v5(pClient,
                                     NULL,
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
        cbs.log->Log(LogLevel::Error, "Publish failed - will not retry");
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
MqttClientFactory::create(std::string address, int port, std::string clientId, MqttClientCallbacks const& cbs)
{
    return unique_ptr<MosquittoClient>(new MosquittoClient(address, port, clientId, cbs));
}

}  // namespace mqttclient