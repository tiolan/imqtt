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
    IMqttMessage(void) = default;

public:
    using payload_t                  = const std::vector<unsigned char>;
    using userProps_t                = std::map<std::string, std::string>;
    using correlationDataProps_t     = std::vector<unsigned char>;
    virtual ~IMqttMessage() noexcept = default;

    virtual std::string const&            getTopic(void) const                 = 0;
    virtual payload_t const&              getPayload(void) const               = 0;
    virtual std::string                   getPayloadCastedToString(void) const = 0;
    virtual int                           getMessageId(void) const             = 0;
    virtual bool                          getRetained(void) const              = 0;
    virtual IMqttMessage::QOS             getQos(void) const                   = 0;
    virtual userProps_t const&            getUserProps(void) const             = 0;
    virtual correlationDataProps_t const& getCorrelationDataProps(void) const  = 0;
    virtual std::string                   toString(void) const                 = 0;
};

using upMqttMessage_t = std::unique_ptr<IMqttMessage>;

class MqttMessageFactory final {
public:
    static upMqttMessage_t create(
        std::string const&                          topic,
        IMqttMessage::payload_t const&&             payload,
        IMqttMessage::QOS                           qos,
        bool                                        retain               = false,
        IMqttMessage::userProps_t const&            userProps            = IMqttMessage::userProps_t(),
        IMqttMessage::correlationDataProps_t const& correlationDataProps = IMqttMessage::correlationDataProps_t(),
        int                                         messageId            = -1);

    static upMqttMessage_t create(
        std::string const&                          topic,
        void*                                       pPayload,
        size_t                                      size,
        IMqttMessage::QOS                           qos,
        bool                                        retain               = false,
        IMqttMessage::userProps_t const&            userProps            = IMqttMessage::userProps_t(),
        IMqttMessage::correlationDataProps_t const& correlationDataProps = IMqttMessage::correlationDataProps_t(),
        int                                         messageId            = -1);

private:
    MqttMessageFactory() = delete;
};
}  // namespace mqttclient