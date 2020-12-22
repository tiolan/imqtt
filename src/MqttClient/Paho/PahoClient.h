/**
 * @file PahoClient.h
 * @author Timo Lange
 * @brief
 * @date 2020
 * @copyright Timo Lange
 */

#pragma once

#include <atomic>
#include <mutex>
#include <random>
#include <string>

#include "IMqttClient.h"
#include "MQTTAsync.h"

namespace mqttclient {
class PahoClient : public mqttclient::IMqttClient {
private:
    static std::atomic_uint counter;
    static std::string      libVersion;
    static std::mutex       libMutex;

    MQTTAsync                         pClient{nullptr};
    IMqttClient::InitializeParameters params;
    std::default_random_engine        rndGenerator{std::random_device()()};

    virtual std::string      GetLibVersion(void) const noexcept override;
    virtual void             ConnectAsync(void) override;
    virtual void             Disconnect(void) override;
    virtual RetCodes         SubscribeAsync(std::string const&, IMqttMessage::QOS, int*, bool) override;
    virtual RetCodes         UnSubscribeAsync(std::string const&, int*) override;
    virtual RetCodes         PublishAsync(upMqttMessage_t, int*) override;
    virtual ConnectionStatus GetConnectionStatus(void) const noexcept override;

    void onSuccessCb(MQTTAsync_successData5* data);
    void onFailureCb(MQTTAsync_failureData5* data);

    int onMessageCb(char*, int, MQTTAsync_message*) const;

public:
    PahoClient(IMqttClient::InitializeParameters const&);
    virtual ~PahoClient() noexcept;
};
}  // namespace mqttclient
