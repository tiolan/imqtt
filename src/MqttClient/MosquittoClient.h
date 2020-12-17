/**
 * @file IMqttClient.h
 * @author Timo Lange
 * @brief
 * @date 2020
 * @copyright Timo Lange
 */

#include <mosquitto.h>

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

#include "IMqttClient.h"

namespace mqttclient {
class MosquittoClient : public IMqttClient {
private:
    static std::atomic_uint counter;
    static std::string      libVersion;
    static std::mutex       libMutex;

    std::atomic_bool            connected{false};
    std::string                 address;
    int                         port;
    std::string                 id;
    bool                        cleanSession{true};
    mosquitto*                  pClient{nullptr};
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

public:
    MosquittoClient(std::string, int, std::string, MqttClientCallbacks const&);
    virtual ~MosquittoClient() noexcept;

    virtual inline std::string GetLibVersion(void) const noexcept override;

    virtual void             ConnectAsync(void) override;
    virtual void             Disconnect(void) override;
    virtual RetCodes         Subscribe(std::string const&, mqttclient::IMqttMessage::QOS, bool) override;
    virtual RetCodes         UnSubscribe(std::string const&) override;
    virtual RetCodes         Publish(upMqttMessage_t) override;
    virtual ConnectionStatus GetConnectionStatus(void) const noexcept override;
};

}  // namespace mqttclient