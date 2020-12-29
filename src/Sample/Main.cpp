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
#include "certs.h"
#endif

#include "IMqttClient.h"

using namespace std;
using namespace i_mqtt_client;
using namespace chrono;
using namespace this_thread;

class Sample final : public IMqttClientCallbacks {
private:
    static bool               exitRun;
    static mutex              exitRunMutex;
    static condition_variable interrupt;

    string const  subscribeTopic{"my/topic"};
    mutable mutex coutMutex;

    IMqttClient::InitializeParameters params;
    unique_ptr<IMqttClient>           client;

    inline static void InterruptHandler(int signal) noexcept;

    void sendMessage(IMqttMessage::QOS) const;

    virtual void Log(LogLevel, string const&) const override;
    virtual void OnMqttMessage(upMqttMessage_t) const override;
    virtual void OnConnectionStatusChanged(ConnectionType, Mqtt5ReasonCode) const override;
    virtual void OnSubscribe(int) const override;
    virtual void OnUnSubscribe(int) const override;
    virtual void OnPublish(token_t, Mqtt5ReasonCode) const override;

public:
    Sample(void)
    {
        signal(SIGINT, InterruptHandler);
        /*create with logs handled by this, messages handled by this, connection info handled by this*/
        params.callbackProvider  = {this, this, this};
        params.clientId          = "myId";
        params.hostAddress       = "localhost";
        params.cleanSession      = true;
        params.keepAliveInterval = 10;
#ifdef IMQTT_WITH_TLS
#ifdef IMQTT_USE_PAHO
        params.hostAddress           = "ssl://" + params.hostAddress;
        params.disableDefaultCaStore = false;
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
#else
        params.port = 1883;
#endif
        client = MqttClientFactory::create(params);
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
    Log(LogLevel::Info, "Message was published for token: " + to_string(token));
}

void
Sample::OnUnSubscribe(int token) const
{
    Log(LogLevel::Info, "Unsubscribe done for token: " + to_string(token));
}

void
Sample::sendMessage(IMqttMessage::QOS qos) const
{
    auto mqttMessage{MqttMessageFactory::create("pub", {'H', 'E', 'L', 'L', 'O', '\0'}, qos)};
    mqttMessage->userProps.insert({"myKey1", "myValue1"});
    mqttMessage->userProps.insert({"myKey2", "myValue2"});
    mqttMessage->correlationDataProps   = IMqttMessage::correlationDataProps_t({'C', 'O', 'R', 'R', '\0'});
    mqttMessage->responseTopic          = "my/response/topic";
    mqttMessage->payloadFormatIndicator = IMqttMessage::FormatIndicator::UTF8;
    mqttMessage->payloadContentType     = "ASCII";
    int token{-1};
    client->PublishAsync(move(mqttMessage), &token);
    Log(LogLevel::Info, "Publish done for token: " + to_string(token));
}

void
Sample::OnSubscribe(int token) const
{
    Log(LogLevel::Info, "Subscribe done for token: " + to_string(token));
    sendMessage(IMqttMessage::QOS::QOS_0);
    sendMessage(IMqttMessage::QOS::QOS_1);
}

void
Sample::OnConnectionStatusChanged(ConnectionType status, Mqtt5ReasonCode reason) const
{
    (void)reason;
    if (status == ConnectionType::CONNECT) {
        Log(LogLevel::Info, "Sample is connected");
        int token{0};
        client->SubscribeAsync(subscribeTopic, IMqttMessage::QOS::QOS_1, &token);
        Log(LogLevel::Info, "Subscribe token: " + to_string(token));
    }
    else {
        Log(LogLevel::Info, "Sample is disconnected");
    }
}

void
Sample::Log(LogLevel lvl, string const& txt) const
{
    lock_guard<mutex> lock(coutMutex);
    switch (lvl) {
    case LogLevel::Debug:
        cout << "D";
        break;
    case LogLevel::Warning:
        cout << "W";
        break;
    case LogLevel::Error:
        cout << "E";
        break;
    case LogLevel::Fatal:
        cout << "D";
        break;
    case LogLevel::Trace:
        cout << "T";
        break;
    case LogLevel::Info:
        [[fallthrough]];
    default:
        cout << "I";
        break;
    }
    cout << ": " << txt << endl;
}

void
Sample::OnMqttMessage(upMqttMessage_t msq) const
{
    Log(LogLevel::Info, "Got Mqtt Message: " + msq->toString());
}

void
Sample::Run(void)
{
    unique_lock<mutex> lock(exitRunMutex);
    /* disable logs */
    client->setCallbacks({nullptr, this, this});
    /* enable logs */
    client->setCallbacks({this, this, this});
    Log(LogLevel::Info, "Using lib version: " + client->GetLibVersion());
    client->ConnectAsync();

    // Block
    interrupt.wait(lock, [] { return exitRun; });
    int token{0};
    client->UnSubscribeAsync(subscribeTopic, &token);
    Log(LogLevel::Info, "Unsubscribe token: " + to_string(token));
    /*Some time to allow the unsubscribe happen*/
    sleep_for(milliseconds(500));
    client->Disconnect();
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
    Sample s;
    s.Run();
    return 0;
}