/**
 * @file IMqttClient.h
 * @author Timo Lange
 * @brief
 * @date 2020
 * @copyright Timo Lange
 */

#include <condition_variable>
#include <csignal>
#include <iostream>
#include <mutex>
#include <string>

#include "IMqttClient.h"

using namespace std;
using namespace mqttclient;

class Sample final : public IMqttClientCallbacks {
private:
    static bool               exitRun;
    static mutex              exitRunMutex;
    static condition_variable interrupt;

    unique_ptr<IMqttClient> client{/* create with logs handled by this, messages handled by this */
                                   MqttClientFactory::create("localhost", 1885, "myId", {this, this, this})};

    inline static void InterruptHandler(int signal) noexcept;

    virtual void Log(LogLevel, string const&) const override;
    virtual void OnMqttMessage(upMqttMessage_t) const override;

public:
    Sample(void) noexcept { signal(SIGINT, InterruptHandler); };
    ~Sample() noexcept = default;
    void Run(void);
};

bool               Sample::exitRun{false};
mutex              Sample::exitRunMutex;
condition_variable Sample::interrupt;

void
Sample::Log(LogLevel lvl, string const& txt) const
{
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
    cout << "got Mqtt Message: " << msq->toString() << endl;
}

void
Sample::Run(void)
{
    unique_lock<mutex> lock(exitRunMutex);
    /* disable logs */
    client->setCallbacks({nullptr, this, this});
    /* enable logs */
    client->setCallbacks({this, this, this});
    cout << client->GetLibVersion() << endl;
    client->ConnectAsync();
    client->Subscribe("mytopic", IMqttMessage::QOS::QOS_0);
    client->Publish(MqttMessageFactory::create("mytopic", {1, 2, 3}, IMqttMessage::QOS::QOS_0));
    // Block
    interrupt.wait(lock, [] { return exitRun; });
}

void
Sample::InterruptHandler(int signal) noexcept
{
    // acquire a mutex to protect "exitRun"
    lock_guard<mutex> lock(exitRunMutex);
    exitRun = true;

    // notify all waiters with acquired "exitRun" mutex
    interrupt.notify_all();
}  // release "exitRun" mutex while going out of scope

int
main(void)
{
    Sample s;
    s.Run();
    return 0;
}