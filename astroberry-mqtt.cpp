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
// https://gist.github.com/piccaso/f463f6ad134a8a2479e62e6e2349e7c0
// https://gist.github.com/evgeny-boger/8cefa502779f98efaf24

#include "astroberry-mqtt.h"
#include "libindi/basedevice.h" // http://www.indilib.org/api/
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <mosquitto.h> // https://mosquitto.org/api/files/mosquitto-h.html
#include "config.h"

#define CONFIG_FILE "/etc/astroberry-mqtt.conf"
#define INDI_HOST "astroberry-server.local"
#define INDI_PORT 7624
#define MQTT_HOST "nasa.local"
#define MQTT_PORT 1883
#define MQTT_USER ""
#define MQTT_PASS ""
#define MQTT_ROOT "observatory"

static std::unique_ptr<mqttClient> indi_client(new mqttClient());

int main(int argc, char **argv)
{

	char * indi_host = INDI_HOST;
	int indi_port = INDI_PORT;
	char * mqtt_host = MQTT_HOST;
	int mqtt_port = MQTT_PORT;
	char* mqtt_user = MQTT_USER;
	char* mqtt_pass = MQTT_PASS;
	mqtt_root_topic = MQTT_ROOT;
	int mqtt_keepalive = 60;
	bool mqtt_clean_session = true;

/*
	// parse config file
	std::ifstream cFile (CONFIG_FILE);
	if (cFile.is_open())
	{
		IDLog("Reading configuration from %s\n", CONFIG_FILE);
        std::string line;
        while(getline(cFile, line)){
            line.erase(std::remove_if(line.begin(), line.end(), isspace),
                                 line.end());
            if(line[0] == '#' || line.empty())
                continue;
            auto delimiterPos = line.find("=");
            const char* name = line.substr(0, delimiterPos).c_str();
            char* value = (char*) line.substr(delimiterPos + 1).c_str();
            
			if (!strcmp(name, "indi_host")) 
			{
				indi_host = value;
				IDLog("%s\n", indi_host);
			}

			if (!strcmp(name, "indi_port"))
			{
				indi_port = atoi(value);
				IDLog("%i\n", indi_port);
			}

			if (!strcmp(name, "mqtt_host"))
			{
				mqtt_host = value;
				IDLog("%s\n", mqtt_host);
			}

			if (!strcmp(name, "mqtt_port"))
			{
				mqtt_port = atoi(value);
				IDLog("%i\n", mqtt_port);
			}

			if (!strcmp(name, "mqtt_user"))
			{
				mqtt_user = value;
				IDLog("%s\n", mqtt_user);
			}

			if (!strcmp(name, "mqtt_pass"))
			{
				mqtt_pass = value;
				IDLog("%s\n", mqtt_pass);
			}

			if (!strcmp(name, "mqtt_root_topic"))
			{
				mqtt_root_topic = value;
				IDLog("%s\n", mqtt_root_topic);
			}

			if (!strcmp(name, "mqtt_keepalive"))
			{
				mqtt_keepalive = atoi(value);
				IDLog("%s\n", mqtt_keepalive);
			}

			if (!strcmp(name, "mqtt_clean_session"))
			{
				if (!strcmp(value, "true"))
				{
					mqtt_clean_session = true;
				} else {
					mqtt_clean_session = false;
				}
				IDLog("%s\n", mqtt_clean_session);
			}
         
            std::cout << name << " " << value << '\n';
        }
	} else {
		IDLog("Can't open %s. Using defaults\n", CONFIG_FILE);
	}
*/

	// parse arguments
	//
	//  TODO
	//

	signal(SIGINT, handleSignal);
	signal(SIGTERM, handleSignal);

    indi_client->setServer(indi_host, indi_port);
    indi_client->connectServer();
    //indi_client->setBLOBMode(B_NEVER, nullptr, nullptr);

	mosquitto_lib_init();
	snprintf(mqtt_clientid, 31, "astroberry-mqtt-%d", getpid());
	mosq = mosquitto_new(mqtt_clientid, mqtt_clean_session, 0);

	if (mosq)
	{
		mosquitto_connect_callback_set(mosq, mqttConnectCallback);
		mosquitto_disconnect_callback_set(mosq, mqttDisconnectCallback);
		mosquitto_subscribe_callback_set(mosq, mqttSubscribeCallback);
		mosquitto_message_callback_set(mosq, mqttMsgCallback);

		mosquitto_username_pw_set(mosq, mqtt_user, mqtt_pass);

		int rc = mosquitto_connect(mosq, mqtt_host, mqtt_port, mqtt_keepalive);
		
		mosquitto_subscribe(mosq, NULL, "environment/pws-b5f0/temperature", 0);
		
		while (1)
		{
			//mosquitto_reconnect(mosq);
			mosquitto_loop(mosq, -1, 1);
			sleep(3);
		}
	}
    
    return 0;
}

