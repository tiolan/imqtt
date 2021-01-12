/**
 * @file Main.cpp
 * @author Timo Lange
 * @brief A sample app showing how to use IMqtt
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

#include <chrono>
#include <condition_variable>
#include <csignal>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#ifdef IMQTT_EXPERIMENTAL
#include "../../cert/certs.h"
#endif

#include "IDispatchQueue.h"
#include "IMqttClient.h"

using namespace std;
using namespace i_mqtt_client;
using namespace chrono;
using namespace this_thread;
using namespace placeholders;

class Sample final : public IMqttClientCallbacks {
private:
    static bool               exitRun;
    static mutex              exitRunMutex;
    static condition_variable interrupt;

    string const  subscribeTopic{"my/topic"};
    mutable mutex coutMutex;

    IMqttClient::InitializeParameters params;
    unique_ptr<IMqttClient>           client;
    unique_ptr<IDispatchQueue>        dispatcher;

    inline static void InterruptHandler(int signal) noexcept;

    void sendMessage(IMqttMessage::QOS) const;
    void LogLib(LogLevelLib lvl, string const& txt) const;

    virtual void Log(LogLevel, string const&) const override;
    virtual void OnMqttMessage(upMqttMessage_t) const override;
    virtual void OnConnectionStatusChanged(ConnectionType, Mqtt5ReasonCode) const override;
    virtual void OnSubscribe(token_t) const override;
    virtual void OnUnSubscribe(token_t) const override;
    virtual void OnPublish(token_t, Mqtt5ReasonCode) const override;

public:
    Sample(void)
      /* Create a dispatcher queue with logging provided by this and messages handed over to this */
      : dispatcher(DispatchQueueFactory::Create(this, *this))
    {
        signal(SIGINT, InterruptHandler);
        /*In debug we might want to get more logs from the underlying MQTT lib*/
#ifdef NDEBUG
        auto mqttLibLogLvl{LogLevelLib::INFO};
#else
        auto mqttLibLogLvl{LogLevelLib::DEBUG};
#endif
        /* Set log callback for underlying mqtt lib logs with minimum log level. This has to be done before
         * instantiating the first client object and cannot be done a second time */
        (void)IMqttClientCallbacks::InitLogMqttLib({bind(&Sample::LogLib, this, _1, _2), mqttLibLogLvl});
        params.clientId          = "myId";
        params.hostAddress       = "localhost";
        params.cleanSession      = true;
        params.keepAliveInterval = 10;
#ifdef IMQTT_WITH_TLS
#ifdef IMQTT_USE_PAHO
        params.hostAddress           = "ssl://" + params.hostAddress;
#endif
#ifdef IMQTT_EXPERIMENTAL
        params.clientCert = CLIENT_CERT;
        params.privateKey = PRIVATE_KEY;
#else
        params.clientCertFilePath = "/src/co/tiolan/imqtt/cert/user1.crt";
        params.privateKeyFilePath = "/src/co/tiolan/imqtt/cert/user1.key";
#endif
        params.port       = 8883;
        params.caFilePath = "/etc/mosquitto/certs/ca.crt";
        params.disableDefaultCaStore = true;
#else
        params.port = 1883;
#endif
        /* Finally create the client with received messages handled by dispatcher queue, logs by this, command callbacks
         * and connection change callbacks not handled (for demo purpose done in two steps)*/
        client = MqttClientFactory::Create(params, dispatcher.get(), this, nullptr, nullptr);
        /* Also set command and connection callbacks handled by this */
        client->SetCallbacks<IMqttConnectionCallbacks>(this);
        client->SetCallbacks<IMqttCommandCallbacks>(this);
        /* Disable logging */
        client->SetCallbacks<IMqttLogCallbacks>();
        /* Enable logging */
        client->SetCallbacks<IMqttLogCallbacks>(this);
    };
    ~Sample() noexcept = default;
    void Run(void);
};

bool               Sample::exitRun{false};
mutex              Sample::exitRunMutex;
condition_variable Sample::interrupt;

void
Sample::OnPublish(token_t token, Mqtt5ReasonCode) const
{
    Log(LogLevel::INFO, "Message was published for token: " + to_string(token));
}

void
Sample::OnUnSubscribe(token_t token) const
{
    Log(LogLevel::INFO, "Unsubscribe done for token: " + to_string(token));
}

