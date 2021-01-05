/**
 * @file MosquittoClient.h
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
    static std::mutex       libMutex;

    std::atomic_bool                  connected{false};
    mosquitto*                        pMosqClient{nullptr};
    IMqttClient::InitializeParameters params;

    void       onConnectCb(struct mosquitto const*, int, int, mosquitto_property const*);
    void       onDisconnectCb(struct mosquitto const*, int, mosquitto_property const*);
    void       onPublishCb(struct mosquitto const*, int, int, mosquitto_property const*) const;
    void       onMessageCb(struct mosquitto const*, struct mosquitto_message const*, mosquitto_property const*) const;
    void       onSubscribeCb(struct mosquitto const*, int, int, int const*, mosquitto_property const*) const;
    void       onUnSubscribeCb(struct mosquitto const*, int, mosquitto_property const*) const;
    void       onLog(struct mosquitto const*, int, char const*) const;
    ReasonCode mosqRcToReasonCode(int, std::string const&) const;

    ReasonCode ConnectAsync(void) override;
    ReasonCode DisconnectAsync(Mqtt5ReasonCode) override;
    ReasonCode SubscribeAsync(std::string const&, IMqttMessage::QOS, int*, bool) override;
    ReasonCode UnSubscribeAsync(std::string const&, int*) override;
    ReasonCode PublishAsync(upMqttMessage_t, int*) override;
    bool       IsConnected(void) const noexcept override;

public:
    MosquittoClient(IMqttClient::InitializeParameters const&,
                    IMqttMessageCallbacks const*,
                    IMqttLogCallbacks const*,
                    IMqttCommandCallbacks const*,
                    IMqttConnectionCallbacks const*);
    virtual ~MosquittoClient() noexcept;
};

}  // namespace i_mqtt_client