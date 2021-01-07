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

/**
 @brief Describes the abstract interface to be used in order to deal with MqttMessages. It hides the
 * underlying implementation of an MQTT message object.
 *
 */
class IMqttMessage {
public:
    using payloadRaw_t           = unsigned char;
    using payload_t              = const std::vector<payloadRaw_t>;
    using userProps_t            = std::map<std::string, std::string>;
    using correlationDataProps_t = std::vector<payloadRaw_t>;
    /**
     * @brief Payload Format Indicator as defined in the MQTTv5 standard
     *
     */
    enum class FormatIndicator { UNSPECIFIED, UTF8 };
    /**
     * @brief Quality of Service as defined in the MQTTv5 standard
     *
     */
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
    const std::string       topic;   /*!< the topic a message was or should be published to */
    const payload_t         payload; /*!< the raw binary payload of the message */
    const IMqttMessage::QOS qos;     /*!< the Quality of Service the message was or should be published with */
    const bool retain; /*!< indicates whether the message was or should be published with the retained flag set */

    /*Optional fields, publicly accessible and settable*/
    int                    messageId{-1};                                        /*!< the message ID, this message was published with */
    userProps_t            userProps{userProps_t()};                             /*!< user properties as defined in the MQTTv5 standard, string map */
    correlationDataProps_t correlationDataProps{correlationDataProps_t()};       /*!< binary correltation data, as defined in the MQTTv5 standard */
    std::string            responseTopic{""};                                    /*!< a response topic as defined in the MQTTv5 standard */
    FormatIndicator        payloadFormatIndicator{FormatIndicator::UNSPECIFIED}; /*!< payload format indicator as defined in the MQTTv5 standard */
    std::string            payloadContentType{""};                               /*!< playload content type as defined in the MQTTv5 standard, string */

    /**
     * @brief Returns the raw byte payload casted a C++ string. Depending on the payload not printable.
     * 
     * @return string container for raw, binary payload data
     */
    virtual std::string GetPayloadCastedToString(void) const         = 0;

    /**
     * @brief Returns the raw byte correlation data casted a C++ string. Depending on the correlation data not
     * printable.
     *
     * @return string container for raw, binary correlation data
     */
    virtual std::string GetCorrelationDataCastedToString(void) const = 0;

    /**
     * @brief Returns a textual description of the MqttMessage
     * 
     * @return textual representation
     */
    virtual std::string ToString(void) const                         = 0;
};

using upMqttMessage_t = std::unique_ptr<IMqttMessage>;


/**
 * @brief Used to instantiate an MqttMessage object behind an IMqttMessage interface.
 *
 */
class MqttMessageFactory final {
public:
    /**
     * @brief Used to create an MqttMessage object behind an IMqttMessage interface.
     *
     * @param topic sets the message's topic
     * @param payload sets the message's payload
     * @param qos sets the message's qos flag
     * @param retain sets the message's retain flag
     * @return unique pointer to an MqttMessage behind an IMqttMessage interface, the user is responsible for object
     * lifetimes
     */
    static upMqttMessage_t Create(std::string const&              topic,
                                  IMqttMessage::payload_t const&& payload,
                                  IMqttMessage::QOS               qos,
                                  bool                            retain = false);
    MqttMessageFactory() = delete;
};
}  // namespace i_mqtt_client