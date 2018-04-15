#define HOSTNAME  "Vaerelset"
#include "SHT21.h"
#include <ESP8266WiFi.h>
#include "SH1106.h" // alias for `#include "SSD1306Wire.h"`
#include "OLEDDisplayUi.h"
#include "myntp.h"
#include "mqtt.h"

#define TMPTOPIC  &("esp8266/" + String(HOSTNAME) + "/tmp/state")[0]
#define HUMTOPIC  &("esp8266/" + String(HOSTNAME) + "/hum/state")[0]

/*  Configuration values to be filled in as needed  */
#define SSID  "..";
#define PASS  "..";
#define WSHOST  "..";
#define WSPORT  81;
#define MQTT_CLIENT ESP.getChipId()
#define MQTT_SERVER "...";
#define MQTT_PORT 17332;
#define MQTT_USER "...";
#define MQTT_PASS "..";

#include <init.h> //  I use this library file to redefine the configuration values - remove if config data are defined abowe

const char *ssid = SSID;
const char *pass = PASS;
char* mqtt_client = "ESP123"; //String(ESP.getChipId());
char* mqtt_host = MQTT_SERVER;
uint16_t mqtt_port = MQTT_PORT;
char* mqtt_user = MQTT_USER;
char* mqtt_pass = MQTT_PASS;

AUTH options = { mqtt_client, mqtt_host, mqtt_port, mqtt_user, mqtt_pass };

class TIMER {
  private:
    uint32_t tcounter;
    uint32_t uval;
  public:
    bool oninterval(uint32_t val) {
      if (millis() > tcounter + val) {
        tcounter = millis();
        return true;
      }
      return false;
    }
};

TIMER mytimer;

SHT21 SHT21;

SH1106  display(0x3c, D2, D1);
OLEDDisplayUi ui ( &display );

float temp;
float hum;
char t_buf[20];
char h_buf[20];

int screenW = 128;
int screenH = 64;
int clockCenterX = screenW / 2;
int clockCenterY = screenH / 2; //((screenH - 16) / 2) + 16; // top yellow part is 16 px height
int clockRadius = 32;

uint32_t timer;

void onMessage(char* topic, char* value) {  //  MQTT message arraived
  Serial.printf("Time: %s, Topic: %s, value: %s\n", MyNTP.timeNow().c_str(), topic, value);
}

void onStatus(bool status) {  //  MQTT status changed (true=connected, false=disconnected)
  if (status) {
    char sub[] = "esp8266/"HOSTNAME"/#";
    mqtt.subscribe(sub);
    Serial.printf("Connected. Subscribing to: %s\n", sub);
  } else {
    Serial.println("Disconnected");
  }
}

void temperatureFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_24);
  display->drawString(clockCenterX + x , clockCenterY + y - 4, String(t_buf) + "°C" );
  display->setFont(ArialMT_Plain_16);
  display->drawString(clockCenterX + x , clockCenterY + y - 22, "Temperature" );
}

void humidityFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_24);
  display->drawString(clockCenterX + x , clockCenterY + y - 4, (String)h_buf + " %" );
  display->setFont(ArialMT_Plain_16);
  display->drawString(clockCenterX + x , clockCenterY + y - 22, "Humidity" );
}

void digitalClockFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_24);
  display->drawString(clockCenterX + x , clockCenterY + y - 4, MyNTP.timeNow() );
  display->setFont(ArialMT_Plain_16);
  display->drawString(clockCenterX + x , clockCenterY + y - 22, MyNTP.dateNow() );
}

void analogClockFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->drawCircle(clockCenterX + x, clockCenterY + y, 2);
  for ( int z = 0; z < 360; z = z + 30 ) {
    float angle = z ;
    angle = ( angle / 57.29577951 ) ; //Convert degrees to radians
    int x2 = ( clockCenterX + ( sin(angle) * clockRadius ) );
    int y2 = ( clockCenterY - ( cos(angle) * clockRadius ) );
    int x3 = ( clockCenterX + ( sin(angle) * ( clockRadius - ( clockRadius / 8 ) ) ) );
    int y3 = ( clockCenterY - ( cos(angle) * ( clockRadius - ( clockRadius / 8 ) ) ) );
    display->drawLine( x2 + x , y2 + y , x3 + x , y3 + y);
  }
  float angle = second() * 6 ;
  angle = ( angle / 57.29577951 ) ; //Convert degrees to radians
  int x3 = ( clockCenterX + ( sin(angle) * ( clockRadius - ( clockRadius / 5 ) ) ) );
  int y3 = ( clockCenterY - ( cos(angle) * ( clockRadius - ( clockRadius / 5 ) ) ) );
  display->drawLine( clockCenterX + x , clockCenterY + y , x3 + x , y3 + y);
  angle = minute() * 6 ;
  angle = ( angle / 57.29577951 ) ; //Convert degrees to radians
  x3 = ( clockCenterX + ( sin(angle) * ( clockRadius - ( clockRadius / 4 ) ) ) );
  y3 = ( clockCenterY - ( cos(angle) * ( clockRadius - ( clockRadius / 4 ) ) ) );
  display->drawLine( clockCenterX + x , clockCenterY + y , x3 + x , y3 + y);
  angle = hour() * 30 + int( ( minute() / 12 ) * 6 )   ;
  angle = ( angle / 57.29577951 ) ; //Convert degrees to radians
  x3 = ( clockCenterX + ( sin(angle) * ( clockRadius - ( clockRadius / 2 ) ) ) );
  y3 = ( clockCenterY - ( cos(angle) * ( clockRadius - ( clockRadius / 2 ) ) ) );
  display->drawLine( clockCenterX + x , clockCenterY + y , x3 + x , y3 + y);
}

FrameCallback frames[] = { analogClockFrame, digitalClockFrame, temperatureFrame, humidityFrame };

void clockOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) { }
OverlayCallback overlays[] = { clockOverlay };

void updateTemp() {
  static float old = 99.9;
  float val = round(SHT21.getTemperature() * 10.0) / 10.0;
  dtostrf(val, 3, 1, t_buf);
  if (val != old) {
    old = val;
    mqtt.publish(TMPTOPIC, t_buf, true);
  }
}

void updateHum() {
  static float old = 99.9;
  float val = round(SHT21.getHumidity());
  dtostrf(val, 2, 0, h_buf);
  if (val != old) {
    old = val;
    mqtt.publish(HUMTOPIC, h_buf, true);
  }
}

void setup() {
  //  WiFi connect
  Serial.begin(115200);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  //  get NTP clock
  MyNTP.set();
  ui.setTargetFPS(50);
  //ui.setActiveSymbol(activeSymbol);
  //ui.setInactiveSymbol(inactiveSymbol);
  // You can change this to
  // TOP, LEFT, BOTTOM, RIGHT
  ui.setIndicatorPosition(RIGHT);
  ui.setIndicatorDirection(LEFT_RIGHT);
  ui.setFrameAnimation(SLIDE_LEFT);
  ui.setFrames(frames, sizeof(frames) / sizeof(frames[0]));
  ui.setOverlays(overlays, sizeof(overlays) / sizeof(overlays[0]));
  ui.init();
  //  init MQTT
  mqtt.onMessage(onMessage);
  mqtt.onStatus(onStatus);
  mqtt.options = options;
  mqtt.begin();
  //  init temp/humidity module
  SHT21.begin();
  //  set screen
  display.flipScreenVertically();
}

void loop() {
  int remainingTimeBudget = ui.update();
  if (remainingTimeBudget > 0) {
    uint32_t ms = millis();
    mqtt.loop();
    if (mytimer.oninterval(5000)) {
      updateTemp();
      updateHum();
    }
    remainingTimeBudget -= (millis() - ms);
    if (remainingTimeBudget > 0) delay(remainingTimeBudget);
  }
}


