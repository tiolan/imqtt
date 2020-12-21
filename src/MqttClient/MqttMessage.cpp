/**
 * @file IMqttClient.h
 * @author Timo Lange
 * @brief
 * @date 2020
 * @copyright Timo Lange
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
    return string(reinterpret_cast<const char*>(payload.data()), reinterpret_cast<size_t>(payload.size()));
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