mqttClient::mqttClient()
{
}
	
void mqttClient::newDevice(INDI::BaseDevice *dp)
{
	//IDLog("%s\n", dp->getDeviceName());
	
	//uint16_t deviceType = dp->getDriverInterface();	
	//IDLog("%s\n", getDeviceType(deviceType));
}

void mqttClient::newProperty(INDI::Property *property)
{
	char topic[1024];
	char msg[128];
	//IDLog("%s/%s\n", property->getDeviceName(), property->getName());

	if (property->getType() == INDI_SWITCH)
	{
		ISwitchVectorProperty *svp = property->getSwitch();
		for (int i=0; i < svp->nsp; i++)
		{
			//IDLog("%s/%s/%s:\t%s\n", svp->device, svp->name, svp->sp[i].name, sstateStr(svp->sp[i].s));
			sprintf(topic, "%s/%s/%s", svp->device, svp->name, svp->sp[i].name);
			sprintf(msg, "%s", sstateStr(svp->sp[i].s));
			mqttPublish(topic, msg);
		}
	}

	if (property->getType() == INDI_NUMBER)
	{
		INumberVectorProperty *nvp = property->getNumber();
		for (int i=0; i < nvp->nnp; i++)
		{
			//IDLog("%s/%s/%s:\t%4.2f\n", nvp->device, nvp->name, nvp->np[i].name, nvp->np[i].value);
			sprintf(topic, "%s/%s/%s", nvp->device, nvp->name, nvp->np[i].name);
			sprintf(msg, "%4.2f", nvp->np[i].value);
			mqttPublish(topic, msg);
		}
	}

	if (property->getType() == INDI_TEXT)
	{
		ITextVectorProperty *tvp = property->getText();
		for (int i=0; i < tvp->ntp; i++)
		{
			//IDLog("%s/%s/%s:\t%s\n", tvp->device, tvp->name, tvp->tp[i].name, tvp->tp[i].text);
			sprintf(topic, "%s/%s/%s", tvp->device, tvp->name, tvp->tp[i].name);
			sprintf(msg, "%s", tvp->tp[i].text);
			mqttPublish(topic, msg);
		}
	}

	if (property->getType() == INDI_LIGHT)
	{
		ILightVectorProperty *lvp = property->getLight();
		for (int i=0; i < lvp->nlp; i++)
		{
			//IDLog("%s/%s/%s:\t%i\n", lvp->device, lvp->name, lvp->lp[i].name, lvp->lp[i].s);
			sprintf(topic, "%s/%s/%s", lvp->device, lvp->name, lvp->lp[i].name);
			sprintf(msg, "%i", lvp->lp[i].s);
			mqttPublish(topic, msg);
		}
	}
}

void mqttClient::newSwitch(ISwitchVectorProperty *svp)
{
	char topic[1024];
	char msg[128];
	for (int i=0; i < svp->nsp; i++)
	{
		sprintf(topic, "%s/%s/%s", svp->device, svp->name, svp->sp[i].name);
		sprintf(msg, "%s", sstateStr(svp->sp[i].s));
		mqttPublish(topic, msg);
	}
}

void mqttClient::newNumber(INumberVectorProperty *nvp)
{
	char topic[1024];
	char msg[128];
	for (int i=0; i < nvp->nnp; i++)
	{
		sprintf(topic, "%s/%s/%s", nvp->device, nvp->name, nvp->np[i].name);
		sprintf(msg, "%4.2f", nvp->np[i].value);
		mqttPublish(topic, msg);
	}
}

void mqttClient::newText(ITextVectorProperty *tvp)
{
	char topic[1024];
	char msg[128];
	for (int i=0; i < tvp->ntp; i++)
	{
		sprintf(topic, "%s/%s/%s", tvp->device, tvp->name, tvp->tp[i].name);
		sprintf(msg, "%s", tvp->tp[i].text);
		mqttPublish(topic, msg);
	}
}

void mqttClient::newLight(ILightVectorProperty *lvp)
{
	char topic[1024];
	char msg[128];
	for (int i=0; i < lvp->nlp; i++)
	{
		sprintf(topic, "%s/%s/%s", lvp->device, lvp->name, lvp->lp[i].name);
		sprintf(msg, "%i", lvp->lp[i].s);
		mqttPublish(topic, msg);
	}
}

