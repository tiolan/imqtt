/**
 * @file MqttMessage.cpp
 * @author Timo Lange
 * @brief Implementation for MQTT messages in C++
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

namespace i_mqtt_client {
MqttMessage::MqttMessage(string const& topic, payload_t const& payload, QOS qos, bool retain)
  : IMqttMessage(topic, payload, qos, retain)
{
}

string
MqttMessage::toString(void) const noexcept
{
    string str{"MqttMessage [topic]:\t" + topic + "\n"};
    str += "MqttMessage [qos]:\t" + to_string(static_cast<int>(qos)) + "\n";
    str += "MqttMessage [retain]:\t" + to_string(retain) + "\n";
    str += "MqttMessage [messageId]:\t" + to_string(messageId) + "\n";
    str += "\nMqttMessage[userProps]:\n";
    for (auto const& prop : userProps) {
        str += "\t[" + prop.first + "]:" + prop.second + "\n";
    }

    return str;
}

string
MqttMessage::getPayloadCastedToString(void) const
{
    return string(reinterpret_cast<const char*>(payload.data()), static_cast<size_t>(payload.size()));
}

upMqttMessage_t
MqttMessageFactory::create(string const& topic, IMqttMessage::payload_t&& payload, IMqttMessage::QOS qos, bool retain)
{
    return upMqttMessage_t(new MqttMessage(topic, payload, qos, retain));
}
}  // namespace i_mqtt_client
