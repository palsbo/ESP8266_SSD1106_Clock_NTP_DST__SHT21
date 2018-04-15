#ifndef MQTT_H
#define MQTT_H

#include <PubSubClient.h>
WiFiClient espClient;
PubSubClient client(espClient);

typedef void (*mqtt_wcallback1)(bool status);
typedef void (*mqtt_wcallback2)(char* topic, char* payload);

mqtt_wcallback1 wcb1 = [](bool status) {
  Serial.println(status ? "MQTT Connected" : "MQTT Disconnected");
};

mqtt_wcallback2 wcb2 = [](char* topic, char* value) {
};

void callback(char* topic, byte* payload, unsigned int length) {
  char value[length + 1];
  memcpy(value, payload, length);
  value[length] = 0;
  if (wcb2) wcb2(topic, value);
}

struct AUTH {
  char * clientname;
  char * server;
  int port;
  char * user;
  char * pass;
};

class {
  private:
    bool stat = false;
    void reconnect() {
      if (wcb1 && stat) wcb1(false);
      stat = false;
      if (client.connect(options.clientname, options.user, options.pass)) {
        if (wcb1 && !stat) wcb1(true);
        stat = true;
      }
    }
  public:
    AUTH options;
    void begin() {
      begin(options);
    }
    void begin(AUTH _au) {
      options = _au;
      String id = (String)options.clientname + "_" + String(ESP.getChipId());
      options.clientname = &id[0];
      client.setServer(options.server, options.port);
      client.setCallback(callback);
    }
    void loop() {
      if (!client.connected()) {
        reconnect();
      }
      client.loop();  //  check for mqtt connection
    }
    bool connected() {
      return client.connected();
    }
    boolean subscribe(const char* topic) {
      client.subscribe(topic);
    };
    void onMessage(mqtt_wcallback2 cb) {
      wcb2 = cb;
    }
    boolean publish(const char* topic, const char* payload) {
      return client.publish(topic, payload);
    };
    boolean publish(const char* topic, const char* payload, boolean retained) {
      return client.publish(topic, payload, retained);
    };
    void onStatus(mqtt_wcallback1 cb) {
      wcb1 = cb;
    }
} mqtt;

#endif
