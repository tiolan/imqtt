/**
 * @file IMqttClient.h
 * @author Timo Lange
 * @brief
 * @date 2020
 * @copyright Timo Lange
 */

#pragma once

#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace mqttclient {
class IMqttMessage {
public:
    using payloadRaw_t           = unsigned char;
    using payload_t              = const std::vector<payloadRaw_t>;
    using userProps_t            = std::map<std::string, std::string>;
    using correlationDataProps_t = std::vector<payloadRaw_t>;
    enum class FormatIndicator { Unspecified, UTF8 };
    enum class QOS { QOS_0, QOS_1, QOS_2 };
    static inline QOS
    intToQos(int qos)
    {
        switch (qos) {
        case 0:
            return QOS::QOS_0;
        case 1:
            return QOS::QOS_1;
        case 2:
            return QOS::QOS_2;
        default:
            throw std::runtime_error("provide a valid qos [0,1,2]");
        }
    }
    static inline int
    qosToInt(QOS qos)
    {
        switch (qos) {
        case QOS::QOS_0:
            return 0;
        case QOS::QOS_1:
            return 1;
        case QOS::QOS_2:
            return 2;
        default:
            throw std::runtime_error("provide a valid qos [0,1,2]");
        }
    }

private:
    IMqttMessage(const IMqttMessage&) = delete;
    IMqttMessage(IMqttMessage&&)      = delete;
    IMqttMessage& operator=(const IMqttMessage&) = delete;
    IMqttMessage& operator=(IMqttMessage&&) = delete;

    void* operator new[](size_t) = delete;

protected:
    IMqttMessage(std::string const& topic, payload_t const& payload, IMqttMessage::QOS qos, bool retain)
      : topic(topic)
      , payload(payload)
      , qos(qos)
      , retain(retain)
    {
    }

    /*Mandatory immutable fields, created via Factory*/
    std::string       topic;
    payload_t         payload;
    IMqttMessage::QOS qos;
    bool              retain;

public:
    virtual ~IMqttMessage() noexcept = default;

    /*Optional fields, publicly accessible*/
    int                    messageId{-1};
    userProps_t            userProps{userProps_t()};
    correlationDataProps_t correlationDataProps{correlationDataProps_t()};
    std::string            responseTopic{""};
    FormatIndicator        payloadFormatIndicator{FormatIndicator::Unspecified};
    std::string            payloadContentType{""};

    virtual std::string const& getTopic(void) const                 = 0;
    virtual payload_t const&   getPayload(void) const               = 0;
    virtual std::string        getPayloadCastedToString(void) const = 0;
    virtual bool               getRetained(void) const              = 0;
    virtual IMqttMessage::QOS  getQos(void) const                   = 0;
    virtual std::string        toString(void) const                 = 0;
};

using upMqttMessage_t = std::unique_ptr<IMqttMessage>;

class MqttMessageFactory final {
public:
    MqttMessageFactory() = delete;
    static upMqttMessage_t create(std::string const&              topic,
                                  IMqttMessage::payload_t const&& payload,
                                  IMqttMessage::QOS               qos,
                                  bool                            retain = false);
};
}  // namespace mqttclient