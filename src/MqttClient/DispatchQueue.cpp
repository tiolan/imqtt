/**
 * @file DispatchQueue.cpp
 * @author Timo Lange
 * @brief Implementation of a queue for storing incoming MQTT messages, to not black callbacks
 * @date 2020
 * @copyright Copyright 2020 Timo Lange

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

#include "DispatchQueue.h"

using namespace std;

namespace i_mqtt_client {
DispatchQueue::DispatchQueue(IMqttLogCallbacks const* log, IMqttMessageCallbacks const& msg)
  : logCb(log)
  , msgCb(msg)
  , messageDispatcherThread(&DispatchQueue::messageDispatcherWorker, this)
{
}

DispatchQueue::~DispatchQueue() noexcept
{
    messageDispatcherExit = true;
    messageDispatcherAwaiter.notify_all();
    if (messageDispatcherThread.joinable()) {
        messageDispatcherThread.join();
    }
    {
        lock_guard<mutex> lock(messageDispatcherMutex);
        auto              num{messageDispatcherQueue.size()};
        if (num) {
            log(LogLevel::WARNING, "Lost " + to_string(num) + " MQTT messages in queue on shutdown");
        }
    }
}

void
DispatchQueue::OnMqttMessage(upMqttMessage_t msg) const
{
    if (!messageDispatcherExit) {
        {
            lock_guard<mutex> lock(messageDispatcherMutex);
            messageDispatcherQueue.push(move(msg));
        }
        messageDispatcherAwaiter.notify_one();
    }
    // TODO: Indicate this case back?
}

void
DispatchQueue::messageDispatcherWorker(void)
{
    log(LogLevel::DEBUG, "Starting MQTT message dispatcher");
    unique_lock<mutex> lock(messageDispatcherMutex);
    while (!messageDispatcherExit) {
        log(LogLevel::DEBUG,
            "Number of MQTT messages still to be processed: " + to_string(messageDispatcherQueue.size()));
        messageDispatcherAwaiter.wait(lock,
                                      [this] { return (messageDispatcherQueue.size() || messageDispatcherExit); });
        if (!messageDispatcherExit && messageDispatcherQueue.size()) {
            auto msg{move(messageDispatcherQueue.front())};
            messageDispatcherQueue.pop();
            lock.unlock();
            msgCb.OnMqttMessage(move(msg));
            lock.lock();
        }
    }
    log(LogLevel::INFO, "Exiting MQTT message dispatcher");
}

void
DispatchQueue::log(LogLevel lvl, std::string const& txt) const
{
    if (logCb) {
        logCb->Log(lvl, txt);
    }
}

unique_ptr<IDispatchQueue>
DispatchQueueFactory::Create(IMqttLogCallbacks const* log, IMqttMessageCallbacks const& msg)
{
    return unique_ptr<IDispatchQueue>(new DispatchQueue(log, msg));
}

}  // namespace i_mqtt_client