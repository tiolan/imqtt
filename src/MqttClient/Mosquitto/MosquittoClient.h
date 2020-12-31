/**
 * @file MosquitoClient.h
 * @author Timo Lange
 * @brief Class definition for Mosquitto wrapper
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

#include <mosquitto.h>

#include <atomic>
#include <mutex>
#include <random>

#include "IMqttClient.h"

namespace i_mqtt_client {
class MosquittoClient : public IMqttClient {
private:
    static std::atomic_uint counter;
    static std::string      libVersion;
    static std::mutex       libMutex;

    std::atomic_bool                  connected{false};
    mosquitto*                        pClient{nullptr};
    IMqttClient::InitializeParameters params;
    std::default_random_engine        rndGenerator{std::random_device()()};

    void       onConnectCb(struct mosquitto*, int, int, const mosquitto_property* props);
    void       onDisconnectCb(struct mosquitto*, int, const mosquitto_property*);
    void       onPublishCb(struct mosquitto*, int, int, const mosquitto_property*);
    void       onMessageCb(struct mosquitto*, const struct mosquitto_message*, const mosquitto_property*);
    void       onSubscribeCb(struct mosquitto*, int, int, const int*, const mosquitto_property*);
    void       onUnSubscribeCb(struct mosquitto*, int, const mosquitto_property*);
    void       onLog(struct mosquitto*, int, const char*);
    ReasonCode mosqRcToReasonCode(int, std::string const&) const;

    virtual std::string GetLibVersion(void) const noexcept override;
    virtual ReasonCode  ConnectAsync(void) override;
    virtual ReasonCode  DisconnectAsync(Mqtt5ReasonCode) override;
    virtual ReasonCode  SubscribeAsync(std::string const&, IMqttMessage::QOS, int*, bool) override;
    virtual ReasonCode  UnSubscribeAsync(std::string const&, int*) override;
    virtual ReasonCode  PublishAsync(upMqttMessage_t, int*) override;
    virtual bool        IsConnected(void) const noexcept override;

public:
    MosquittoClient(IMqttClient::InitializeParameters const&);
    virtual ~MosquittoClient() noexcept;
};

}  // namespace i_mqtt_client