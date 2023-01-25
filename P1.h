/*

Arduino library to read and parse data from P1/HAN serial ports.

Develped and tested agains a power meter deployed by Vattenfall in Sweden.
Should be generic and able to parse any obis data but only metadata from swedish power meters will be enriched with descriptive data.

*/
#ifndef P1_H
#define P1_H
#include <Arduino.h>

#define P1_BUFFER_SIZE 256
#define P1_ARRAY_SIZE 2048
#define OBIS_MAP_SIZE 30 // Size of array holding metadata
#define P1_MAP_SIZE 30 // Size of array holding p1_records

#define P1_LINES 100

#define P1_BAUDRATE 115200

// Reading from a P1 port signal must be inverted.
// In testing flip to false
#define P1_SERIAL_INVERTED true

#if defined(ESP8266)
#define p1_log(str) Serial.println(F(str));
#define p1_logf(fmt, args...) Serial.printf(fmt, ##args);
#define debug_log(str) if(_debug){Serial.println(F(str));}
#define debug_logf(fmt, args...) if(_debug){Serial.printf(fmt, ##args);}

#define P1_PROM_SIZE 6144
#define P1_JSON_SIZE 4096
#define P1_SERIAL_BUFFER 1024

#elif defined(ESP32)
#define p1_log(str) Serial.println(str);
#define p1_logf(fmt, args...) Serial.printf(fmt, ##args);
#define debug_log(str) Serial.println(str);
#define debug_logf(fmt, args...) Serial.printf(fmt, ##args);

#define P1_PROM_SIZE 6144
#define P1_JSON_SIZE 4096
#define P1_RX 16
#define P1_TX 17

#define P1_SERIAL_BUFFER 1024

#elif 
#define p1_log(str) Serial.println(str);
#define p1_logf(fmt, args...) Serial.printf(fmt, ##args);
#define debug_log(str) Serial.println(str);
#define debug_logf(fmt, args...) Serial.printf(fmt, ##args);

#define P1_PROM_SIZE 6144
#define P1_JSON_SIZE 4096
#define P1_RX 16
#define P1_TX 17

#define P1_SERIAL_BUFFER 1024
#endif


// Struct to hold collected data
typedef struct
{
  char obis[12];
  char value[50];
  char unit[10];
  char type[10]; // Metric type
  char metric[40]; // Metric name
  char desc[75]; // Description of data
} p1_record;

// Struct to hold metadata
typedef struct {
  char obis[12];
  char type[10];
  char metric[40];
  char desc[75];
} obis_map;

class P1 {
public:
  P1();
  void begin(Stream &serial);
  bool update();

  void debug(bool input);
  void configInfo();
  void setId(char *id);
  
  bool getJson(char* json);
  bool getPrometheus(char* prom);

  void txSample();
  
private:
  bool getSerialData();

  unsigned int P1CRC16(unsigned int crc, unsigned char *buf, int len);
  int getLineStart(char *input, int start, int end);
  bool parseData();
  int getIndexOf(char *input, char find);
  void listArray();
  int getMapIndex(char *input);
  

  Stream* _serial;
  static char _id[];
  static char _data[P1_ARRAY_SIZE];
  static bool _debug;
  static char sample[];
  static p1_record _p1array[P1_MAP_SIZE];
  static obis_map _map[OBIS_MAP_SIZE];
};

#endif