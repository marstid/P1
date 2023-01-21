#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <P1.h>


const char *ssid = "YourSSIDHere";
const char *password = "YourPSKHere";

WebServer server(80);

// Defalt values defined in P1.h but can be set in sketch if needed
#define P1_BAUDRATE 115200
#define P1_SERIAL_BUFFER 1024
#define P1_RX 16
#define P1_TX 17
//#define P1_SERIAL_INVERTED false // Uncomment if developing with sample data ie running p1.txSample(); in the second loop

// Create a local variable to hold data
char json[P1_JSON_SIZE];
char prom[P1_PROM_SIZE];

// Manage loop variables
long last;
long last2; 

// P1 sends updates every 10s, let's check every 5s if there is a message available
long interval = 5000;

// Create P1 object
P1 p1;

void setup() {
  Serial.begin(115200);

  // Configure WIFI
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/json", handleJson);
  server.on("/metrics", handleProm);

  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");

  // Configure P1
  // Set debug to get verbose output. Flip to false once your program works as expected.
  p1.debug(true);

  // Set custom identifier for json/prometheus data
  p1.setId("123");

  // Print configuraion info
  p1.configInfo();

  // ##### Configure Hardware Serial2 on a ESP32

  //Serial bugffer size - This need to be set before Serial2.begin() to take effect
  // Buffer need to be large enough to hold the complete message to get correct readings
  Serial2.setRxBufferSize(P1_SERIAL_BUFFER);

  // - NOTE the P1 signal are inverted.
  // During looback test, connect RX2 to TX2 and set P1_SERIAL_INVERTED false - as this example
  Serial2.begin(P1_BAUDRATE, SERIAL_8N1, P1_RX, P1_TX, P1_SERIAL_INVERTED);

  // Init P1 with the serial connected to meters P1 port
  p1.begin(Serial2);

  Serial.println("\n\nSetup completed...");
}

// Web content
void handleRoot() {
  char temp[400];
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;

  snprintf(temp, 400,

           "<html>\
  <head>\
    <meta http-equiv='refresh' content='5'/>\
    <title>ESP32 P1 Demo</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Hello from ESP32 P1!</h1>\
    <p>Uptime: %02d:%02d:%02d</p>\
  </body>\
</html>",

           hr, min % 60, sec % 60);
  server.send(200, "text/html", temp);
}

void handleJson() {
  server.send(200, "text/plain", json);
}

void handleProm() {
  server.send(200, "text/plain", prom);
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
}

void loop() {
  server.handleClient();

  // Non blocking timer will update P1 values and data objects every 10s
  if (millis() - last > interval) {
    if (p1.update()) {
      Serial.println("Update successful!");
      // new values fetched, let's update the local variables
      p1.getJson(json);
      p1.getPrometheus(prom);
    } else {
      Serial.println("Update failed!");
    }
    last = millis();
  }

  // Non blocking timer used when sending mock data
  if (millis() - last2 > 10000L) {
 //   p1.txSample();  // Connect a cable between RX & TX and uncomment to send sample data during development
    last2 = millis();
  }
}