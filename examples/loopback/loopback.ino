#include <P1.h>

// Defalt values defined in P1.h but can be set in sketch if needed
#define P1_BAUDRATE 115200
#define P1_SERIAL_BUFFER 1024
#define P1_RX 16
#define P1_TX 17
#define P1_SERIAL_INVERTED false  // IMPORTANT - P1 port serial is inverted. Default is true

// Create a local variable to hold a JSON array of data
char json[P1_JSON_SIZE];
char prom[P1_PROM_SIZE];

// Manage loop variables
long last;
long interval = 10000L;

// Create P1 object
P1 p1;

void setup() {
  Serial.begin(115200);

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

void loop() {

  // Non blocking timer will update P1 values and JSON object every 10s
  if (millis() - last > interval) {

    if (p1.update()) {
      Serial.println("Update successful!");
      // new values fetched, let's update the local json varible
      p1.getJson(json);

      // fetch the prometheus export data char
      p1.getPrometheus(prom);
    } else {
      Serial.println("Update failed!");
    }

    // Send sample data that will loop back to RX and read by next p1.update()
    // Connect a cabel between RX2 & TX2 (GPIO 16 and 17 on ESP32 Devboard)
    p1.txSample();

    last = millis();
  }
}
