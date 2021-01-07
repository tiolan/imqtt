
/**
 * @file IMqttClientDefines.h
 * @author Timo Lange
 * @brief Constant defines and textual representations
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

#include <functional>
#include <limits>

namespace i_mqtt_client {
/**
 * @brief Represents error codes, used for feedback to IMqttClient calls.
 */
enum class ReasonCode {
    OKAY,
    ERROR_GENERAL,
    ERROR_NO_CONNECTION,
    ERROR_TLS,
    NOT_ALLOWED,
    /*When adding ReasonCodes, also add them to the string representation*/
};

/**
 * @brief Represents MQTTv3 ReasonCodes retunred by the library
 * 
 */
enum class MqttReasonCode : int {
    ACCEPTED                      = 0x0,
    UNACCEPTABLE_PROTOCOL_VERSION = 0x1,
    IDENTIFIER_REJECTED           = 0x2,
    SERVER_UNAVAILABLE            = 0x3,
    BAD_USERNAME_OR_PASSWORD      = 0x4,
    NOT_AUTHORIZED                = 0x5,
    /*When adding MqttReasonCodes, also add them to the string representation*/
};

/**
 * @brief Represents MQTTv5 ReasonCodes returned by the broker
 *
 */
enum class Mqtt5ReasonCode : int {
    SUCCESS /*GRANTED_QOS_0*/              = 0x0,  /*CONNACK, PUBACK, PUBREC, PUBREL, PUBCOMP, UNSUBACK, AUTH / SUBACK*/
    GRANTED_QOS_1                          = 0x01, /*SUBACK*/
    GRANTED_QOS_2                          = 0x02, /*SUBACK*/
    DISCONNECT_WITH_WILL_MESSAGE           = 0x04, /*DISCONNECT*/
    NO_MATCHING_SUBSCRIBERS                = 0x10, /*PUBACK, PUBREC*/
    NO_SUBSCRIPTION_EXISTS                 = 0x11, /*UNSUBACK*/
    CONTINUE_AUTHENTICATION                = 0x18, /*AUTH*/
    RE_AUTHENTICATE                        = 0x19, /*AUTH*/
    UNSPECIFIED_ERROR                      = 0x80, /*CONNACK, PUBACK, PUBREC, SUBACK, UNSUBACK, DISCONNECT*/
    MALFORMED_PACKET                       = 0x81, /*CONNACK, DISCONNECT*/
    PROTOCOL_ERROR                         = 0x82, /*CONNACK, DISCONNECT*/
    IMPLEMENTATION_SPECIFIC_ERROR          = 0x83, /*CONNACK, PUBACK, PUBREC, SUBACK, UNSUBACK, DISCONNECT*/
    UNSUPPORTED_PROTOCOL_VERSION           = 0x84, /*CONNACK*/
    CLIENT_IDENTIFIER_NOT_VALID            = 0x85, /*CONNACK*/
    BAD_USER_NAME_OR_PASSWORD              = 0x86, /*CONNACK*/
    NOT_AUTHORIZED                         = 0x87, /*CONNACK, PUBACK, PUBREC, SUBACK, UNSUBACK, DISCONNECT*/
    SERVER_UNAVAILABLE                     = 0x88, /*CONNACK*/
    SERVER_BUSY                            = 0x89, /*CONNACK, DISCONNECT*/
    BANNED                                 = 0x8A, /*CONNACK*/
    SERVER_SHUTTING_DOWN                   = 0x8B, /*DISCONNECT*/
    BAD_AUTHENTICATION_METHOD              = 0x8C, /*CONNACK, DISCONNECT*/
    KEEP_ALIVE_TIMEOUT                     = 0x8D, /*DISCONNECT*/
    SESSION_TAKEN_OVER                     = 0x8E, /*DISCONNECT*/
    TOPIC_FILTER_INVALID                   = 0x8F, /*SUBACK,UNSUBACK, DISCONNECT*/
    TOPIC_NAME_INVALID                     = 0x90, /*CONNACK, PUBACK, PUBREC, DISCONNECT*/
    PACKET_IDENTIFIER_IN_USE               = 0x91, /*PUBACK, PUBREC, SUBACK, UNSUBACK*/
    PACKET_IDENTIFIER_NOT_FOUND            = 0x92, /*PUBREL, PUBCOMP */
    RECEIVE_MAXIMUM_EXCEEDED               = 0x93, /*DISCONNECT*/
    TOPIC_ALIAS_INVALID                    = 0x94, /*DISCONNECT*/
    PACKET_TOO_LARGE                       = 0x95, /*CONNACK*/
    MESSAGE_RATE_TOO_HIGH                  = 0x96, /*DISCONNECT*/
    QUOTA_EXCEEDED                         = 0x97, /*CONNACK, PUBACK, PUBREC, SUBACK*/
    ADMINISTRATIVE_ACTION                  = 0x98, /*DISCONNECT*/
    PAYLOAD_FORMAT_INVALID                 = 0x99, /*PUBACK, PUBREC*/
    RETAIN_NOT_SUPPORTED                   = 0x9A, /*CONNACK, DISCONNECT*/
    QOS_NOT_SUPPORTED                      = 0x9B, /*CONNACK*/
    USE_ANOTHER_SERVER                     = 0x9C, /*CONNACK, DISCONNECT*/
    SERVER_MOVED                           = 0x9D, /*CONNACK*/
    SHARED_SUBSCRIPTIONS_NOT_SUPPORTED     = 0x9E, /*SUBACK, DISCONNECT*/
    CONNECTION_RATE_EXCEEDED               = 0x9F, /*CONNACK*/
    MAXIMUM_CONNECT_TIME                   = 0xA0, /*DISCONNECT*/
    SUBSCRIPTION_IDENTIFIERS_NOT_SUPPORTED = 0xA1, /*SUBACK*/
    WILDCARD_SUBSCRIPTIONS_NOT_SUPPORTED   = 0xA2, /*SUBACK, DISCONNECT*/
    /*When adding Mqtt5ReasonCodes, also add them to the string representation*/
};

