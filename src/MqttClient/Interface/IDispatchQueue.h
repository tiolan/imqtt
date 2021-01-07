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
/**
 * @brief MQTT library implementations usually rely on callbacks, that are invoked by the MQTT library to handover the
 * MQTT message to the user. These callbacks usually have to be done very quick in order to not block the MQTT library
 * from doing further actions, such as responsing to ping requests. An object of type DispatchQueue, hidden behind an
 * IDispatchQueue basically is a FIFO queue, that implements the IMqttMessageCallbacks::OnMqttMessage callback. When
 * providing a pointer to IDispatchQueue to an IMqttClient, the callbacks of the underlying MQTT library will store all
 * incoming messages inside the FIFO. IDispatch queue will hand them over in the same way to an IMqttClient object via
 * IMqttMessageCallbacks::OnMqttMessage, but decoupled from the MQTT library's callback. Such, that a processing of a
 * message may take a relatively long time without blocking the MQTT library. The queue runs on a separate thread and
 * stores all incoming messages in RAM.
 * @todo The user should be able to provide a maximum message queue size. Currently all incoming messages would be
 * buffered until the RAM is full.
 */
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

/**
 * @brief Used to instantiate a DispatchQueue object behind an IDispatchQueue interface.
 *
 */
class DispatchQueueFactory final {
public:
    /**
     * @brief Generates a DispatchQueue object behind an IDispatchQueue interface. The user is responsible for object
     * lifetime management.
     *
     * @param log reference to an object providing a log callback
     * @param msg reference to an object providig a message callback in order to deliver messages to the user
     * @return unique pointer to a DispatchQueue hidden behind an IDispatchQueue interface
     */
    static std::unique_ptr<IDispatchQueue> Create(IMqttLogCallbacks const* log, IMqttMessageCallbacks const& msg);
    DispatchQueueFactory() = delete;
};
}  // namespace i_mqtt_client
