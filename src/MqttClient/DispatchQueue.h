/**
 * @file DispatchQueue.h
 * @author Timo Lange
 * @brief Class definition for DispatchQueue
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

#pragma once

#include <atomic>
#include <condition_variable>
#include <queue>
#include <thread>

#include "IDispatchQueue.h"
#include "IMqttMessage.h"

namespace i_mqtt_client {
class DispatchQueue : public IDispatchQueue {
private:
    IMqttLogCallbacks const*            logCb;
    IMqttMessageCallbacks const&        msgCb;
    mutable std::mutex                  messageDispatcherMutex;
    mutable std::queue<upMqttMessage_t> messageDispatcherQueue;
    mutable std::condition_variable     messageDispatcherAwaiter;
    std::thread                         messageDispatcherThread;
    std::atomic_bool                    messageDispatcherExit{false};

    void messageDispatcherWorker(void);
    void log(LogLevel, std::string const&) const;

    virtual void OnMqttMessage(upMqttMessage_t) const override;

public:
    DispatchQueue(IMqttLogCallbacks const*, IMqttMessageCallbacks const&);
    virtual ~DispatchQueue() noexcept;
};
}  // namespace i_mqtt_client