using ReasonCodeStringShort_t = const std::string;
using ReasonCodeStringLong_t  = const std::string;
using ReasonCodeRepr_t        = const std::pair<ReasonCodeStringShort_t, ReasonCodeStringLong_t>;

using MqttReasonCodeStringShort_t = const std::string;
using MqttReasonCodeStringLong_t  = const std::string;
using MqttReasonCodeRepr_t        = const std::pair<MqttReasonCodeStringShort_t, MqttReasonCodeStringLong_t>;

using Mqtt5ReasonCodeStringShort_t = const std::string;
using Mqtt5ReasonCodeStringLong_t  = const std::string;
using Mqtt5ReasonCodeRepr_t        = const std::pair<Mqtt5ReasonCodeStringShort_t, Mqtt5ReasonCodeStringLong_t>;

/**
 * @brief Represents log levels for messages from the IMqttClient implementation
 * 
 */
enum class LogLevel : int {
    TRACE   = 1,
    DEBUG   = 2,
    INFO    = 3,
    WARNING = 4,
    ERROR   = 5,
    FATAL   = 6,
};

/**
 * @brief Represents log levels for messages from the underlying MQTT library
 *
 */
enum class LogLevelLib : int {
    TRACE   = 1,
    DEBUG   = 2,
    INFO    = 3,
    WARNING = 4,
    ERROR   = 5,
    FATAL   = 6,
    NONE    = std::numeric_limits<int>::max(),
};

/**
 * @brief A callback that is invoked for handing over logs from the underlying MQTT library to the user. May be set to
 * nullptr in order to disable logging.
 *
 */
using MmqttLibLogCb_t = std::function<void(LogLevelLib, std::string const&)>;

/**
 * @brief Pair containing the callback function for logs from the underlying MQTT library and the minimum log level.
 *
 */
using MqttLogInit_t = std::pair<MmqttLibLogCb_t, LogLevelLib>;
}  // namespace i_mqtt_client