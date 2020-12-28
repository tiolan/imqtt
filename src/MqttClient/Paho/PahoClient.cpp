/**
 * @file PahoClient.cpp
 * @author Timo Lange
 * @brief
 * @date 2020
 * @copyright Timo Lange
 */

#include "PahoClient.h"

using namespace std;

namespace mqttclient {
std::atomic_uint PahoClient::counter{0ul};
std::string      PahoClient::libVersion;
std::mutex       PahoClient::libMutex;

PahoClient::PahoClient(IMqttClient::InitializeParameters const& parameters)
  : params(parameters)
{
    setCallbacks(params.callbackProvider);
    bool initError{false};
    {
        lock_guard<mutex> lock(libMutex);
        // Init lib, if nobody ever did
        if (counter.fetch_add(1) == 0) {
            cbs.log->Log(LogLevel::Info, "Initializing paho lib");
            MQTTAsync_init_options initOptions MQTTAsync_init_options_initializer;
            /*For now let paho init openssl*/
            initOptions.do_openssl_init = 1;
            MQTTAsync_global_init(&initOptions);
            /*for now trace everything*/
            MQTTAsync_setTraceLevel(MQTTASYNC_TRACE_MAXIMUM);
            libVersion = "libpaho " + string(MQTTAsync_getVersionInfo()[1].value);
        };
    }  // unlock mutex, from here everything is instance specific
    cbs.log->Log(LogLevel::Info, "Initializing paho instance");
    auto brokerAddress{params.hostAddress + ":" + to_string(params.port)};
    cbs.log->Log(LogLevel::Info, "Broker-Address: " + brokerAddress);
    MQTTAsync_createOptions createOptions MQTTAsync_createOptions_initializer5;
    initError |= MQTTASYNC_SUCCESS != MQTTAsync_createWithOptions(&pClient,
                                                                  brokerAddress.c_str(),
                                                                  params.clientId.c_str(),
                                                                  MQTTCLIENT_PERSISTENCE_NONE,
                                                                  NULL,
                                                                  &createOptions);

    initError |=
        MQTTASYNC_SUCCESS !=
        MQTTAsync_setCallbacks(
            pClient,
            this,
            [](void* pThis, char*) {
                static_cast<PahoClient*>(pThis)->cbs.log->Log(LogLevel::Warning,
                                                              "Paho connection status changed: Disconnected");
                static_cast<PahoClient*>(pThis)->cbs.con->OnConnectionStatusChanged(ConnectionStatus::Disconnected);
            },
            [](void* pThis, char* topicName, int topicLen, MQTTAsync_message* message) -> int {
                return static_cast<PahoClient*>(pThis)->onMessageCb(topicName, topicLen, message);
            },
            NULL);
    initError |=
        MQTTASYNC_SUCCESS !=
        MQTTAsync_setDisconnected(pClient, this, [](void* pThis, MQTTProperties*, MQTTReasonCodes reason) {
            static_cast<PahoClient*>(pThis)->cbs.log->Log(
                LogLevel::Warning,
                "Paho connection status changed: Disconnected (" + string(MQTTReasonCode_toString(reason)) + (")"));
            static_cast<PahoClient*>(pThis)->cbs.con->OnConnectionStatusChanged(ConnectionStatus::Disconnected);
        });
    initError |=
        MQTTASYNC_SUCCESS != MQTTAsync_setConnected(pClient, this, [](void* pThis, char*) {
            static_cast<PahoClient*>(pThis)->cbs.log->Log(LogLevel::Info, "Paho connection status changed: Connected");
            static_cast<PahoClient*>(pThis)->cbs.con->OnConnectionStatusChanged(ConnectionStatus::Connected);
        });
    if (initError) {
        throw runtime_error("Failed to initialize Paho instance");
    }
}

PahoClient::~PahoClient() noexcept
{
    cbs.log->Log(LogLevel::Info, "Deinitializing paho instance");
    Disconnect();
    MQTTAsync_destroy(&pClient);
}

void
PahoClient::onSuccessCb(MQTTAsync_successData5* data)
{
    cbs.log->Log(LogLevel::Trace, "Success: MQTT reason code: " + string(MQTTReasonCode_toString(data->reasonCode)));
}

void
PahoClient::onFailureCb(MQTTAsync_failureData5* data)
{
    cbs.log->Log(LogLevel::Trace, "Failure: MQTT reason code: " + string(MQTTReasonCode_toString(data->reasonCode)));
    cbs.log->Log(LogLevel::Trace, "Failure: Paho return code: " + string(MQTTAsync_strerror(data->code)));
    if (data->message) {
        cbs.log->Log(LogLevel::Trace, "Failure: Paho reason: " + string(data->message));
    }
}

int
PahoClient::onMessageCb(char* pTopic, int topicLen, MQTTAsync_message* msg) const
{
    cbs.log->Log(LogLevel::Debug, "Paho received message");

    bool acceptMsg{true};

    auto internalMessage{MqttMessageFactory::create(
        string(pTopic, topicLen),
        IMqttMessage::payload_t(static_cast<IMqttMessage::payloadRaw_t*>(msg->payload),
                                static_cast<IMqttMessage::payloadRaw_t*>(msg->payload) + msg->payloadlen),
        IMqttMessage::intToQos(msg->qos),
        msg->retained == 0 ? false : true)};

    for (auto prop{0}; prop < msg->properties.count; prop++) {
        switch (msg->properties.array[prop].identifier) {
        case MQTTPROPERTY_CODE_USER_PROPERTY: {
            auto key{string(msg->properties.array[prop].value.data.data, msg->properties.array[prop].value.data.len)};
            auto value{
                string(msg->properties.array[prop].value.value.data, msg->properties.array[prop].value.value.len)};
            if (!internalMessage->userProps.insert(make_pair(key, value)).second) {
                cbs.log->Log(LogLevel::Error, "Received invalid user properties - ignoring");
            }
        } break;
        case MQTTPROPERTY_CODE_CORRELATION_DATA: {
            auto pData{msg->properties.array[prop].value.data.data};
            auto dataLen{msg->properties.array[prop].value.data.len};
            internalMessage->correlationDataProps = IMqttMessage::correlationDataProps_t(pData, pData + dataLen);
        } break;
        case MQTTPROPERTY_CODE_RESPONSE_TOPIC: {
            internalMessage->responseTopic =
                string(msg->properties.array[prop].value.data.data, msg->properties.array[prop].value.data.len);
        } break;
        case MQTTPROPERTY_CODE_PAYLOAD_FORMAT_INDICATOR: {
            if (msg->properties.array[prop].value.byte == 1)
                internalMessage->payloadFormatIndicator = IMqttMessage::FormatIndicator::UTF8;
        } break;
        case MQTTPROPERTY_CODE_CONTENT_TYPE: {
            internalMessage->payloadContentType =
                string(msg->properties.array[prop].value.data.data, msg->properties.array[prop].value.data.len);
        } break;
        default:
            break;
        }
    }

    cbs.msg->OnMqttMessage(move(internalMessage));

    if (acceptMsg) {
        MQTTAsync_freeMessage(&msg);
        MQTTAsync_free(pTopic);
        return 1;
    }
    return 0;
}

string
PahoClient::GetLibVersion(void) const noexcept
{
    return libVersion;
}

void
PahoClient::ConnectAsync(void)
{
    cbs.log->Log(LogLevel::Info, "Start connecting to broker");
    MQTTAsync_connectOptions connectOptions MQTTAsync_connectOptions_initializer5;
    connectOptions.keepAliveInterval  = params.keepAliveInterval;
    connectOptions.automaticReconnect = params.autoReconnect ? 1 : 0;
    connectOptions.cleanstart         = params.cleanSession ? 1 : 0;
    connectOptions.maxRetryInterval   = params.reconnectDelayMax;
    connectOptions.minRetryInterval =
        params.reconnectDelayMin +
        uniform_int_distribution<int>(params.reconnectDelayMinLower, params.reconnectDelayMinUpper)(rndGenerator);
    cbs.log->Log(LogLevel::Debug,
                 "Reconnect delay min: " + to_string(connectOptions.minRetryInterval) + "," +
                     " max: " + to_string(connectOptions.maxRetryInterval));
    connectOptions.context    = this;
    connectOptions.onSuccess5 = [](void* pThis, MQTTAsync_successData5* data) {
        static_cast<PahoClient*>(pThis)->onSuccessCb(data);
    };
    connectOptions.onFailure5 = [](void* pThis, MQTTAsync_failureData5* data) {
        static_cast<PahoClient*>(pThis)->onFailureCb(data);
    };
    if (!params.mqttUsername.empty()) {
        connectOptions.username = params.mqttUsername.c_str();
        connectOptions.password = params.mqttPassword.c_str();
    }
    if (!params.httpProxy.empty()) {
        connectOptions.httpProxy = params.httpProxy.c_str();
    }
    if (!params.httpsProxy.empty()) {
        connectOptions.httpsProxy = params.httpsProxy.c_str();
    }
#ifdef IMQTT_WITH_TLS
    MQTTAsync_SSLOptions sslOptions MQTTAsync_SSLOptions_initializer;
    connectOptions.ssl             = &sslOptions;
    connectOptions.ssl->trustStore = params.caFilePath.empty() ? nullptr : params.caFilePath.c_str();
    connectOptions.ssl->CApath     = params.caDirPath.empty() ? nullptr : params.caDirPath.c_str();
    connectOptions.ssl->keyStore   = params.clientCertFilePath.empty() ? nullptr : params.clientCertFilePath.c_str();
    connectOptions.ssl->privateKey = params.privateKeyFilePath.empty() ? nullptr : params.privateKeyFilePath.c_str();
    connectOptions.ssl->disableDefaultTrustStore = params.disableDefaultCaStore ? 1 : 0;
#ifdef IMQTT_EXPERIMENTAL
    connectOptions.ssl->clientCertString = params.clientCert.empty() ? nullptr : params.clientCert.c_str();
    connectOptions.ssl->privateKeyString = params.privateKey.empty() ? nullptr : params.privateKey.c_str();
#endif
    connectOptions.ssl->privateKeyPassword =
        params.privateKeyPassword.empty() ? nullptr : params.privateKeyPassword.c_str();
    connectOptions.ssl->verify               = 1;
    connectOptions.ssl->enableServerCertAuth = 1;
    connectOptions.ssl->ssl_error_context    = this;
    connectOptions.ssl->ssl_error_cb         = [](const char* str, size_t len, void* pThis) -> int {
        static_cast<PahoClient*>(pThis)->cbs.log->Log(LogLevel::Error, string(str, len));
        return 0;
    };
#endif
    auto rc{MQTTAsync_connect(pClient, &connectOptions)};
    if (rc == MQTTASYNC_SUCCESS) {
    }
    else if (rc > MQTTASYNC_SUCCESS) {
        cbs.log->Log(LogLevel::Error, "Paho connect error, MQTT failure code: " + to_string(rc));
    }
    else if (rc < MQTTASYNC_SUCCESS) {
        cbs.log->Log(LogLevel::Fatal, "MQTTAsync_connect could not be called: " + string(MQTTAsync_strerror(rc)));
        throw runtime_error("MQTTAsync_connect could not be called");
    }
}

void
PahoClient::Disconnect(void)
{
    cbs.log->Log(LogLevel::Info, "Disconnecting from broker");
    MQTTAsync_disconnectOptions disconnectOptions MQTTAsync_disconnectOptions_initializer5;
    disconnectOptions.context    = this;
    disconnectOptions.onSuccess5 = [](void* pThis, MQTTAsync_successData5* data) {
        static_cast<PahoClient*>(pThis)->onSuccessCb(data);
    };
    disconnectOptions.onFailure5 = [](void* pThis, MQTTAsync_failureData5* data) {
        static_cast<PahoClient*>(pThis)->onFailureCb(data);
    };

    auto rc{MQTTAsync_disconnect(pClient, &disconnectOptions)};
    cbs.log->Log(LogLevel::Info, "Paho client disconnected: " + string(MQTTAsync_strerror(rc)));
}

IMqttClient::RetCodes
PahoClient::SubscribeAsync(string const& topic, IMqttMessage::QOS qos, int* token, bool getRetained)
{
    cbs.log->Log(LogLevel::Trace, "Subscribing to topic: \"" + topic + "\"");
    MQTTAsync_callOptions callOptions MQTTAsync_callOptions_initializer;
    callOptions.context    = this;
    callOptions.onSuccess5 = [](void* pThis, MQTTAsync_successData5* data) {
        static_cast<PahoClient*>(pThis)->cbs.log->Log(LogLevel::Debug, "Paho Subscribed");
        static_cast<PahoClient*>(pThis)->onSuccessCb(data);
        static_cast<PahoClient*>(pThis)->cbs.msg->OnSubscribe(data->token);
    };
    callOptions.onFailure5 = [](void* pThis, MQTTAsync_failureData5* data) {
        static_cast<PahoClient*>(pThis)->onFailureCb(data);
    };
    callOptions.subscribeOptions                   = MQTTSubscribe_options_initializer;
    callOptions.subscribeOptions.noLocal           = params.allowLocalTopics ? 0 : 1;
    callOptions.subscribeOptions.retainAsPublished = getRetained ? 0 : 2;

    switch (MQTTAsync_subscribe(pClient, topic.c_str(), IMqttMessage::qosToInt(qos), &callOptions)) {
    case MQTTASYNC_SUCCESS:
        if (token) {
            *token = callOptions.token;
        }
        return RetCodes::OKAY;
        break;
    case MQTTASYNC_DISCONNECTED:
        return RetCodes::ERROR_TEMPORARY;
        break;
    default:
        break;
    }
    return RetCodes::ERROR_PERMANENT;
}

IMqttClient::RetCodes
PahoClient::UnSubscribeAsync(string const& topic, int* token)
{
    cbs.log->Log(LogLevel::Trace, "Unsubscribing from topic: \"" + topic + "\"");
    MQTTAsync_callOptions callOptions MQTTAsync_callOptions_initializer;
    callOptions.context    = this;
    callOptions.onSuccess5 = [](void* pThis, MQTTAsync_successData5* data) {
        static_cast<PahoClient*>(pThis)->cbs.log->Log(LogLevel::Debug, "Paho UnSubscribed");
        static_cast<PahoClient*>(pThis)->onSuccessCb(data);
        static_cast<PahoClient*>(pThis)->cbs.msg->OnUnSubscribe(data->token);
    };
    callOptions.onFailure5 = [](void* pThis, MQTTAsync_failureData5* data) {
        static_cast<PahoClient*>(pThis)->onFailureCb(data);
    };

    switch (MQTTAsync_unsubscribe(pClient, topic.c_str(), &callOptions)) {
    case MQTTASYNC_SUCCESS:
        if (token) {
            *token = callOptions.token;
        }
        return RetCodes::OKAY;
        break;
    case MQTTASYNC_DISCONNECTED:
        return RetCodes::ERROR_TEMPORARY;
        break;
    default:
        break;
    }
    return RetCodes::ERROR_PERMANENT;
}

IMqttClient::RetCodes
PahoClient::PublishAsync(upMqttMessage_t mqttMsg, int* token)
{
    cbs.log->Log(LogLevel::Trace, "Publishing to topic: \"" + mqttMsg->getTopic() + "\"");
    auto                              propertiesOkay{true};
    MQTTAsync_callOptions callOptions MQTTAsync_callOptions_initializer;
    callOptions.context    = this;
    callOptions.onFailure5 = [](void* pThis, MQTTAsync_failureData5* data) {
        static_cast<PahoClient*>(pThis)->onFailureCb(data);
    };
    callOptions.onSuccess5 = [](void* pThis, MQTTAsync_successData5* data) {
        static_cast<PahoClient*>(pThis)->cbs.log->Log(LogLevel::Debug, "Paho Published");
        static_cast<PahoClient*>(pThis)->onSuccessCb(data);
        static_cast<PahoClient*>(pThis)->cbs.msg->OnPublish(data->token);
    };

    MQTTAsync_message msg MQTTAsync_message_initializer;
    msg.payload    = const_cast<void*>(reinterpret_cast<const void*>(mqttMsg->getPayload().data()));
    msg.payloadlen = static_cast<int>(mqttMsg->getPayload().size());
    msg.msgid      = mqttMsg->messageId > 0 ? mqttMsg->messageId : msg.msgid;
    msg.qos        = IMqttMessage::qosToInt(mqttMsg->getQos());
    msg.retained   = mqttMsg->getRetained() ? 1 : 0;

    for (auto const& userProp : mqttMsg->userProps) {
        MQTTProperty prop;
        prop.identifier       = MQTTPROPERTY_CODE_USER_PROPERTY;
        prop.value.data.data  = const_cast<char*>(userProp.first.c_str());
        prop.value.data.len   = static_cast<int>(userProp.first.size());
        prop.value.value.data = const_cast<char*>(userProp.second.c_str());
        prop.value.value.len  = static_cast<int>(userProp.second.size());

        if (MQTTASYNC_SUCCESS != MQTTProperties_add(&msg.properties, &prop)) {
            cbs.log->Log(LogLevel::Error, "Was not able to add user property, ignoring message");
            propertiesOkay = false;
        }
    }
    {
        MQTTProperty prop;
        prop.identifier      = MQTTPROPERTY_CODE_RESPONSE_TOPIC;
        prop.value.data.data = const_cast<char*>(mqttMsg->responseTopic.c_str());
        prop.value.data.len  = static_cast<int>(mqttMsg->responseTopic.size());
        if (MQTTASYNC_SUCCESS != MQTTProperties_add(&msg.properties, &prop)) {
            cbs.log->Log(LogLevel::Error, "Was not able to add reponse topic, ignoring message");
            propertiesOkay = false;
        }
    }
    {
        MQTTProperty prop;
        prop.identifier      = MQTTPROPERTY_CODE_CORRELATION_DATA;
        prop.value.data.data = reinterpret_cast<char*>(mqttMsg->correlationDataProps.data());
        prop.value.data.len  = static_cast<int>(mqttMsg->correlationDataProps.size());
        if (MQTTASYNC_SUCCESS != MQTTProperties_add(&msg.properties, &prop)) {
            cbs.log->Log(LogLevel::Error, "Was not able to add correlation data, ignoring message");
            propertiesOkay = false;
        }
    }
    {
        MQTTProperty prop;
        prop.identifier = MQTTPROPERTY_CODE_PAYLOAD_FORMAT_INDICATOR;
        prop.value.byte = mqttMsg->payloadFormatIndicator == IMqttMessage::FormatIndicator::UTF8 ? 1u : 0u;
        if (MQTTASYNC_SUCCESS != MQTTProperties_add(&msg.properties, &prop)) {
            cbs.log->Log(LogLevel::Error, "Was not able to add format indicator, ignoring message");
            propertiesOkay = false;
        }
    }
    {
        MQTTProperty prop;
        prop.identifier      = MQTTPROPERTY_CODE_CONTENT_TYPE;
        prop.value.data.data = const_cast<char*>(mqttMsg->payloadContentType.c_str());
        prop.value.data.len  = static_cast<int>(mqttMsg->payloadContentType.size());
        if (MQTTASYNC_SUCCESS != MQTTProperties_add(&msg.properties, &prop)) {
            cbs.log->Log(LogLevel::Error, "Was not able to add content type, ignoring message");
            propertiesOkay = false;
        }
    }
    auto ret{RetCodes::ERROR_PERMANENT};
    if (propertiesOkay) {
        auto rc{MQTTAsync_sendMessage(pClient, mqttMsg->getTopic().c_str(), &msg, &callOptions)};
        switch (rc) {
        case MQTTASYNC_SUCCESS:
            ret = RetCodes::OKAY;
            if (token) {
                *token = callOptions.token;
            }
            break;
        case MQTTASYNC_DISCONNECTED:
            ret = RetCodes::ERROR_TEMPORARY;
            break;
        default:
            break;
        }
    }
    MQTTProperties_free(&msg.properties);
    return ret;
}

IMqttConnectionCallbacks::ConnectionStatus
PahoClient::GetConnectionStatus(void) const noexcept
{
    return MQTTAsync_isConnected(pClient) == 0 ? ConnectionStatus::Disconnected : ConnectionStatus::Connected;
}

unique_ptr<IMqttClient>
MqttClientFactory::create(IMqttClient::InitializeParameters const& params)
{
    return unique_ptr<PahoClient>(new PahoClient(params));
}
}  // namespace mqttclient
