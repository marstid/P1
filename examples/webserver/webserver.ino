/*

Example that fetch data from P1 in json and prometheus format and then provide that via webserver


*/

#include <P1.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
ESP8266WebServer server(80);
#elif defined(ESP32)
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>

//#define P1_RX 16
//#define P1_TX 17


WebServer server(80);
#endif

const char *ssid = "JUPPI";
const char *password = "Bajenstreet29";


// Defalt values defined in P1.h but can be set in sketch if needed
//#define P1_BAUDRATE 115200
//#define P1_SERIAL_BUFFER 512
#define P1_SERIAL_INVERTED false  // Uncomment if developing with sample data ie running p1.txSample(); in the second loop



// Create a local variable to hold data
char json[P1_JSON_SIZE];
char prom[P1_PROM_SIZE];

// Manage loop variables
long last;
long last2;
long last3;


// Create P1 object
P1 p1;

void setup() {

  Serial.begin(115200);

  // Configure WIFI
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  Serial.println("Connecting to WIFI");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnection established!");
  Serial.printf("Connected to %s with ip %s\n\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());

  // Configure webserver
  server.on("/", handleRoot);
  server.on("/json", handleJson);
  server.on("/metrics", handleProm);

  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");

  // Configure P1
  // Set debug to get verbose output. Flip to false once your program works as expected.
  p1.debug(false);

  // Set custom identifier for json/prometheus data
  //char id[] = "12345";
  p1.setId((char *) "12345");

#if defined(ESP8266)
  Serial.println("Configuring serial input");
  delay(10000);
  // ##### Configure Hardware Serial on a ESP8266
  Serial.setRxBufferSize(P1_SERIAL_BUFFER);
  Serial.begin(P1_BAUDRATE, SERIAL_8N1, SERIAL_FULL);
  Serial.println("Swapping UART0 RX to inverted");
  Serial.flush();


  // Invert the RX serialport by setting a register value, Signal from P1 ports needs to be inverted
  if (P1_SERIAL_INVERTED) {
    USC0(UART0) = USC0(UART0) | BIT(UCRXI);
  }

  // Init P1 with the serial connected to meters P1 port
  p1.begin(Serial);
  Serial.println("Serial port is ready to recieve.");

#elif defined(ESP32)
  // ##### Configure Hardware Serial2 on a ESP32

  //Serial bugffer size - This need to be set before Serial2.begin() to take effect
  // Buffer need to be large enough to hold the complete message to get correct readings
  Serial2.setRxBufferSize(P1_SERIAL_BUFFER);

  // - NOTE the P1 signal are inverted.
  // During looback test, connect RX2 to TX2 and set P1_SERIAL_INVERTED false - as this example
  Serial2.begin(P1_BAUDRATE, SERIAL_8N1, P1_RX, P1_TX, P1_SERIAL_INVERTED);

  // Init P1 with the serial connected to meters P1 port
  p1.begin(Serial2);
#endif

  Serial.println("\n\nSetup completed...");


  Serial.printf("Free heap: %d\n", ESP.getFreeHeap());
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

  // Non blocking timer will update P1 values and data objects every 2 second
  if (millis() - last > 2000L) {
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
  if (millis() - last2 > 5000L) {
    //   p1.txSample();  // Connect a cable between RX & TX and uncomment to send sample data during development and remember to disable signal invert
    last2 = millis();
  }

  // Check that WIFI is still connected
  if ((WiFi.status() != WL_CONNECTED) && (millis() - last3 > 60000L)) {
    Serial.print(millis());
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
    last3 = millis();
  }
}