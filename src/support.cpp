#include "support.h"
#include <WiFiManager.h>
#include <Arduino.h>

WiFiClient espClient;
WiFiManager wifiManager;
PubSubClient MQTTclient(espClient);
int WIFI_lost = 0;
int MQTT_lost = 0;
int WiFiStatus = WIFI_CONNECT_TIMEOUT;
uint networksFound = 0;
unsigned long WiFiTimeoutMillis;

struct rising_edge_cnt_struct{
  volatile uint32_t SCK = 0;
  volatile uint32_t MOSI = 0;
  volatile uint32_t MISO = 0;
} rising_edge_cnt;

IRAM_ATTR void handleInterrupt_SCK() {
  rising_edge_cnt.SCK++;
}

IRAM_ATTR void handleInterrupt_MOSI() {
  rising_edge_cnt.MOSI++;
}

IRAM_ATTR void handleInterrupt_MISO() {
  rising_edge_cnt.MISO++;
}

void MeasureFrequency() {  // measure the frequency on the pins
  pinMode(SCK_PIN, INPUT);
  pinMode(MOSI_PIN, INPUT);
  pinMode(MISO_PIN, INPUT);
  Serial.println(F("Measure frequency for SCK, MOSI and MISO pin"));
  attachInterrupt(digitalPinToInterrupt(SCK_PIN), handleInterrupt_SCK, RISING);
  attachInterrupt(digitalPinToInterrupt(MOSI_PIN), handleInterrupt_MOSI, RISING);
  attachInterrupt(digitalPinToInterrupt(MISO_PIN), handleInterrupt_MISO, RISING);
  unsigned long starttimeMicros = micros();
  while (micros() - starttimeMicros < 1000000);
  detachInterrupt(SCK_PIN);
  detachInterrupt(MOSI_PIN);
  detachInterrupt(MISO_PIN);

  Serial.printf_P(PSTR("SCK frequency=%iHz (expected: >3000Hz) "), rising_edge_cnt.SCK);
  if (rising_edge_cnt.SCK > 3000)
    Serial.println(F("o.k."));
  else
    Serial.println(F("out of range!"));

  Serial.printf("MOSI frequency=%iHz (expected: <SCK frequency) ", rising_edge_cnt.MOSI);
  if ((rising_edge_cnt.MOSI > 30) & (rising_edge_cnt.MOSI < rising_edge_cnt.SCK))
    Serial.println(F("o.k."));
  else
    Serial.println(F("out of range!"));

  Serial.printf("MISO frequency=%iHz (expected: ~0Hz) ", rising_edge_cnt.MISO);
  if (rising_edge_cnt.MISO <= 10) {
    Serial.println(F("o.k."));
  }
  else {
    Serial.println(F("out of range!"));
    delay(1000);
    while (1);
  }
}

void initWiFi(){
  String nodeID= HOSTNAME + String(ESP.getChipId());
  wifiManager.setConfigPortalTimeout(180);
  wifiManager.autoConnect(nodeID.c_str(), WIFI_PASSWORD);

  //WiFi.setAutoReconnect(true);
  //WiFi.persistent(true);
  //wifiManager.resetSettings();
}

