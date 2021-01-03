/**
 * @file IDispatchQueue.h
 * @author Timo Lange
 * @brief Abstract interface definition for DispatchQueue
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

#include <memory>

#include "IMqttClientCallbacks.h"

namespace i_mqtt_client {
class IDispatchQueue : public IMqttMessageCallbacks {
protected:
    IDispatchQueue(void) = default;

public:
    IDispatchQueue(const IDispatchQueue&) = delete;
    IDispatchQueue(IDispatchQueue&&)      = delete;
    IDispatchQueue& operator=(const IDispatchQueue&) = delete;
    IDispatchQueue& operator=(IDispatchQueue&&) = delete;
    void*           operator new[](size_t)      = delete;

    virtual ~IDispatchQueue() noexcept = default;
};

class DispatchQueueFactory final {
public:
    static std::unique_ptr<IDispatchQueue> Create(IMqttLogCallbacks const*, IMqttMessageCallbacks const&);
    DispatchQueueFactory() = delete;
};
}  // namespace i_mqtt_client
