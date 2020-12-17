/**
 * @file IMqttClient.h
 * @author Timo Lange
 * @brief
 * @date 2020
 * @copyright Timo Lange
 */

#pragma once

#include <string>

#include "IMqttMessage.h"

namespace mqttclient {

class MqttMessage final : public IMqttMessage {
private:
    std::string            topic;
    payload_t              payload;
    int                    messageId;
    IMqttMessage::QOS      qos;
    bool                   retain;
    userProps_t            userProps;
    correlationDataProps_t correlationDataProps;

public:
    virtual inline std::string const&            getTopic(void) const noexcept override;
    virtual inline payload_t const&              getPayload(void) const noexcept override;
    virtual inline std::string                   getPayloadCastedToString(void) const override;
    virtual inline int                           getMessageId(void) const noexcept override;
    virtual inline bool                          getRetained(void) const noexcept override;
    virtual inline QOS                           getQos(void) const noexcept override;
    virtual inline userProps_t const&            getUserProps(void) const noexcept override;
    virtual inline correlationDataProps_t const& getCorrelationDataProps(void) const noexcept override;
    virtual inline std::string                   toString(void) const noexcept override;

    MqttMessage(std::string const&            topic,
                payload_t const&              payload,
                QOS                           qos,
                userProps_t const&            userProps,
                int                           messageId,
                bool                          retain,
                correlationDataProps_t const& correlationDataProps);
};
}  // namespace mqttclient