/**
 * @file IMqttClientCallbacks.h
 * @author Timo Lange
 * @brief Callback definitions for an abstract MqttClient
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

#include <map>
#include <string>

#include "IMqttMessage.h"

namespace i_mqtt_client {

class IMqttLogCallbacks {
private:
    static MqttLogInit_t mqttLibLogInitParams;

protected:
    IMqttLogCallbacks(void) = default;
    static void                 LogMqttLib(LogLevelLib, std::string const&);
    static MqttLogInit_t const& InitLogMqttLib(MqttLogInit_t const&);

public:
    virtual ~IMqttLogCallbacks() noexcept = default;
    /*To be overriden by user, if needed*/
    virtual void Log(LogLevel, std::string const&) const {/*by default, do not log*/};
};

class IMqttConnectionCallbacks {
protected:
    IMqttConnectionCallbacks(void) = default;

public:
    virtual ~IMqttConnectionCallbacks() noexcept = default;
    enum class ConnectionType { CONNECT, DISCONNECT };

    /*To be overriden by user, if needed*/
    virtual void OnConnectionStatusChanged(ConnectionType, Mqtt5ReasonCode) const {
        /*by default, do nothing*/
    };
};

class IMqttMessageCallbacks {
protected:
    IMqttMessageCallbacks(void) = default;

public:
    virtual ~IMqttMessageCallbacks() noexcept = default;

    /*To be overriden by user*/
    virtual void OnMqttMessage(upMqttMessage_t mqttMessage) const = 0;
};

class IMqttCommandCallbacks {
protected:
    IMqttCommandCallbacks(void) = default;
    using token_t               = int;

public:
    virtual ~IMqttCommandCallbacks() noexcept = default;

    /*To be overriden by user, if needed*/
    virtual void OnSubscribe(token_t) const {
        /*by default, do nothing*/
    };
    virtual void OnUnSubscribe(token_t) const {
        /*by default, do nothing*/
    };
    virtual void OnPublish(token_t, Mqtt5ReasonCode) const {
        /*for Paho and QOS0, the token is always 0*/
        /*by default, do nothing*/
    };
};

class IMqttClientCallbacks : public IMqttLogCallbacks,
                             public IMqttMessageCallbacks,
                             public IMqttConnectionCallbacks,
                             public IMqttCommandCallbacks {
protected:
    IMqttClientCallbacks(void) = default;

public:
    virtual ~IMqttClientCallbacks() noexcept = default;
};

struct MqttClientCallbacks final {
    MqttClientCallbacks(IMqttLogCallbacks const*        log,
                        IMqttMessageCallbacks const*    msg,
                        IMqttConnectionCallbacks const* con,
                        IMqttCommandCallbacks const*    cmd)
      // TODO: This is non-sense
      : log(log ? log : nullptr)
      , msg(msg ? msg : nullptr)
      , con(con ? con : nullptr)
      , cmd(cmd ? cmd : nullptr)
    {
    }
    IMqttLogCallbacks const*        log;
    IMqttMessageCallbacks const*    msg;
    IMqttConnectionCallbacks const* con;
    IMqttCommandCallbacks const*    cmd;
};


}  // namespace i_mqtt_client