int MQTTreconnect() {
  char strtmp[50];
  static int reconnect_trials=0;
  //Serial.printf("MQTTreconnect(): (MQTTclient.state=%i), WiFi.status()=%i networksFound=%i ...\n", MQTTclient.state(), WiFi.status(), networksFound);
  if(!MQTTclient.connected()) {
    Serial.printf("MQTTreconnect(): Attempting MQTT connection (MQTTclient.state=%i), WiFi.status()=%i ...\n", MQTTclient.state(), WiFi.status());  // state(), see https://pubsubclient.knolleary.net/api#state
    if(reconnect_trials++>9){                                                                                                                       // WiFi.status()=3=connected, see https://realglitch.com/2018/07/arduino-wifi-status-codes/
      Serial.printf("MQTTreconnect(): reconnect_trials=%i\n", reconnect_trials);
      WiFi.disconnect(); // work around for https://github.com/esp8266/Arduino/issues/7432
      reconnect_trials=0;
    }

    if (MQTTclient.connect(HOSTNAME, MQTT_USER, MQTT_PASSWORD, MQTT_PREFIX TOPIC_CONNECTED, 0, true, PAYLOAD_CONNECTED_FALSE)) {
      Serial.println((" connected"));
      Serial.printf("MQTTclient.connected=%i\n", MQTTclient.connected());
      reconnect_trials=0;

#ifdef USE_HA_DISCOVERY
      publishHADiscovery();
#endif

      output_P((ACStatus)type_status, PSTR(TOPIC_CONNECTED), PSTR(PAYLOAD_CONNECTED_TRUE));
      output_P((ACStatus)type_status, PSTR(TOPIC_VERSION), PSTR(VERSION));

      itoa(WiFi.RSSI(), strtmp, 10);
      output_P((ACStatus)type_status, PSTR(TOPIC_RSSI), strtmp);
      itoa(WIFI_lost, strtmp, 10);
      output_P((ACStatus)type_status, PSTR(TOPIC_WIFI_LOST), strtmp);
      itoa(MQTT_lost, strtmp, 10);
      output_P((ACStatus)type_status, PSTR(TOPIC_MQTT_LOST), strtmp);
      WiFi.BSSIDstr().toCharArray(strtmp, 20);
      output_P((ACStatus)type_status, PSTR(TOPIC_WIFI_BSSID), strtmp);

      itoa(rising_edge_cnt.SCK, strtmp, 10);
      output_P((ACStatus)type_status, PSTR(TOPIC_FSCK), strtmp);
      itoa(rising_edge_cnt.MOSI, strtmp, 10);
      output_P((ACStatus)type_status, PSTR(TOPIC_FMOSI), strtmp);
      itoa(rising_edge_cnt.MISO, strtmp, 10);
      output_P((ACStatus)type_status, PSTR(TOPIC_FMISO), strtmp);
      
      MQTTclient.subscribe(MQTT_SET_PREFIX "#");
      return MQTT_RECONNECTED;
    }
    else {
      Serial.print(F(" reconnect failed, reason "));
      itoa(MQTTclient.state(), strtmp, 10);
      Serial.print(strtmp);
      Serial.print(", WiFi status: ");
      Serial.println(WiFi.status());
      return MQTT_NOT_CONNECTED;
    }
  }
  MQTTclient.loop();
  return MQTT_CONNECTED;
}

void publish_cmd_ok() {
  output_P((ACStatus)type_status, PSTR(TOPIC_CMD_RECEIVED), PSTR(PAYLOAD_CMD_OK));
}
void publish_cmd_unknown() {
  output_P((ACStatus)type_status, PSTR(TOPIC_CMD_RECEIVED), PSTR(PAYLOAD_CMD_UNKNOWN));
}
void publish_cmd_invalidparameter() {
  output_P((ACStatus)type_status, PSTR(TOPIC_CMD_RECEIVED), PSTR(PAYLOAD_CMD_INVALID_PARAMETER));
}
// Publish the AC mode of operation state over MQTT so that other systems can syncronize with MHI-AC-ctrl
void publish_ac_state_update() {


}

#ifdef USE_HA_DISCOVERY
#include <ArduinoJson.h>

static bool publishJsonRetained(const char* topic, DynamicJsonDocument& doc) {
  unsigned int len = measureJson(doc);
  if (MQTTclient.beginPublish(topic, len, true)) {
    serializeJson(doc, MQTTclient);
    return MQTTclient.endPublish();
  }
  return false;
}

