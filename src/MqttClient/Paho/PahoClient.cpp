/**
 * @file PahoClient.cpp
 * @author Timo Lange
 * @brief Implementation for Paho library wrapper
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

#include "PahoClient.h"

#include <future>

using namespace std;

namespace i_mqtt_client {
once_flag PahoClient::initFlag;

PahoClient::PahoClient(InitializeParameters const&     parameters,
                       IMqttMessageCallbacks const*    msg,
                       IMqttLogCallbacks const*        log,
                       IMqttCommandCallbacks const*    cmd,
                       IMqttConnectionCallbacks const* con)
  : IMqttClient(log, cmd, msg, con)
  , params(parameters)
{
    // Init lib, if nobody ever did
    call_once(initFlag, [this] {
        logCb->Log(LogLevel::INFO, "Initializing paho lib");
        MQTTAsync_init_options initOptions MQTTAsync_init_options_initializer;
        /*For now let paho init openssl*/
        initOptions.do_openssl_init = 1;
        MQTTAsync_global_init(&initOptions);
        /*This either disables logging, or gets, what the user already set*/
        auto logInfo{IMqttLogCallbacks::InitLogMqttLib({nullptr, LogLevelLib::NONE})};
        if (logInfo.second == LogLevelLib::NONE) {
            /*disable logs*/
            MQTTAsync_setTraceCallback(nullptr);
        }
        else {
            /*user wants to log, provide callback to Paho*/
            MQTTAsync_setTraceCallback([](enum MQTTASYNC_TRACE_LEVELS lvl, char* pTxt) {
                auto logLvl{LogLevelLib::INFO};
                switch (lvl) {
                case MQTTASYNC_TRACE_LEVELS::MQTTASYNC_TRACE_MINIMUM:
                    /*fallthrough*/
                case MQTTASYNC_TRACE_LEVELS::MQTTASYNC_TRACE_MEDIUM:
                    /*fallthrough*/
                case MQTTASYNC_TRACE_LEVELS::MQTTASYNC_TRACE_MAXIMUM:
                    logLvl = LogLevelLib::TRACE;
                    break;
                case MQTTASYNC_TRACE_LEVELS::MQTTASYNC_TRACE_PROTOCOL:
                    logLvl = LogLevelLib::DEBUG;
                    break;
                case MQTTASYNC_TRACE_LEVELS::MQTTASYNC_TRACE_ERROR:
                    /*fallthrough*/
                case MQTTASYNC_TRACE_LEVELS::MQTTASYNC_TRACE_SEVERE:
                    logLvl = LogLevelLib::ERROR;
                    break;
                case MQTTASYNC_TRACE_LEVELS::MQTTASYNC_TRACE_FATAL:
                    logLvl = LogLevelLib::FATAL;
                    break;
                default:
                    break;
                }
                IMqttClientCallbacks::LogMqttLib(logLvl, string(pTxt));
            });
            /*get minimum log level*/
            switch (logInfo.second) {
            case LogLevelLib::TRACE:
                MQTTAsync_setTraceLevel(MQTTASYNC_TRACE_LEVELS::MQTTASYNC_TRACE_MAXIMUM);
                break;
            case LogLevelLib::DEBUG:
                MQTTAsync_setTraceLevel(MQTTASYNC_TRACE_LEVELS::MQTTASYNC_TRACE_PROTOCOL);
                break;
            case LogLevelLib::FATAL:
                MQTTAsync_setTraceLevel(MQTTASYNC_TRACE_LEVELS::MQTTASYNC_TRACE_FATAL);
                break;
            case LogLevelLib::NONE:
                MQTTAsync_setTraceCallback(nullptr);
                break;
            default:
                MQTTAsync_setTraceLevel(MQTTASYNC_TRACE_LEVELS::MQTTASYNC_TRACE_ERROR);
                break;
            }
        }
        IMqttClient::libVersion = "libpaho " + string(MQTTAsync_getVersionInfo()[1].value);
    });

    logCb->Log(LogLevel::INFO, "Initializing paho instance");
    auto brokerAddress{params.hostAddress + ":" + to_string(params.port)};
    logCb->Log(LogLevel::INFO, "Broker-Address: " + brokerAddress);

    if (params.reconnectDelayMinLower < 0 || params.reconnectDelayMinUpper < 0 ||
        params.reconnectDelayMinLower > params.reconnectDelayMinUpper) {
        throw runtime_error("reconnectDelay not properly set");
    }

    MQTTAsync_createOptions createOptions MQTTAsync_createOptions_initializer5;

    auto rc{MQTTASYNC_SUCCESS};
    rc = MQTTAsync_createWithOptions(
        &pClient, brokerAddress.c_str(), params.clientId.c_str(), MQTTCLIENT_PERSISTENCE_NONE, nullptr, &createOptions);
    if (MQTTASYNC_SUCCESS != rc) {
        throw runtime_error("Was not able to create paho client: " + string(MQTTAsync_strerror(rc)));
    }
    rc = MQTTAsync_setCallbacks(
        pClient,
        this,
        [](void* pThis, char*) {
            static_cast<PahoClient*>(pThis)->logCb->Log(LogLevel::WARNING, "Paho disconnected from broker");
            static_cast<PahoClient*>(pThis)->conCb->OnConnectionStatusChanged(ConnectionType::DISCONNECT,
                                                                              Mqtt5ReasonCode::SUCCESS);
        },
        [](void* pThis, char* topicName, int topicLen, MQTTAsync_message* message) {
            return static_cast<PahoClient*>(pThis)->onMessageCb(topicName, topicLen, message);
        },
        nullptr);
    if (MQTTASYNC_SUCCESS != rc) {
        throw runtime_error("Was not able to set paho callbacks: " + string(MQTTAsync_strerror(rc)));
    }
    rc = MQTTAsync_setDisconnected(pClient, this, [](void* pThis, MQTTProperties*, MQTTReasonCodes reason) {
        static_cast<PahoClient*>(pThis)->logCb->Log(
            LogLevel::WARNING, "Paho disconnected from broker, rc: " + Mqtt5ReasonCodeToStringRepr(reason).first);
        static_cast<PahoClient*>(pThis)->conCb->OnConnectionStatusChanged(ConnectionType::DISCONNECT,
                                                                          static_cast<Mqtt5ReasonCode>(reason));
    });
    if (MQTTASYNC_SUCCESS != rc) {
        throw runtime_error("Was not able to set paho disconnected callback: " + string(MQTTAsync_strerror(rc)));
    }
    rc = MQTTAsync_setConnected(pClient, this, [](void* pThis, char*) {
        static_cast<PahoClient*>(pThis)->logCb->Log(LogLevel::INFO, "Paho connected to broker");
        static_cast<PahoClient*>(pThis)->conCb->OnConnectionStatusChanged(ConnectionType::CONNECT,
                                                                          Mqtt5ReasonCode::SUCCESS);
    });
    if (MQTTASYNC_SUCCESS != rc) {
        throw runtime_error("Was not able to set paho connected callback: " + string(MQTTAsync_strerror(rc)));
    }
}

