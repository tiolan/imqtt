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
public:
    virtual inline std::string const& getTopic(void) const noexcept override;
    virtual inline payload_t const&   getPayload(void) const noexcept override;
    virtual inline std::string        getPayloadCastedToString(void) const override;
    virtual inline bool               getRetained(void) const noexcept override;
    virtual inline QOS                getQos(void) const noexcept override;
    virtual inline std::string        toString(void) const noexcept override;

    MqttMessage(std::string const&, payload_t const&, QOS, bool);
};
}  // namespace mqttclient