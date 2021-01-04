/**
 * @file PahoClient.h
 * @author Timo Lange
 * @brief Class definition for Paho library wrapper
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

#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

#include "IMqttClient.h"
#include "MQTTAsync.h"

namespace i_mqtt_client {
class PahoClient : public IMqttClient {
private:
    struct Context final {
        PahoClient* pThis;
        void*       pContext;
        Context(PahoClient* pClient, void* pContext)
          : pThis(pClient)
          , pContext(pContext){};
    };
    static std::once_flag initFlag;

    InitializeParameters params;
    MQTTAsync            pClient{nullptr};

    virtual ReasonCode ConnectAsync(void) override;
    virtual ReasonCode DisconnectAsync(Mqtt5ReasonCode) override;
    virtual ReasonCode SubscribeAsync(std::string const&, IMqttMessage::QOS, int*, bool) override;
    virtual ReasonCode UnSubscribeAsync(std::string const&, int*) override;
    virtual ReasonCode PublishAsync(upMqttMessage_t, int*) override;
    virtual bool       IsConnected(void) const noexcept override;

    void       printDetailsOnSuccess(std::string const&, MQTTAsync_successData5*);
    void       printDetailsOnFailure(std::string const&, MQTTAsync_failureData5*);
    ReasonCode pahoRcToReasonCode(int, std::string const&) const;
    int        onMessageCb(char*, int, MQTTAsync_message*) const;

public:
    PahoClient(IMqttClient::InitializeParameters const&,
               IMqttMessageCallbacks const*,
               IMqttLogCallbacks const*,
               IMqttCommandCallbacks const*,
               IMqttConnectionCallbacks const*);
    virtual ~PahoClient() noexcept;
};
}  // namespace i_mqtt_client
