/**
 * @file IMqttMessage.h
 * @author Timo Lange
 * @brief Abstract interface definition for MQTT messages in C++
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

#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "IMqttClientDefines.h"

namespace i_mqtt_client {
class IMqttMessage {
public:
    using payloadRaw_t           = unsigned char;
    using payload_t              = const std::vector<payloadRaw_t>;
    using userProps_t            = std::map<std::string, std::string>;
    using correlationDataProps_t = std::vector<payloadRaw_t>;
    enum class FormatIndicator { UNSPECIFIED, UTF8 };
    enum class QOS : int { QOS_0 = 0, QOS_1 = 1, QOS_2 = 2 };

private:
    IMqttMessage(const IMqttMessage&) = delete;
    IMqttMessage(IMqttMessage&&)      = delete;
    IMqttMessage& operator=(const IMqttMessage&) = delete;
    IMqttMessage& operator=(IMqttMessage&&) = delete;
    void*         operator new[](size_t)    = delete;

protected:
    IMqttMessage(std::string const& topic, payload_t const& payload, QOS qos, bool retain)
      : topic(topic)
      , payload(payload)
      , qos(qos)
      , retain(retain)
    {
    }

public:
    virtual ~IMqttMessage() noexcept = default;

    /*Mandatory immutable fields, created via Factory*/
    const std::string       topic;
    const payload_t         payload;
    const IMqttMessage::QOS qos;
    const bool              retain;

    /*Optional fields, publicly accessible and settable*/
    int                    messageId{-1};
    userProps_t            userProps{userProps_t()};
    correlationDataProps_t correlationDataProps{correlationDataProps_t()};
    std::string            responseTopic{""};
    FormatIndicator        payloadFormatIndicator{FormatIndicator::UNSPECIFIED};
    std::string            payloadContentType{""};

    virtual std::string GetPayloadCastedToString(void) const         = 0;
    virtual std::string GetCorrelationDataCastedToString(void) const = 0;
    virtual std::string ToString(void) const                         = 0;
};

using upMqttMessage_t = std::unique_ptr<IMqttMessage>;

class MqttMessageFactory final {
public:
    MqttMessageFactory() = delete;
    static upMqttMessage_t Create(std::string const&              topic,
                                  IMqttMessage::payload_t const&& payload,
                                  IMqttMessage::QOS               qos,
                                  bool                            retain = false);
};
}  // namespace i_mqtt_client