PahoClient::~PahoClient() noexcept
{
    logCb->Log(LogLevel::INFO, "Deinitializing paho instance");
    if (IsConnected()) {
        DisconnectAsync(Mqtt5ReasonCode::SUCCESS);
    }
    MQTTAsync_destroy(&pClient);
}

void
PahoClient::printDetailsOnSuccess(string const& details, MQTTAsync_successData5 const* data) const
{
    logCb->Log(LogLevel::DEBUG,
               details + ": okay for token: " + to_string(data->token) +
                   ", MQTT5 rc: " + string(MQTTReasonCode_toString(data->reasonCode)));
}

void
PahoClient::printDetailsOnFailure(string const& details, MQTTAsync_failureData5 const* data) const
{
    logCb->Log(LogLevel::ERROR,
               details + ": failed for token: " + to_string(data->token) +
                   ", MQTT5 rc: " + string(MQTTReasonCode_toString(data->reasonCode)) +
                   ", Paho rc: " + string(MQTTAsync_strerror(data->code)));
    if (data->message) {
        logCb->Log(
            LogLevel::ERROR,
            details + ": failed for token: " + to_string(data->token) + ", Paho description: " + string(data->message));
    }
}

int
PahoClient::onMessageCb(char* pTopic, int topicLen, MQTTAsync_message* msg) const
{
    logCb->Log(LogLevel::TRACE, "Paho received message");

    bool acceptMsg{true};

    auto internalMessage{MqttMessageFactory::Create(
        string(pTopic, topicLen),
        IMqttMessage::payload_t(static_cast<IMqttMessage::payloadRaw_t*>(msg->payload),
                                static_cast<IMqttMessage::payloadRaw_t*>(msg->payload) + msg->payloadlen),
        static_cast<IMqttMessage::QOS>(msg->qos),
        msg->retained != 0)};

    internalMessage->messageId = msg->msgid;

    for (auto prop{0}; prop < msg->properties.count; prop++) {
        switch (msg->properties.array[prop].identifier) {
        case MQTTPROPERTY_CODE_USER_PROPERTY: {
            auto key{string(msg->properties.array[prop].value.data.data, msg->properties.array[prop].value.data.len)};
            auto value{
                string(msg->properties.array[prop].value.value.data, msg->properties.array[prop].value.value.len)};
            if (!internalMessage->userProps.insert(make_pair(key, value)).second) {
                logCb->Log(LogLevel::ERROR, "Received invalid user properties - ignoring");
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

    msgCb->OnMqttMessage(move(internalMessage));

    if (acceptMsg) {
        MQTTAsync_freeMessage(&msg);
        MQTTAsync_free(pTopic);
        return 1;
    }
    return 0;
}

ReasonCode
PahoClient::ConnectAsync(void)
{
    logCb->Log(LogLevel::INFO, "Start connecting to broker");
    MQTTAsync_connectOptions connectOptions MQTTAsync_connectOptions_initializer5;
    connectOptions.keepAliveInterval  = params.keepAliveInterval;
    connectOptions.automaticReconnect = params.autoReconnect ? 1 : 0;
    connectOptions.cleanstart         = params.cleanSession ? 1 : 0;
    connectOptions.maxRetryInterval   = params.reconnectDelayMax;
    connectOptions.minRetryInterval =
        params.reconnectDelayMin +
        uniform_int_distribution<int>(params.reconnectDelayMinLower, params.reconnectDelayMinUpper)(rndGenerator);
    logCb->Log(LogLevel::DEBUG,
               "Reconnect delay min: " + to_string(connectOptions.minRetryInterval) + "," +
                   " max: " + to_string(connectOptions.maxRetryInterval));

    static auto ctx{Context(this, nullptr)};
    auto        rcPromise{promise<int>()};
    ctx.pContext              = &rcPromise;
    connectOptions.context    = &ctx;
    connectOptions.onSuccess5 = [](void* pCtx, MQTTAsync_successData5* data) {
        static_cast<Context*>(pCtx)->pThis->printDetailsOnSuccess("MQTTAsync_connect", data);
        try {
            static_cast<promise<int>*>(static_cast<Context*>(pCtx)->pContext)->set_value(MQTTASYNC_SUCCESS);
        }
        catch (future_error const&) {
            /*Nothing to be done here*/
        }
    };
    connectOptions.onFailure5 = [](void* pCtx, MQTTAsync_failureData5* data) {
        /*This callback sometimes (e.g. with invalid broker url) is called multiple times, so we have to catch here*/
        static_cast<Context*>(pCtx)->pThis->printDetailsOnFailure("MQTTAsync_connect", data);
        try {
            static_cast<promise<int>*>(static_cast<Context*>(pCtx)->pContext)->set_value(data->code);
        }
        catch (future_error const&) {
            /*Nothing to be done here*/
        }
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
        static_cast<PahoClient*>(pThis)->logCb->Log(LogLevel::ERROR, string(str, len));
        return 0;
    };
#endif
    auto reason{MqttReasonCode::ACCEPTED};
    if ((reason = static_cast<MqttReasonCode>(MQTTAsync_connect(pClient, &connectOptions))) >
        MqttReasonCode::ACCEPTED) {
        logCb->Log(LogLevel::ERROR,
                   "MQTTAsync_connect returned MQTT error: " + MqttReasonCodeToStringRepr(reason).first);
    }
    /*use rc from the callbacks, wait forever because it is assumed one of the callbacks is always called*/
    return pahoRcToReasonCode(rcPromise.get_future().get(), "MQTTAsync_connect");
}

ReasonCode
PahoClient::DisconnectAsync(Mqtt5ReasonCode rc)
{
    logCb->Log(LogLevel::INFO, "Disconnecting from broker");
    MQTTAsync_disconnectOptions disconnectOptions MQTTAsync_disconnectOptions_initializer5;
    disconnectOptions.timeout    = 10 /*ms*/;
    disconnectOptions.reasonCode = static_cast<MQTTReasonCodes>(rc);
    disconnectOptions.context    = this;
    disconnectOptions.onSuccess5 = [](void* pThis, MQTTAsync_successData5* data) {
        static_cast<PahoClient*>(pThis)->printDetailsOnSuccess("MQTTAsync_disconnect", data);
        /*user callback is invoked by Paho when the connection drops*/
    };
    disconnectOptions.onFailure5 = [](void* pThis, MQTTAsync_failureData5* data) {
        static_cast<PahoClient*>(pThis)->printDetailsOnFailure("MQTTAsync_disconnect", data);
    };
    /* TODO: Currently there is a race-condition between the call to DisconnectAsync and IsConnected in the Destructor
     * maybe wait for callbacks to happen, same as in Connect */
    return pahoRcToReasonCode(MQTTAsync_disconnect(pClient, &disconnectOptions), "MQTTAsync_disconnect");
}

ReasonCode
PahoClient::SubscribeAsync(string const& topic, IMqttMessage::QOS qos, int* token, bool getRetained)
{
    logCb->Log(LogLevel::TRACE, "Subscribing to topic: \"" + topic + "\"");
    MQTTAsync_callOptions callOptions MQTTAsync_callOptions_initializer;
    callOptions.context    = this;
    callOptions.onSuccess5 = [](void* pThis, MQTTAsync_successData5* data) {
        static_cast<PahoClient*>(pThis)->printDetailsOnSuccess("MQTTAsync_subscribe", data);
        static_cast<PahoClient*>(pThis)->cmdCb->OnSubscribe(data->token);
    };
    callOptions.onFailure5 = [](void* pThis, MQTTAsync_failureData5* data) {
        static_cast<PahoClient*>(pThis)->printDetailsOnFailure("MQTTAsync_subscribe", data);
        // TODO: call the user callback with Mqtt5ReasonCode
    };
    callOptions.subscribeOptions                   = MQTTSubscribe_options_initializer;
    callOptions.subscribeOptions.noLocal           = params.allowLocalTopics ? 0 : 1;
    callOptions.subscribeOptions.retainAsPublished = getRetained ? 0 : 2;

    auto status{pahoRcToReasonCode(MQTTAsync_subscribe(pClient, topic.c_str(), static_cast<int>(qos), &callOptions),
                                   "MQTTAsync_subscribe")};
    if (token) {
        *token = callOptions.token;
    }
    return status;
}

ReasonCode
PahoClient::UnSubscribeAsync(string const& topic, int* token)
{
    logCb->Log(LogLevel::TRACE, "Unsubscribing from topic: \"" + topic + "\"");
    MQTTAsync_callOptions callOptions MQTTAsync_callOptions_initializer;
    callOptions.context    = this;
    callOptions.onSuccess5 = [](void* pThis, MQTTAsync_successData5* data) {
        static_cast<PahoClient*>(pThis)->printDetailsOnSuccess("MQTTAsync_unsubscribe", data);
        static_cast<PahoClient*>(pThis)->cmdCb->OnUnSubscribe(data->token);
    };
    callOptions.onFailure5 = [](void* pThis, MQTTAsync_failureData5* data) {
        static_cast<PahoClient*>(pThis)->printDetailsOnFailure("MQTTAsync_unsubscribe", data);
        // TODO: call the user callback with Mqtt5ReasonCode
    };

    auto status{
        pahoRcToReasonCode(MQTTAsync_unsubscribe(pClient, topic.c_str(), &callOptions), "MQTTAsync_unsubscribe")};
    if (token) {
        *token = callOptions.token;
    }
    return status;
}

ReasonCode
PahoClient::PublishAsync(upMqttMessage_t mqttMsg, int* token)
{
    logCb->Log(LogLevel::DEBUG, "Publishing to topic: \"" + mqttMsg->topic + "\"");
    MQTTAsync_callOptions callOptions MQTTAsync_callOptions_initializer;
    callOptions.context    = this;
    callOptions.onFailure5 = [](void* pThis, MQTTAsync_failureData5* data) {
        static_cast<PahoClient*>(pThis)->printDetailsOnFailure("MQTTAsync_sendMessage", data);
        static_cast<PahoClient*>(pThis)->cmdCb->OnPublish(data->token, static_cast<Mqtt5ReasonCode>(data->reasonCode));
    };
    callOptions.onSuccess5 = [](void* pThis, MQTTAsync_successData5* data) {
        static_cast<PahoClient*>(pThis)->printDetailsOnSuccess("MQTTAsync_sendMessage", data);
        static_cast<PahoClient*>(pThis)->logCb->Log(LogLevel::DEBUG,
                                                    "Paho Publish finished for token: " + to_string(data->token));
        static_cast<PahoClient*>(pThis)->cmdCb->OnPublish(data->token, static_cast<Mqtt5ReasonCode>(data->reasonCode));
    };

    MQTTAsync_message msg MQTTAsync_message_initializer;
    msg.payload    = const_cast<void*>(reinterpret_cast<const void*>(mqttMsg->payload.data()));
    msg.payloadlen = static_cast<int>(mqttMsg->payload.size());
    msg.msgid      = mqttMsg->messageId > 0 ? mqttMsg->messageId : msg.msgid;
    msg.qos        = static_cast<int>(mqttMsg->qos);
    msg.retained   = mqttMsg->retain ? 1 : 0;

    auto propertiesOkay{true};
    for (auto const& userProp : mqttMsg->userProps) {
        MQTTProperty prop;
        prop.identifier       = MQTTPROPERTY_CODE_USER_PROPERTY;
        prop.value.data.data  = const_cast<char*>(userProp.first.c_str());
        prop.value.data.len   = static_cast<int>(userProp.first.size());
        prop.value.value.data = const_cast<char*>(userProp.second.c_str());
        prop.value.value.len  = static_cast<int>(userProp.second.size());

        if (MQTTASYNC_SUCCESS != MQTTProperties_add(&msg.properties, &prop)) {
            logCb->Log(LogLevel::ERROR, "Was not able to add user property, ignoring message");
            propertiesOkay = false;
        }
    }
    {
        MQTTProperty prop;
        prop.identifier      = MQTTPROPERTY_CODE_RESPONSE_TOPIC;
        prop.value.data.data = const_cast<char*>(mqttMsg->responseTopic.c_str());
        prop.value.data.len  = static_cast<int>(mqttMsg->responseTopic.size());
        if (MQTTASYNC_SUCCESS != MQTTProperties_add(&msg.properties, &prop)) {
            logCb->Log(LogLevel::ERROR, "Was not able to add reponse topic, ignoring message");
            propertiesOkay = false;
        }
    }
    {
        MQTTProperty prop;
        prop.identifier      = MQTTPROPERTY_CODE_CORRELATION_DATA;
        prop.value.data.data = reinterpret_cast<char*>(mqttMsg->correlationDataProps.data());
        prop.value.data.len  = static_cast<int>(mqttMsg->correlationDataProps.size());
        if (MQTTASYNC_SUCCESS != MQTTProperties_add(&msg.properties, &prop)) {
            logCb->Log(LogLevel::ERROR, "Was not able to add correlation data, ignoring message");
            propertiesOkay = false;
        }
    }
    {
        MQTTProperty prop;
        prop.identifier = MQTTPROPERTY_CODE_PAYLOAD_FORMAT_INDICATOR;
        prop.value.byte = mqttMsg->payloadFormatIndicator == IMqttMessage::FormatIndicator::UTF8 ? 1U : 0U;
        if (MQTTASYNC_SUCCESS != MQTTProperties_add(&msg.properties, &prop)) {
            logCb->Log(LogLevel::ERROR, "Was not able to add format indicator, ignoring message");
            propertiesOkay = false;
        }
    }
    {
        MQTTProperty prop;
        prop.identifier      = MQTTPROPERTY_CODE_CONTENT_TYPE;
        prop.value.data.data = const_cast<char*>(mqttMsg->payloadContentType.c_str());
        prop.value.data.len  = static_cast<int>(mqttMsg->payloadContentType.size());
        if (MQTTASYNC_SUCCESS != MQTTProperties_add(&msg.properties, &prop)) {
            logCb->Log(LogLevel::ERROR, "Was not able to add content type, ignoring message");
            propertiesOkay = false;
        }
    }

    auto status{ReasonCode::ERROR_GENERAL};
    if (propertiesOkay) {
        status = pahoRcToReasonCode(MQTTAsync_sendMessage(pClient, mqttMsg->topic.c_str(), &msg, &callOptions),
                                    "MQTTAsync_sendMessage");
        if (status == ReasonCode::OKAY && token) {
            *token = callOptions.token;
        }
    }
    MQTTProperties_free(&msg.properties);
    return status;
}

bool
PahoClient::IsConnected(void) const noexcept
{
    return MQTTAsync_isConnected(pClient) != 0;
}

ReasonCode
PahoClient::pahoRcToReasonCode(int rc, string const& details) const
{
    auto status{ReasonCode::ERROR_GENERAL};
    auto logLvl{LogLevel::ERROR};
    switch (rc) {
    case MQTTASYNC_SUCCESS:
        logLvl = LogLevel::DEBUG;
        status = ReasonCode::OKAY;
        break;
    case MQTTASYNC_DISCONNECTED:
        logLvl = LogLevel::WARNING;
        status = ReasonCode::ERROR_NO_CONNECTION;
        break;
    default:
        break;
    }
    logCb->Log(logLvl,
               details + ": " + ReasonCodeToStringRepr(status).first + ", Paho: " + string(MQTTAsync_strerror(rc)));
    return status;
}

unique_ptr<IMqttClient>
MqttClientFactory::Create(IMqttClient::InitializeParameters const& params,
                          IMqttMessageCallbacks const*             msg,
                          IMqttLogCallbacks const*                 log,
                          IMqttCommandCallbacks const*             cmd,
                          IMqttConnectionCallbacks const*          con)
{
    return unique_ptr<PahoClient>(new PahoClient(params, msg, log, cmd, con));
}
}  // namespace i_mqtt_client
