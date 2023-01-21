#include <P1.h>

// Defalt values defined in P1.h but can be set in sketch if needed
#define P1_BAUDRATE 115200
#define P1_SERIAL_BUFFER 1024
#define P1_RX 16
#define P1_TX 17

// Manage loop variables
long last;
long interval = 10000L;

// Create P1 object
P1 p1;

void setup() {
  Serial.begin(115200);

  // Set debug to get verbose output. Flip to false once your program works as expected.
  p1.debug(true);

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

  // Non blocking timer will update P1 values every 10s
  if (millis() - last > interval) {
    p1.update();
    last = millis();
  }
}
