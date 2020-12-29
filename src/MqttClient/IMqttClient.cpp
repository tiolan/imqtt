
/**
 * @file IMqttClient.cpp
 * @author Timo Lange
 * @brief Implementation for default functionality defined in IMqttClient.h
 * @date 2020
 * @copyright Copyright 2020 Timo Lange

                                              Licensed under the Apache License,
    Version 2.0(the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#include "IMqttClient.h"

#include <map>

using namespace std;

namespace i_mqtt_client {

static const map<ReasonCode, IMqttClient::ReasonCodeRepr_t> reasonCodeToString{
    {ReasonCode::OKAY, {"OKAY", "The operation was successful"}},
    {ReasonCode::ERROR_GENERAL, {"ERROR_GENERAL", "A general error occured"}},
    {ReasonCode::ERROR_NO_CONNECTION, {"ERROR_NO_CONNECTION", "No connection to the broker"}},
    {ReasonCode::ERROR_TLS, {"ERROR_TLS", "A TLS error occured"}},
    {ReasonCode::NOT_ALLOWED, {"NOT_ALLOWED", "The broker refused the connection"}}};

IMqttClient::ReasonCodeRepr_t
IMqttClient::ReasonCodeToStringRepr(ReasonCode rc)
{
    auto repr{reasonCodeToString.find(rc)};
    if (repr != reasonCodeToString.end()) {
        return repr->second;
    }
    return {"UNKNOWN", "The provided reason code is unknown"};
}

static const map<MqttReasonCode, IMqttClient::MqttReasonCodeRepr_t> mqttReasonCodeToString{
    {MqttReasonCode::ACCEPTED, {"ACCEPTED", "Connection accepted"}},
    {MqttReasonCode::UNACCEPTABLE_PROTOCOL_VERSION, {"UNACCEPTABLE_PROTOCOL_VERSION", "Unacceptable protocol version"}},
    {MqttReasonCode::IDENTIFIER_REJECTED, {"IDENTIFIER_REJECTED", "Identifier rejected"}},
    {MqttReasonCode::SERVER_UNAVAILABLE, {"SERVER_UNAVAILABLE", "Server unavailable"}},
    {MqttReasonCode::BAD_USERNAME_OR_PASSWORD, {"BAD_USERNAME_OR_PASSWORD", "Bad user name or password"}},
    {MqttReasonCode::NOT_AUTHORIZED, {"NOT_AUTHORIZED", "Not authorized"}}};

IMqttClient::ReasonCodeRepr_t
IMqttClient::MqttReasonCodeToStringRepr(int rc)
{
    return MqttReasonCodeToStringRepr(static_cast<MqttReasonCode>(rc));
}

IMqttClient::ReasonCodeRepr_t
IMqttClient::MqttReasonCodeToStringRepr(MqttReasonCode rc)
{
    auto repr{mqttReasonCodeToString.find(rc)};
    if (repr != mqttReasonCodeToString.end()) {
        return repr->second;
    }
    return {"UNKNOWN", "Unknown MQTT reason code"};
}

const static map<Mqtt5ReasonCode, IMqttClient::Mqtt5ReasonCodeRepr_t> mqtt5ReasonCodeToString{
    {Mqtt5ReasonCode::SUCCESS, {"SUCCESS", "Success"}},
    {Mqtt5ReasonCode::GRANTED_QOS_1, {"GRANTED_QOS_1", "Granted QoS 1"}},
    {Mqtt5ReasonCode::GRANTED_QOS_2, {"GRANTED_QOS_2", "Granted QoS 2 "}},
    {Mqtt5ReasonCode::DISCONNECT_WITH_WILL_MESSAGE, {"DISCONNECT_WITH_WILL_MESSAGE", "Disconnect with Will Message"}},
    {Mqtt5ReasonCode::NO_MATCHING_SUBSCRIBERS, {"NO_MATCHING_SUBSCRIBERS", "No matching subscribers "}},
    {Mqtt5ReasonCode::NO_SUBSCRIPTION_EXISTS, {"NO_SUBSCRIPTION_EXISTS", "No subscription existed"}},
    {Mqtt5ReasonCode::CONTINUE_AUTHENTICATION, {"CONTINUE_AUTHENTICATION", "Continue authentication"}},
    {Mqtt5ReasonCode::RE_AUTHENTICATE, {"RE_AUTHENTICATE", "Re-authenticate"}},
    {Mqtt5ReasonCode::UNSPECIFIED_ERROR, {"UNSPECIFIED_ERROR", "Unspecified error"}},
    {Mqtt5ReasonCode::MALFORMED_PACKET, {"MALFORMED_PACKET", "Malformed Packet"}},
    {Mqtt5ReasonCode::PROTOCOL_ERROR, {"PROTOCOL_ERROR", "Protocol Error"}},
    {Mqtt5ReasonCode::IMPLEMENTATION_SPECIFIC_ERROR,
     {"IMPLEMENTATION_SPECIFIC_ERROR", "Implementation specific error"}},
    {Mqtt5ReasonCode::UNSUPPORTED_PROTOCOL_VERSION, {"UNSUPPORTED_PROTOCOL_VERSION", "Unsupported Protocol Version"}},
    {Mqtt5ReasonCode::CLIENT_IDENTIFIER_NOT_VALID, {"CLIENT_IDENTIFIER_NOT_VALID", "Client Identifier not valid"}},
    {Mqtt5ReasonCode::BAD_USER_NAME_OR_PASSWORD, {"BAD_USER_NAME_OR_PASSWORD", "Bad User Name or Password"}},
    {Mqtt5ReasonCode::NOT_AUTHORIZED, {"NOT_AUTHORIZED", "Not authorized"}},
    {Mqtt5ReasonCode::SERVER_UNAVAILABLE, {"SERVER_UNAVAILABLE", "Server unavailable"}},
    {Mqtt5ReasonCode::SERVER_BUSY, {"SERVER_BUSY", "Server busy"}},
    {Mqtt5ReasonCode::BANNED, {"BANNED", "Banned"}},
    {Mqtt5ReasonCode::SERVER_SHUTTING_DOWN, {"SERVER_SHUTTING_DOWN", "Server shutting down"}},
    {Mqtt5ReasonCode::BAD_AUTHENTICATION_METHOD, {"BAD_AUTHENTICATION_METHOD", "Bad authentication method"}},
    {Mqtt5ReasonCode::KEEP_ALIVE_TIMEOUT, {"KEEP_ALIVE_TIMEOUT", "Keep Alive timeout"}},
    {Mqtt5ReasonCode::SESSION_TAKEN_OVER, {"SESSION_TAKEN_OVER", "Session taken over"}},
    {Mqtt5ReasonCode::TOPIC_FILTER_INVALID, {"TOPIC_FILTER_INVALID", "Topic Filter invalid"}},
    {Mqtt5ReasonCode::TOPIC_NAME_INVALID, {"TOPIC_NAME_INVALID", "Topic Name invalid"}},
    {Mqtt5ReasonCode::PACKET_IDENTIFIER_IN_USE, {"PACKET_IDENTIFIER_IN_USE", "Packet Identifier in use"}},
    {Mqtt5ReasonCode::PACKET_IDENTIFIER_NOT_FOUND, {"PACKET_IDENTIFIER_NOT_FOUND", "Packet Identifier not found"}},
    {Mqtt5ReasonCode::RECEIVE_MAXIMUM_EXCEEDED, {"RECEIVE_MAXIMUM_EXCEEDED", "Receive Maximum exceeded"}},
    {Mqtt5ReasonCode::TOPIC_ALIAS_INVALID, {"TOPIC_ALIAS_INVALID", "Topic Alias invalid"}},
    {Mqtt5ReasonCode::PACKET_TOO_LARGE, {"PACKET_TOO_LARGE", "Packet too large"}},
    {Mqtt5ReasonCode::MESSAGE_RATE_TOO_HIGH, {"MESSAGE_RATE_TOO_HIGH", "Message rate too high"}},
    {Mqtt5ReasonCode::QUOTA_EXCEEDED, {"QUOTA_EXCEEDED", "Quota exceeded"}},
    {Mqtt5ReasonCode::ADMINISTRATIVE_ACTION, {"ADMINISTRATIVE_ACTION", "Administrative action"}},
    {Mqtt5ReasonCode::PAYLOAD_FORMAT_INVALID, {"PAYLOAD_FORMAT_INVALID", "Payload format invalid"}},
    {Mqtt5ReasonCode::RETAIN_NOT_SUPPORTED, {"RETAIN_NOT_SUPPORTED", "Retain not supported"}},
    {Mqtt5ReasonCode::QOS_NOT_SUPPORTED, {"QOS_NOT_SUPPORTED", "QoS not supported"}},
    {Mqtt5ReasonCode::USE_ANOTHER_SERVER, {"USE_ANOTHER_SERVER", "Use another server"}},
    {Mqtt5ReasonCode::SERVER_MOVED, {"SERVER_MOVED", "Server moved"}},
    {Mqtt5ReasonCode::SHARED_SUBSCRIPTIONS_NOT_SUPPORTED,
     {"SHARED_SUBSCRIPTIONS_NOT_SUPPORTED", "Shared Subscription not supported"}},
    {Mqtt5ReasonCode::CONNECTION_RATE_EXCEEDED, {"CONNECTION_RATE_EXCEEDED", "Connection rate exceeded"}},
    {Mqtt5ReasonCode::MAXIMUM_CONNECT_TIME, {"MAXIMUM_CONNECT_TIME", "Maximum connect time"}},
    {Mqtt5ReasonCode::SUBSCRIPTION_IDENTIFIERS_NOT_SUPPORTED,
     {"SUBSCRIPTION_IDENTIFIERS_NOT_SUPPORTED", "Subscription Identifiers not supported"}},
    {Mqtt5ReasonCode::WILDCARD_SUBSCRIPTIONS_NOT_SUPPORTED,
     {"WILDCARD_SUBSCRIPTIONS_NOT_SUPPORTED", "Wildcard Subscription not supported"}},
};

IMqttClient::ReasonCodeRepr_t
IMqttClient::Mqtt5ReasonCodeToStringRepr(int rc)
{
    return Mqtt5ReasonCodeToStringRepr(static_cast<Mqtt5ReasonCode>(rc));
}

IMqttClient::ReasonCodeRepr_t
IMqttClient::Mqtt5ReasonCodeToStringRepr(Mqtt5ReasonCode rc)
{
    // TODO: Once https://github.com/eclipse/mosquitto/issues/1984 is understood, back to
    // return mqtt5ReasonCodeToString.at(rc);
    auto repr{mqtt5ReasonCodeToString.find(rc)};
    if (repr != mqtt5ReasonCodeToString.end()) {
        return repr->second;
    }
    return {"UNKNOWN", "Unknown MQTT5 reason code"};
}
}  // namespace i_mqtt_client