void
Sample::sendMessage(IMqttMessage::QOS qos) const
{
    auto mqttMessage{MqttMessageFactory::Create("pub", {'H', 'E', 'L', 'L', 'O', '\0'}, qos)};
    mqttMessage->userProps.insert({"myKey1", "myValue1"});
    mqttMessage->userProps.insert({"myKey2", "myValue2"});
    mqttMessage->correlationDataProps   = IMqttMessage::correlationDataProps_t({'C', 'O', 'R', 'R', '\0'});
    mqttMessage->responseTopic          = "my/response/topic";
    mqttMessage->payloadFormatIndicator = IMqttMessage::FormatIndicator::UTF8;
    mqttMessage->payloadContentType     = "ASCII";
    token_t token{-1};
    client->PublishAsync(move(mqttMessage), &token);
    Log(LogLevel::INFO, "Publish done for token: " + to_string(token));
}

void
Sample::OnSubscribe(token_t token) const
{
    Log(LogLevel::INFO, "Subscribe done for token: " + to_string(token));
    sendMessage(IMqttMessage::QOS::QOS_0);
    sendMessage(IMqttMessage::QOS::QOS_1);
}

void
Sample::OnConnectionStatusChanged(ConnectionType type, Mqtt5ReasonCode reason) const
{
    if (type == ConnectionType::CONNECT && reason == Mqtt5ReasonCode::SUCCESS) {
        Log(LogLevel::INFO, "Sample is connected");
        token_t token{0};
        client->SubscribeAsync(subscribeTopic, IMqttMessage::QOS::QOS_1, &token);
        Log(LogLevel::INFO, "Subscribe token: " + to_string(token));
    }
    else {
        Log(LogLevel::INFO,
            "Sample is disconnected, MQTT5 rc: " + IMqttClient::Mqtt5ReasonCodeToStringRepr(reason).first);
    }
}

void
Sample::LogLib(LogLevelLib lvl, string const& txt) const
{
    lock_guard<mutex> lock(coutMutex);
    switch (lvl) {
    case LogLevelLib::DEBUG:
        cout << "LIB_D";
        break;
    case LogLevelLib::WARNING:
        cout << "LIB_W";
        break;
    case LogLevelLib::ERROR:
        cout << "LIB_E";
        break;
    case LogLevelLib::FATAL:
        cout << "LIB_F";
        break;
    case LogLevelLib::TRACE:
        cout << "LIB_T";
        break;
    case LogLevelLib::INFO:
        /*fallthrough*/
    default:
        cout << "LIB_I";
        break;
    }
    cout << ": " << txt << endl;
}

void
Sample::Log(LogLevel lvl, string const& txt) const
{
    lock_guard<mutex> lock(coutMutex);
    switch (lvl) {
    case LogLevel::DEBUG:
        cout << "D";
        break;
    case LogLevel::WARNING:
        cout << "W";
        break;
    case LogLevel::ERROR:
        cout << "E";
        break;
    case LogLevel::FATAL:
        cout << "D";
        break;
    case LogLevel::TRACE:
        cout << "T";
        break;
    case LogLevel::INFO:
        /*fallthrough*/
    default:
        cout << "I";
        break;
    }
    cout << ": " << txt << endl;
}

void
Sample::OnMqttMessage(upMqttMessage_t msq) const
{
    /*such printing should not be done in production - it is intentionally not guarded in IMqtt*/
#ifndef NDEBUG
    Log(LogLevel::INFO, "Got Mqtt Message: \n" + msq->ToString());
#else
    Log(LogLevel::INFO, "Got Mqtt Message");
#endif
    Log(LogLevel::INFO, "Simulating long message processing");
    sleep_for(seconds(1));
    Log(LogLevel::INFO, "Done with message processing");
}

void
Sample::Run(void)
{
    unique_lock<mutex> lock(exitRunMutex);
    Log(LogLevel::INFO, "Using lib version: " + client->GetLibVersion());
    client->ConnectAsync();

    // Block
    interrupt.wait(lock, [] { return exitRun; });
    token_t token{0};
    client->UnSubscribeAsync(subscribeTopic, &token);
    Log(LogLevel::INFO, "Unsubscribe token: " + to_string(token));
    /*Some time to allow the unsubscribe happen*/
    sleep_for(milliseconds(500));
    client->DisconnectAsync();
}

void
Sample::InterruptHandler(int signal) noexcept
{
    lock_guard<mutex> lock(exitRunMutex);
    exitRun = true;
    interrupt.notify_all();
}

int
main(void)
{
    /*for testing sonar*/
    /*
    char tmp[2] = {
        0,
    };
    tmp[4] = 'a';
    */
    Sample s;
    s.Run();
    return 0;
}