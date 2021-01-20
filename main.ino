#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#define LED D4 // Use built-in LED which connected to D4 pin or GPIO 2
#define NETWORK_NAME "network name" 
#define PASSWORD "network password"
#define MQTT_SERVER "broker.mqttdashboard.com"
#define MQTT_CLIENT_ID "your client id"
#define MEASURE_PERIOD 10000
#define PORT 80
#define ANA A0
#define DIGI D5
#define POMPOUT D6

WiFiClient espClient;

PubSubClient client(espClient);
char buffer [50]; //extend if bigger data is expected

ESP8266WebServer server(PORT); // Port for webserver

long now = millis();
long lastMeasure = 0;

double analogValue = 0.0;
int digitalValue = 0;

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(NETWORK_NAME);
  WiFi.begin(NETWORK_NAME, PASSWORD);
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

  // Feel free to add more if statements to control more GPIOs with MQTT

  if(topic == "plant-pump"){
    if(messageTemp == "1"){
      digitalWrite(POMPOUT, true);
    }else{
      digitalWrite(POMPOUT, false);
    }
  }
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
       if (client.connect("ESP8266Client")) {
     You can do it like this:
       if (client.connect("ESP1_Office")) {
     Then, for the other ESP:
       if (client.connect("ESP2_Garage")) {
      That should solve your MQTT multiple connections problem
    */
    if (client.connect(MQTT_CLIENT_ID)) {
      Serial.println("connected");  
      // Subscribe or resubscribe to a topic
      // You can subscribe to more topics (to control more LEDs in this example)
      client.subscribe("plant-pump");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(ANA, INPUT);
  pinMode(DIGI, INPUT);
  pinMode(POMPOUT, OUTPUT);
  
  Serial.begin(115200);
  
  setup_wifi();
  
  client.setServer(MQTT_SERVER, 1883);
  client.setCallback(callback);
  
  setupServer();
}

void loop() {
  server.handleClient();

  if (!client.connected()) {
    reconnect();
  }
  if(!client.loop())
    client.connect(MQTT_CLIENT_ID);

  now = millis();
  // Publishes new temperature and humidity every 30 seconds
  if (now - lastMeasure > MEASURE_PERIOD) {
    lastMeasure = now;
    analogValue = analogRead(ANA);
    digitalValue = digitalRead(DIGI);
    // Serial data
    Serial.print("Analog raw: ");
    Serial.println(analogValue);
    Serial.print("Digital raw: ");
    Serial.println(digitalValue);
    Serial.println(" ");
    if (digitalValue == 0) {
      digitalWrite(POMPOUT, false);
    } else {
      Serial.println("PUMPIN");
      digitalWrite(POMPOUT, true);
    }

    static char humidityTemp[7];
    dtostrf(analogValue, 6, 2, humidityTemp);

    sprintf(buffer, "%.0f;%d\n", analogValue, digitalValue);

    // Publishes Temperature and Pump values
    client.publish("plant-data", buffer);
  }

  delay(1000);
}

void setupServer() {
  server.on("/", handleRoot);               
  server.begin();                           
  Serial.println("HTTP server started");
}

void handleRoot() {
  server.send(200, "text/plain", "Hello from Make");
}