void publishHADiscovery() {
  if (!MQTTclient.connected()) return;

  // --- Climate entity discovery ---
  {
    DynamicJsonDocument doc(2560);

    doc["name"] = HOSTNAME;
    doc["unique_id"] = String(HOSTNAME) + "_climate";
    doc["object_id"] = String(HOSTNAME) + "_climate";

    // Modes
    JsonArray modes = doc.createNestedArray("modes");
    modes.add("auto");
    modes.add("dry");
    modes.add("cool");
    modes.add("fan_only");
    modes.add("heat");
    modes.add("off");
    doc["mode_command_topic"] = MQTT_SET_PREFIX TOPIC_MODE;
    doc["mode_state_topic"] = MQTT_PREFIX TOPIC_MODE;
    doc["mode_command_template"] = "{{ {'auto':'Auto','dry':'Dry','cool':'Cool','fan_only':'Fan','heat':'Heat','off':'Off'}[value] }}";
    doc["mode_state_template"] = "{{ {'Auto':'auto','Dry':'dry','Cool':'cool','Fan':'fan_only','Heat':'heat','Off':'off','Stop':'off'}.get(value, 'off') }}";

    // Power
    doc["power_command_topic"] = MQTT_SET_PREFIX TOPIC_POWER;
    doc["payload_on"] = PAYLOAD_POWER_ON;
    doc["payload_off"] = PAYLOAD_POWER_OFF;

    // Temperature
    doc["temperature_command_topic"] = MQTT_SET_PREFIX TOPIC_TSETPOINT;
    doc["temperature_state_topic"] = MQTT_PREFIX TOPIC_TSETPOINT;
    doc["current_temperature_topic"] = MQTT_PREFIX TOPIC_TROOM;
    doc["min_temp"] = 18;
    doc["max_temp"] = 30;
    doc["precision"] = 0.5;
    doc["temperature_unit"] = "C";

    // Fan
    JsonArray fan_modes = doc.createNestedArray("fan_modes");
    fan_modes.add("1");
    fan_modes.add("2");
    fan_modes.add("3");
    fan_modes.add("4");
    fan_modes.add("Auto");
    doc["fan_mode_command_topic"] = MQTT_SET_PREFIX TOPIC_FAN;
    doc["fan_mode_state_topic"] = MQTT_PREFIX TOPIC_FAN;

    // Vertical vanes (UD)
    JsonArray swing_modes = doc.createNestedArray("swing_modes");
    swing_modes.add("Up");
    swing_modes.add("Up/Center");
    swing_modes.add("Center/Down");
    swing_modes.add("Down");
    swing_modes.add("Swing");
    doc["swing_mode_command_topic"] = MQTT_SET_PREFIX TOPIC_VANES;
    doc["swing_mode_state_topic"] = MQTT_PREFIX TOPIC_VANES;
    doc["swing_mode_command_template"] = "{{ {'Up':'1','Up/Center':'2','Center/Down':'3','Down':'4','Swing':'Swing'}[value] }}";
    doc["swing_mode_state_template"] = "{{ {'1':'Up','2':'Up/Center','3':'Center/Down','4':'Down','Swing':'Swing'}.get(value, value) }}";

#ifdef USE_EXTENDED_FRAME_SIZE
    // Horizontal vanes (LR)
    JsonArray swing_h_modes = doc.createNestedArray("swing_horizontal_modes");
    swing_h_modes.add("Left");
    swing_h_modes.add("Left/Center");
    swing_h_modes.add("Center");
    swing_h_modes.add("Center/Right");
    swing_h_modes.add("Right");
    swing_h_modes.add("Wide");
    swing_h_modes.add("Spot");
    swing_h_modes.add("Swing");
    doc["swing_horizontal_mode_command_topic"] = MQTT_SET_PREFIX TOPIC_VANESLR;
    doc["swing_horizontal_mode_state_topic"] = MQTT_PREFIX TOPIC_VANESLR;
    doc["swing_horizontal_mode_command_template"] = "{{ {'Left':'1','Left/Center':'2','Center':'3','Center/Right':'4','Right':'5','Wide':'6','Spot':'7','Swing':'Swing'}[value] }}";
    doc["swing_horizontal_mode_state_template"] = "{{ {'1':'Left','2':'Left/Center','3':'Center','4':'Center/Right','5':'Right','6':'Wide','7':'Spot','Swing':'Swing'}.get(value, value) }}";
#endif

    // Availability (uses existing LWT)
    doc["availability_topic"] = MQTT_PREFIX TOPIC_CONNECTED;
    doc["payload_available"] = PAYLOAD_CONNECTED_TRUE;
    doc["payload_not_available"] = PAYLOAD_CONNECTED_FALSE;

    // Device info
    JsonObject device = doc.createNestedObject("device");
    device.createNestedArray("identifiers").add(HOSTNAME);
    device["name"] = HOSTNAME;
    device["manufacturer"] = "Mitsubishi Heavy Industries";
    device["sw_version"] = VERSION;

    String topic = String("homeassistant/climate/") + HOSTNAME + "/config";
    if (publishJsonRetained(topic.c_str(), doc)) {
      Serial.println(F("HA Discovery: climate entity published"));
    } else {
      Serial.printf_P(PSTR("HA Discovery: climate FAILED (json=%u bytes)\n"), measureJson(doc));
    }
  }

#ifdef USE_EXTENDED_FRAME_SIZE
  // --- 3D Auto switch entity discovery ---
  {
    DynamicJsonDocument doc(512);

    doc["name"] = String(HOSTNAME) + " 3D Auto";
    doc["unique_id"] = String(HOSTNAME) + "_3Dauto";
    doc["object_id"] = String(HOSTNAME) + "_3Dauto";
    doc["command_topic"] = MQTT_SET_PREFIX TOPIC_3DAUTO;
    doc["state_topic"] = MQTT_PREFIX TOPIC_3DAUTO;
    doc["payload_on"] = PAYLOAD_3DAUTO_ON;
    doc["payload_off"] = PAYLOAD_3DAUTO_OFF;
    doc["icon"] = "mdi:rotate-3d-variant";

    // Availability
    doc["availability_topic"] = MQTT_PREFIX TOPIC_CONNECTED;
    doc["payload_available"] = PAYLOAD_CONNECTED_TRUE;
    doc["payload_not_available"] = PAYLOAD_CONNECTED_FALSE;

    // Same device (groups with climate entity)
    JsonObject device = doc.createNestedObject("device");
    device.createNestedArray("identifiers").add(HOSTNAME);
    device["name"] = HOSTNAME;

    String topic = String("homeassistant/switch/") + HOSTNAME + "-3Dauto/config";
    if (publishJsonRetained(topic.c_str(), doc)) {
      Serial.println(F("HA Discovery: 3D Auto switch published"));
    } else {
      Serial.printf_P(PSTR("HA Discovery: 3D Auto FAILED (json=%u bytes)\n"), measureJson(doc));
    }
  }
#endif

  Serial.println(F("HA Discovery: complete"));
}
#endif

