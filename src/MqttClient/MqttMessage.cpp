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
MqttMessage::MqttMessage(string const&                 topic,
                         payload_t const&              payload,
                         QOS                           qos,
                         userProps_t const&            userProps,
                         int                           messageId,
                         bool                          retain,
                         correlationDataProps_t const& correlationDataProps)
  : topic(topic)
  , payload(payload)
  , messageId(messageId)
  , qos(qos)
  , retain(retain)
  , userProps(userProps)
  , correlationDataProps(correlationDataProps)
{
}

string
MqttMessage::toString(void) const noexcept
{
    string str{"MqttMessage [topic]:\t" + getTopic() + "\n"};
    str += "MqttMessage [qos]:\t" + to_string(IMqttMessage::qosToInt(getQos())) + "\n";
    str += "MqttMessage [retain]:\t" + to_string(getRetained()) + "\n";
    str += "MqttMessage [messageId]:\t" + to_string(getMessageId()) + "\n";
    str += "\nMqttMessage[userProps]:\n";
    for (auto const& prop : getUserProps()) {
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

int
MqttMessage::getMessageId(void) const noexcept
{
    return messageId;
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

IMqttMessage::userProps_t const&
MqttMessage::getUserProps(void) const noexcept
{
    return userProps;
}

IMqttMessage::correlationDataProps_t const&
MqttMessage::getCorrelationDataProps(void) const noexcept
{
    return correlationDataProps;
}

upMqttMessage_t
MqttMessageFactory::create(string const&                               topic,
                           IMqttMessage::payload_t&&                   payload,
                           IMqttMessage::QOS                           qos,
                           bool                                        retain,
                           IMqttMessage::userProps_t const&            userProps,
                           IMqttMessage::correlationDataProps_t const& correlationDataProps,
                           int                                         messageId)
{
    return upMqttMessage_t(new MqttMessage(topic, payload, qos, userProps, messageId, retain, correlationDataProps));
}

upMqttMessage_t
MqttMessageFactory::create(string const&                               topic,
                           void*                                       pPayload,
                           size_t                                      size,
                           IMqttMessage::QOS                           qos,
                           bool                                        retain,
                           IMqttMessage::userProps_t const&            userProps,
                           IMqttMessage::correlationDataProps_t const& correlationDataProps,
                           int                                         messageId)
{
    return MqttMessageFactory::create(
        topic,
        IMqttMessage::payload_t(static_cast<unsigned char*>(pPayload), static_cast<unsigned char*>(pPayload) + size),
        qos,
        retain,
        userProps,
        correlationDataProps,
        messageId);
}
}  // namespace mqttclient
