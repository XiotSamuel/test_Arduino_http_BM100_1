// source https://techtutorialsx.com/2017/05/20/esp32-http-post-requests/

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// add radio head
#include <RH_RF22.h>
#include <SPI.h>
#include <RHSoftwareSPI.h>

#define CLIENT_ADDRESS 0x21
#define SERVER_ADDRESS 5

// Class to manage message delivery and receipt, using the driver declared above
// RHReliableDatagram manager(driver, CLIENT_ADDRESS);

const char *ssid = "EnergysGroup";
const char *password = "NewV1s10n";

// source https://www.emqx.com/en/blog/esp32-connects-to-the-free-public-mqtt-broker

#include <PubSubClient.h>
// MQTT Broker
const char *mqtt_broker = "staging.energysmeter.com";
const char *topic = "esp32/test";
const char *iaq_topic = "esp32/iaq_reading";
const char *cmd_topic = "esp32/command";
const char *mqtt_username = "user2";
const char *mqtt_password = "Qazw123$";
const int mqtt_port = 1883;

// all command for the BM100

//{D1,D2,D3,D4,D5,D6,D7,D8,D9,D10,D11,
// D12,D13,D14,D15,D16,D17,D18,D19,D20,D21,D22}
// uint8_t msg1[] = {0x11, 0x0E, 0x10, 0xE0, 0x50, 0x00, 0x00, 0x00, 0x50, 0x00, 0x50,
// 0x00, 0x50, 0x00, 0x50, 0x00, 0x50, 0x00, 0x50, 0x00, 0x50, 0x40, 0x3B };
// uint8_t msg2[] = {0x11, 0x0E, 0x10, 0xE0, 0x50, 0x00, 0x00, 0x00, 0x50, 0x00, 0x50,
// 0x00, 0x50, 0x00, 0x50, 0x00, 0x50, 0x00, 0x50, 0x00, 0x50, 0x55, 0x3B };
// uint8_t msg3[] = {0x11, 0x0E, 0x10, 0xE0, 0x50, 0x00, 0x00, 0x00, 0x50, 0x00, 0x50,
// 0x00, 0x50, 0x00, 0x50, 0x00, 0x50, 0x00, 0x50, 0x00, 0x50, 0x6A, 0x3B };
// uint8_t msg4[] = {0x11, 0x0E, 0x10, 0xE0, 0x50, 0x00, 0x00, 0x00, 0x50, 0x00, 0x50,
// 0x00, 0x50, 0x00, 0x50, 0x00, 0x50, 0x00, 0x50, 0x00, 0x50, 0x7F, 0x3B };
uint8_t cmd1[] = {0x11, 0x04, 0x01, 0xe9};
uint8_t cmd2[] = {0x11, 0x04, 0x02, 0xe9};
uint8_t cmd3[] = {0x11, 0x04, 0x03, 0xe9};
uint8_t cmd4[] = {0x11, 0x04, 0x04, 0xe9};
uint8_t cmd5[] = {0x11, 0x04, 0x05, 0xe9};

uint8_t cmd0[] = {0x11, 0x04, 0x00, 0xe9};
// uint8_t data[] = {0x11, 0x08, 0x50, 0x50, 0x00, 0x00, 0xAD, 0x9A};

// Dont put this on the stack:
uint8_t buf[RH_RF22_MAX_MESSAGE_LEN];

uint8_t rf_data[] = {0x11, 0x08, 0x50, 0x50, 0x00, 0x00, 0xAD, 0xFF};

WiFiClient espClient;
PubSubClient mqttclient(espClient);

void callback(char *topic, byte *payload, unsigned int length);
void bm100mqttSub(void *pvParam);

// Singleton instance of the radio driver
// RH_RF22 driver;
RHSoftwareSPI spi;
// ss_pin nSEL GPIO15
// irq_pin nIRQ GPIO5
RH_RF22 driver(22, 23, spi);

void setup()
{

  Serial.begin(115200);
  delay(4000); // Delay needed before calling the WiFi.begin

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  { // Check for the connection
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println("Connected to the WiFi network");

  mqttclient.setServer(mqtt_broker, mqtt_port);
  mqttclient.setCallback(callback);
  while (!mqttclient.connected())
  {
    String client_id = "esp32-client-";
    client_id += String(WiFi.macAddress());
    Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
    if (mqttclient.connect(client_id.c_str(), mqtt_username, mqtt_password))
    {
      Serial.println("Public emqx mqtt broker connected");
    }
    else
    {
      Serial.print("failed with state ");
      Serial.print(mqttclient.state());
      delay(2000);
    }
  }

  // publish and subscribe
  mqttclient.publish(topic, "LALALA I am LALALAL");
  mqttclient.subscribe(topic);
  mqttclient.subscribe(cmd_topic);

  xTaskCreate(bm100mqttSub, "bm100mqttSub", 2048, NULL, 5, NULL);

  // start RH driver
  Serial.println("set pin");
  spi.setPins(12, 13, 14);
  Serial.println("driver init");
  if (!driver.init())
    Serial.println("init failed");
  // Defaults after init are 434.0MHz, 0.05MHz AFC pull-in, modulation FSK_Rb2_4Fd36

  // Serial.println("init rfid");

  // int i = 0;
  // while (i < 8)
  // {
  //   Serial.println("Sending data");
  //   driver.setHeaderFrom(0x21);
  //   driver.setHeaderTo(0x21);
  //   driver.setHeaderId(0x21);
  //   driver.setHeaderFlags(0x21);
  //   driver.send(rf_data, sizeof(rf_data));
  //   delay(1000);
  //   i++;
  // }

  // Serial.println("setup done");

}

void bm100mqttSub(void *pvParam)
{
  Serial.println("mqttSub Task start");
  while (1)
  {
    mqttclient.loop();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void callback(char *topic, byte *payload, unsigned int length)
{

  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message:");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  Serial.println("-----------------------");

  DynamicJsonDocument doc(1024);
  deserializeJson(doc, payload);
  JsonObject root = doc.as<JsonObject>();
  int cmd = int(root["cmd"]);

  if (cmd)
  {
    Serial.print("cmd: ");
    Serial.println(cmd);
    driver.setHeaderFrom(0x50);
    driver.setHeaderTo(0x00);
    driver.setHeaderId(0x00);
    driver.setHeaderFlags(0xAD);

    if (cmd == 1)
    {
      if (driver.send(cmd1, sizeof(cmd1)))
      {
        Serial.println("cmd 1 send ok");
      }
    }
    else if (cmd == 2)
    {
      if (driver.send(cmd2, sizeof(cmd2)))
      {
        Serial.println("cmd 2 send ok");
      }
    }
    else if (cmd == 3)
    {
      if (driver.send(cmd3, sizeof(cmd3)))
      {
        Serial.println("cmd 3 send ok");
      }
    }
    else if (cmd == 4)
    {
      if (driver.send(cmd4, sizeof(cmd4)))
      {
        Serial.println("cmd 4 send ok");
      }
    }
    else if (cmd == 5)
    {
      if (driver.send(cmd5, sizeof(cmd5)))
      {
        Serial.println("cmd 5 send ok");
      }
    }
    driver.waitPacketSent();
  }
}

void loop()
{

  // driver.setHeaderFrom(0x50);
  // driver.setHeaderTo(0x00);
  // driver.setHeaderId(0x00);
  // driver.setHeaderFlags(0xAD);
  // Serial.println("cmd0");
  // driver.send(cmd0, sizeof(cmd0));

  // driver.waitPacketSent();
  // delay(1000);
  // delay(10000);  //Send a request every 10 seconds
}