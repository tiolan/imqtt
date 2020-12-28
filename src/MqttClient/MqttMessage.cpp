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

#include "MqttMessage.h"

using namespace std;

namespace mqttclient {
MqttMessage::MqttMessage(string const& topic, payload_t const& payload, QOS qos, bool retain)
  : IMqttMessage(topic, payload, qos, retain)
{
}

string
MqttMessage::toString(void) const noexcept
{
    string str{"MqttMessage [topic]:\t" + getTopic() + "\n"};
    str += "MqttMessage [qos]:\t" + to_string(IMqttMessage::qosToInt(getQos())) + "\n";
    str += "MqttMessage [retain]:\t" + to_string(getRetained()) + "\n";
    str += "MqttMessage [messageId]:\t" + to_string(messageId) + "\n";
    str += "\nMqttMessage[userProps]:\n";
    for (auto const& prop : userProps) {
        str += "\t[" + prop.first + "]:" + prop.second + "\n";
    }

    return str;
}

string const&
MqttMessage::getTopic(void) const noexcept
{
    return topic;
}

IMqttMessage::payload_t const&
MqttMessage::getPayload(void) const noexcept
{
    return payload;
}

string
MqttMessage::getPayloadCastedToString(void) const
{
    return string(reinterpret_cast<const char*>(payload.data()), static_cast<size_t>(payload.size()));
}

bool
MqttMessage::getRetained(void) const noexcept
{
    return retain;
}

IMqttMessage::QOS
MqttMessage::getQos(void) const noexcept
{
    return qos;
}

upMqttMessage_t
MqttMessageFactory::create(string const& topic, IMqttMessage::payload_t&& payload, IMqttMessage::QOS qos, bool retain)
{
    return upMqttMessage_t(new MqttMessage(topic, payload, qos, retain));
}
}  // namespace mqttclient