void output_P(const ACStatus status, PGM_P topic, PGM_P payload) {
  const int mqtt_topic_size = 100;
  char mqtt_topic[mqtt_topic_size];
  
  Serial.printf_P(PSTR("status=%i topic=%s payload=%s\n"), status, topic, payload);
  
  if ((status & 0xc0) == type_status){
    strncpy_P(mqtt_topic, PSTR(MQTT_PREFIX), mqtt_topic_size);
  }
  else if ((status & 0xc0) == type_opdata)
    strncpy_P(mqtt_topic, PSTR(MQTT_OP_PREFIX), mqtt_topic_size);
  else if ((status & 0xc0) == type_erropdata)
    strncpy_P(mqtt_topic, PSTR(MQTT_ERR_OP_PREFIX), mqtt_topic_size);
  strncat_P(mqtt_topic, topic, mqtt_topic_size - strlen(mqtt_topic));
  if(MQTTclient.publish_P(mqtt_topic, payload, false)==false){
    Serial.println( "MQTT Connection error");
  }
}

#if TEMP_MEASURE_PERIOD > 0
OneWire oneWire(ONE_WIRE_BUS);       // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire); // Pass our oneWire reference to Dallas Temperature.
DeviceAddress insideThermometer;     // arrays to hold device address

byte getDs18x20Temperature(int temp_hysterese) {
  static unsigned long DS1820Millis = millis();
  static int16_t tempR_old = 0xffff;

  if (millis() - DS1820Millis > TEMP_MEASURE_PERIOD * 1000) {
    int16_t tempR = sensors.getTemp(insideThermometer);
    tempR += ROOM_TEMP_DS18X20_OFFSET*128;
    if (tempR > (48*128) || tempR < (-10*128)) {    // skip onrealistic values
      tempR = tempR_old;    // use previous value
    }
    int16_t tempR_diff = tempR - tempR_old; // avoid using other functions inside the brackets of abs, see https://www.arduino.cc/reference/en/language/functions/math/abs/
    if (abs(tempR_diff) > temp_hysterese) {
      tempR_old = tempR;
      char strtmp[10];
      dtostrf(sensors.rawToCelsius(tempR), 0, 2, strtmp);
      //Serial.printf_P(PSTR("new DS18x20 temperature=%s°C\n"), strtmp);
      output_P((ACStatus)type_status, PSTR(TOPIC_TDS1820), strtmp);
    }
    DS1820Millis = millis();
    sensors.requestTemperatures();
  }
  //Serial.printf_P(PSTR("temp DS18x20 tempR_old=%i %i\n"), tempR_old, (byte)(tempR_old/32 + 61));
  if(tempR_old < 0)
    return 0;
  return tempR_old/32 + 61;
}

void printAddress(DeviceAddress deviceAddress) {
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16)
      Serial.print(F("0"));
    Serial.print(deviceAddress[i], HEX);
  }
}

void setup_ds18x20() {
  sensors.begin();
  Serial.printf_P(PSTR("Found %i DS18xxx family devices.\n"), sensors.getDS18Count());
  if (!sensors.getAddress(insideThermometer, 0))
    Serial.println(F("Unable to find address for Device 0"));
  else
    Serial.printf_P(PSTR("Device 0 Address: 0x%02x\n"), insideThermometer);
  sensors.setResolution(insideThermometer, 9); // set the resolution to 9 bit
  sensors.setWaitForConversion(false);
  sensors.requestTemperatures(); // Send the command to get temperatures
}
#endif

