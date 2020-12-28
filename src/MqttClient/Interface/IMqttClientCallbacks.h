/**
 * @file IMqttClientCallbacks.h
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

#pragma once

#include "IMqttMessage.h"

namespace mqttclient {
class IMqttLogCallbacks {
protected:
    IMqttLogCallbacks(void) = default;

public:
    virtual ~IMqttLogCallbacks() noexcept = default;

    enum class LogLevel { Trace, Debug, Info, Warning, Error, Fatal };
    /*To be overriden by user, if needed*/
    virtual void Log(LogLevel, std::string const&) const {/*by default, do not log*/};
};

class IMqttConnectionCallbacks {
protected:
    IMqttConnectionCallbacks(void) = default;

public:
    virtual ~IMqttConnectionCallbacks() noexcept = default;
    enum class ConnectionStatus { Connected, Disconnected };

    /*To be overriden by user, if needed*/
    virtual void
    OnConnectionStatusChanged(ConnectionStatus status) const
    {
        (void)status; /*by default, do nothing*/
    };
};

class IMqttMessageCallbacks {
protected:
    IMqttMessageCallbacks(void) = default;

public:
    virtual ~IMqttMessageCallbacks() noexcept = default;

    /*To be overriden by user*/
    virtual void OnMqttMessage(upMqttMessage_t mqttMessage) const = 0;
    /*To be overriden by user, if needed*/
    virtual void
    OnSubscribe(int token) const
    {
        (void)token; /*by default, do nothing*/
    };
    virtual void
    OnUnSubscribe(int token) const
    {
        (void)token; /*by default, do nothing*/
    };
    virtual void
    OnPublish(int token) const
    {
        /*for Paho and QOS0, the token is always 0*/
        (void)token; /*by default, do nothing*/
    };
};

class IMqttClientCallbacks : public IMqttLogCallbacks, public IMqttMessageCallbacks, public IMqttConnectionCallbacks {
protected:
    IMqttClientCallbacks(void) = default;

public:
    virtual ~IMqttClientCallbacks() noexcept = default;
};

struct MqttClientCallbacks final {
    MqttClientCallbacks(IMqttLogCallbacks const*        log,
                        IMqttMessageCallbacks const*    msg,
                        IMqttConnectionCallbacks const* con)
      : log(log ? log : nullptr)
      , msg(msg ? msg : nullptr)
      , con(con ? con : nullptr)
    {
    }
    IMqttLogCallbacks const*        log;
    IMqttMessageCallbacks const*    msg;
    IMqttConnectionCallbacks const* con;
};


}  // namespace mqttclient