void mqttClient::newMessage(INDI::BaseDevice *dp, int messageID)
{
    IDLog("%s\n", dp->messageQueue(messageID).c_str());
}

char* mqttClient::getDeviceType(uint16_t deviceType)
{
	char* strDevice;
	switch (deviceType) {
		case 0:
			strDevice =  (char*) "GENERAL";
			break;
		case (1 << 0):
			strDevice =  (char*) "TELESCOPE";
			break;
		case (1 << 1):
			strDevice =  (char*) "CCD";
			break;
		case (1 << 2):
			strDevice =  (char*) "GUIDER";
			break;
		case (1 << 3):
			strDevice =  (char*) "FOCUSER";
			break;
		case (1 << 4):
			strDevice =  (char*) "FILTER";
			break;
		case (1 << 5):
			strDevice =  (char*) "DOME";
			break;
		case (1 << 6):
			strDevice =  (char*) "GPS";
			break;
		case (1 << 7):
			strDevice =  (char*) "WEATHER";
			break;
		case (1 << 8):
			strDevice =  (char*) "AO";
			break;
		case (1 << 9):
			strDevice =  (char*) "DUSTCAP";
			break;
		case (1 << 10):
			strDevice =  (char*) "LIGHTBOX";
			break;
		case (1 << 11):
			strDevice =  (char*) "DETECTOR";
			break;
		case (1 << 12):
			strDevice =  (char*) "ROTATOR";
			break;
		case (1 << 13):
			strDevice =  (char*) "SPECTROGRAPH";
			break;
		case (1 << 14):
			strDevice =  (char*) "CORRELATOR";
			break;
		case (1 << 15):
			strDevice =  (char*) "AUX";
			break;
		default:
			strDevice =  (char*) "AUX";
	}
	return strDevice;
}

void handleSignal(int s)
{
	switch (s) {
		case (SIGINT):
			mosquitto_destroy(mosq);
			mosquitto_lib_cleanup();
			exit(0);
			break;
		case (SIGTERM):
			mosquitto_destroy(mosq);
			mosquitto_lib_cleanup();
			exit(0);
			break;
		default:
			mosquitto_destroy(mosq);
			mosquitto_lib_cleanup();
			exit(0);
	}
}

void mqttConnectCallback(struct mosquitto *mosq, void *obj, int result)
{
	IDLog("Connected to MQTT broker\n");
	//printf("connect callback, rc=%d\n", result);
}

void mqttDisconnectCallback(struct mosquitto *mosq, void *obj, int result)
{
	IDLog("Disconnected from MQTT broker\n");
	//printf("disconnect callback, rc=%d\n", result);
}

void mqttSubscribeCallback(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos)
{
	IDLog("Subscribed to MQTT topic\n");
	//printf("subscribe callback, rc=%d\n", mid);
}

void mqttMsgCallback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
	bool match = 0;
	IDLog("Received MQTT message '%.*s' for topic '%s'\n", message->payloadlen, (char*) message->payload, message->topic);
	//printf("got message '%.*s' for topic '%s'\n", message->payloadlen, (char*) message->payload, message->topic);

	mosquitto_topic_matches_sub("environment/pws-b5f0/temperature", message->topic, &match);
	if (match) {
		IDLog("Received temperature from pws-b5f0\n");
		//printf("got temperature from pws-b5f0\n");
	}
}

void mqttPublish(char topic[1024], char msg[128])
{
	char msgtopic[1024] = {};
	
	/*
	// do we need to sanitize topic names?
	char cleanTopic[1024];
	int j = 0;

    for(int i = 0; topic[i] != '\0'; ++i)
    {
        if ((topic[i] >= 'a' && topic[i]<='z') || (topic[i] >= 'A' && topic[i]<='Z') || (topic[i] == '/') || (topic[i] == '_') || (topic[i] == ' '))
        {
            cleanTopic[j++] = topic[i]; 
        }
    }
    cleanTopic[j] = '\0';
    */

	//strcpy (msgtopic, MQTT_ROOT);
	//strcat (msgtopic, "/");
	//strcat (msgtopic, topic);
	sprintf(msgtopic, "%s/%s", mqtt_root_topic, topic);

	mosquitto_publish(mosq, NULL, msgtopic, strlen(msg), msg, 0, 0);

	//IDLog("%s/%s\t%s\n", MQTT_ROOT, topic, msg);
	
}
