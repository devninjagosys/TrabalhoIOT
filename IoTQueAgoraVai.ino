#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <MFRC522.h>

#define REDL       D8
#define GREENL     D4
#define CARDBUTT   D0
#define RST_PIN         D1          
#define SS_PIN          D2

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key keyN;
MFRC522::StatusCode status;

// Change the credentials below, so your ESP8266 connects to your router
const char* ssid = "hsNCE";
const char* password = "";
const char* id = "sala100000";
String currID = "00000000";
const char* room = "H201";
byte cardStatus = 0;
bool cardInserted = false;
bool badCard = false;
// Change the variable to your Raspberry Pi IP address, so it connects to your MQTT broker
const char* mqtt_server = "10.10.17.129";


// Initializes the espClient. You should change the espClient name if you have multiple ESPs running in your home automation system
WiFiClient espClient;

PubSubClient client(espClient);

// Timers auxiliar variables
long now = millis();
long lastMeasure = 0;
long lastMeasure1 = 0;
int randoGen;

// Don't change the function below. This functions connects your ESP8266 to your router
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
}

// This functions is executed when some device publishes a message to a topic that your ESP8266 is subscribed to
// Change the function below to add logic to your program, so when a device publishes a message to a topic that
// your ESP8266 is subscribed you can actually do something
void callback(String topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(messageTemp);
  if (!root.success()) {
    Serial.println("parseObject() failed");
    return;
  }
  String nameRecieved = root["id_node"];
  if (nameRecieved == String(id)) {
    Serial.println("This is my message!");
    String commandRecieved = root["status"];
    digitalWrite(REDL, commandRecieved == String("2"));
    digitalWrite(GREENL, commandRecieved == String("1"));
  }
  // Feel free to add more if statements to control more GPIOs with MQTT

  // If a message is received on the topic room/REDL, you check if the message is either on or off. Turns the REDL GPIO according to the message
  //  if(topic=="room/ack"){
  //      Serial.print("Changing Room ack to ");
  //      if(messageTemp == "on"){
  //        digitalWrite(REDL, HIGH);
  //        Serial.print("On");
  //      }
  //      else if(messageTemp == "off"){
  //        digitalWrite(REDL, LOW);
  //        Serial.print("Off");
  //      }
  //  }
  Serial.println();
}

// This functions reconnects your ESP8266 to your MQTT broker
// Change the function below if you want to subscribe to more topics with your ESP8266
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    /*
      YOU MIGHT NEED TO CHANGE THIS LINE, IF YOU'RE HAVING PROBLEMS WITH MQTT MULTIPLE CONNECTIONS
      To change the ESP device ID, you will have to give a new name to the ESP8266.
      Here's how it looks:
       if (client.connect("nomemelhor")) {
      You can do it like this:
       if (client.connect("nomemelhor")) {
      Then, for the other ESP:
       if (client.connect("ESP2_Garage")) {
      That should solve your MQTT multiple connections problem
    */
    if (client.connect(id)) {
      Serial.println("connected");
      // Subscribe or resubscribe to a topic
      // You can subscribe to more topics (to control more LEDs in this example)
      client.subscribe("room/ack");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void sendMyData() {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["id_aula"] = currID;
  root["id_sala"] = room;
  root["id_node"] = id;
  char buffert[100];
  root.printTo(buffert);
  client.publish("room/card", buffert);

  Serial.println("dado enviado");
  Serial.println(buffert);
}

void readCardd(){
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
        badCard = true;
        Serial.println("Bad cardA");
        return;
      }

      // Select one of the cards
      if ( ! mfrc522.PICC_ReadCardSerial()) {
        badCard = true;
        Serial.println("Bad cardB");
        return;
      }
      byte block = 5;
      byte len = 18;
      byte buffer1[18];
      status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &keyN, &(mfrc522.uid)); //line 834 of MFRC522.cpp file
      if (status != MFRC522::STATUS_OK) {
        Serial.print(F("Bad cardC because "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        badCard = true;
        return;
      }
      status = mfrc522.MIFARE_Read(block, buffer1, &len);
      if (status != MFRC522::STATUS_OK) {
        Serial.print(F("Bad cardD because"));
        Serial.println(mfrc522.GetStatusCodeName(status));
        badCard = true;
        return;
      }
      char tempc[8];
      for (int i = 0; i < 8; i++){
        if(buffer1[i] == NULL){
          Serial.println(F("Bad cardE"));
          badCard = true;
          return;
        }
        tempc[i] = (char)buffer1[i];
      }
      currID = String(tempc);
      Serial.print(F("Card read, ID: "));
      Serial.println(currID);
      badCard = false;
}

// The setup function sets your ESP GPIOs to Outputs, starts the serial communication at a baud rate of 115200
// Sets your mqtt broker and sets the callback function
// The callback function is what receives messages and actually controls the LEDs
void setup() {
  randoGen = random(0, 5000);
  pinMode(REDL, OUTPUT);
  pinMode(GREENL, OUTPUT);
  pinMode(CARDBUTT, INPUT);
  cardInserted = !digitalRead(CARDBUTT);
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  keyN.keyByte[0] = 0x54;
  keyN.keyByte[1] = 0x6b;
  keyN.keyByte[2] = 0xad;
  keyN.keyByte[3] = 0xa2;
  keyN.keyByte[4] = 0x92;
  keyN.keyByte[5] = 0xcb;
  SPI.begin();               
  mfrc522.PCD_Init();
}


// For this project, you don't need to change anything in the loop function. Basically it ensures that you ESP is connected to your broker
void loop() {
  if (!cardInserted) {
    digitalWrite(REDL, 0);
    digitalWrite(GREENL, 0);
  }
  else if(badCard){
    digitalWrite(REDL, 1);
    digitalWrite(GREENL, 0);
  }
  if (digitalRead(CARDBUTT) == cardInserted && (now  > lastMeasure1 + 1000 || now < lastMeasure1)) {
    lastMeasure1 = now;
    cardInserted = !cardInserted;
    if (cardInserted) {
      readCardd();
      Serial.println("Card inserted");
      sendMyData();
    }
    else {
      badCard = false;
      mfrc522.PCD_StopCrypto1();
      mfrc522.PICC_HaltA();
      currID = "00000000";
      Serial.println("Card removed");
      sendMyData();
    }
  }
  if (!client.connected()) {
    reconnect();
  }
  if (!client.loop())
    client.connect(id);

  now = millis();
  if (!badCard && now  > lastMeasure + 5000 + randoGen || now < lastMeasure) {
    lastMeasure = now;
    randoGen = random(0, 500);
    sendMyData();


  }
}
