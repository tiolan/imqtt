/**
 * @file IMqttClient.h
 * @author Timo Lange
 * @brief
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
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <random>
#include <thread>

#include "IMqttClient.h"

namespace mqttclient {
class MosquittoClient : public IMqttClient {
private:
    static std::atomic_uint counter;
    static std::string      libVersion;
    static std::mutex       libMutex;

    std::atomic_bool                  connected{false};
    mosquitto*                        pClient{nullptr};
    IMqttClient::InitializeParameters params;
    std::default_random_engine        rndGenerator{std::random_device()()};

    std::mutex                  messageDispatcherMutex;
    std::thread                 messageDispatcherThread;
    std::queue<upMqttMessage_t> messageDispatcherQueue;
    std::condition_variable     messageDispatcherAwaiter;
    std::atomic_bool            messageDispatcherExit{false};

    void messageDispatcherWorker(void);
    void dispatchMessage(upMqttMessage_t&&);
    void onConnectCb(struct mosquitto*, int, int, const mosquitto_property* props);
    void onDisconnectCb(struct mosquitto*, int, const mosquitto_property*);
    void onPublishCb(struct mosquitto*, int, int, const mosquitto_property*);
    void onMessageCb(struct mosquitto*, const struct mosquitto_message*, const mosquitto_property*);
    void onSubscribeCb(struct mosquitto*, int, int, const int*, const mosquitto_property*);
    void onUnSubscribeCb(struct mosquitto*, int, const mosquitto_property*);
    void onLog(struct mosquitto*, int, const char*);

    virtual std::string      GetLibVersion(void) const noexcept override;
    virtual void             ConnectAsync(void) override;
    virtual void             Disconnect(void) override;
    virtual RetCodes         SubscribeAsync(std::string const&, IMqttMessage::QOS, int*, bool) override;
    virtual RetCodes         UnSubscribeAsync(std::string const&, int*) override;
    virtual RetCodes         PublishAsync(upMqttMessage_t, int*) override;
    virtual ConnectionStatus GetConnectionStatus(void) const noexcept override;

public:
    MosquittoClient(IMqttClient::InitializeParameters const&);
    virtual ~MosquittoClient() noexcept;
};

}  // namespace mqttclient