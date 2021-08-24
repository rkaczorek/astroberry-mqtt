/*******************************************************************************
  Copyright(c) 2021 Radek Kaczorek  <rkaczorek AT gmail DOT com>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License version 2 as published by the Free Software Foundation.
 .
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.
 .
 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
*******************************************************************************/

#pragma once

#include <libindi/baseclient.h>

class mqttClient : public INDI::BaseClient
{
public:
    mqttClient();
    ~mqttClient() = default;

protected:
    void newDevice(INDI::BaseDevice *dp) override;
    void removeDevice(INDI::BaseDevice */*dp*/) override {}
    void newProperty(INDI::Property *property) override;
    void removeProperty(INDI::Property */*property*/) override {}
    void newSwitch(ISwitchVectorProperty *svp) override;
    void newNumber(INumberVectorProperty *nvp) override;
    void newText(ITextVectorProperty *tvp) override;
    void newLight(ILightVectorProperty *lvp) override;
    void newBLOB(IBLOB */*dp*/) override {}
    void newMessage(INDI::BaseDevice *dp, int messageID) override;
    void serverConnected() override {}
    void serverDisconnected(int /*exit_code*/) override {}

private:
	char* getDeviceType(uint16_t deviceType);
};

struct mosquitto *mosq = NULL;
char *mqtt_root_topic = NULL;
char mqtt_clientid[32] = {};
void handleSignal(int s);
void mqttConnectCallback(struct mosquitto *mosq, void *obj, int result);
void mqttDisconnectCallback(struct mosquitto *mosq, void *obj, int result);
void mqttSubscribeCallback(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos);
void mqttMsgCallback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message);
void mqttPublish(char* topic, char* msg);

