/**
 * @file IMqttClient.h
 * @author Timo Lange
 * @brief
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

#pragma once

#include <string>

#include "IMqttMessage.h"

namespace mqttclient {

class MqttMessage final : public IMqttMessage {
public:
    virtual inline std::string const& getTopic(void) const noexcept override;
    virtual inline payload_t const&   getPayload(void) const noexcept override;
    virtual inline std::string        getPayloadCastedToString(void) const override;
    virtual inline bool               getRetained(void) const noexcept override;
    virtual inline QOS                getQos(void) const noexcept override;
    virtual inline std::string        toString(void) const noexcept override;

    MqttMessage(std::string const&, payload_t const&, QOS, bool);
};
}  // namespace mqttclient