#include <Arduino.h>
#include "P1.h"


char P1::_data[P1_ARRAY_SIZE];
bool P1::_debug = false;
p1_record P1::_p1array[P1_MAP_SIZE];
char P1::_id[50] = "0";

P1::P1() {
}

void P1::begin(Stream &serial) {
  _serial = &serial;
}

void P1::configInfo() {
  Serial.println();
  p1_log("-- P1 Configuration")

  Serial.printf("P1_BAUDRATE: %d\n", P1_BAUDRATE);
  Serial.printf("P1_BUFFER_SIZE: %d\n", P1_BUFFER_SIZE);
}


void P1::debug(bool input) {
  _debug = input;
}

// Set custom id lable for data export to be able to sigle out meters
void P1::setId(char *id) {
  strcpy(_id, id);
}

// Updates data by reading telegram and update objects
bool P1::update() {
  bool status = false;
  if (getSerialData()) {
    if (parseData()) {
      status = true;
    }
  }
  return status;
}

// Private function to list array during dev
void P1::listArray() {
  int buf_len = sizeof(_p1array) / sizeof(_p1array[0]);
  for (int i = 0; i < buf_len; i++) {

    if (_p1array[i].obis[0] != '\0') {
      Serial.printf("o: %s, v: %s, u: %s, t: %s, m: %s, d: %s\n", _p1array[i].obis, _p1array[i].value, _p1array[i].unit, _p1array[i].type, _p1array[i].metric, _p1array[i].desc);
    }
  }
}

// Process data to create Prometheus export formatted char*
bool P1::getPrometheus(char *prom) {
  memset(prom, 0, P1_PROM_SIZE);

  int buf_len = sizeof(_p1array) / sizeof(_p1array[0]);
  for (int i = 0; i < buf_len; i++) {

    if (_p1array[i].obis[0] != '\0') {
      char help[150] = "";
      char type[150] = "";
      char data[100] = "";
      #if defined(ESP8266)
      sprintf(help, "# HELP %s %s\n", _p1array[i].metric, _p1array[i].desc);
      sprintf(type, "# TYPE %s %s\n", _p1array[i].metric, _p1array[i].type);
      #elif defined(ESP32)
      sprintf(help, "# HELP %s %s\n", _p1array[i].metric, _p1array[i].desc);
      sprintf(type, "# TYPE %s %s\n", _p1array[i].metric, _p1array[i].type);
      #endif
      
      sprintf(data, "%s{sensor=\"p1\", id=\"%s\", obis=\"%s\", unit=\"%s\"}%s\n", _p1array[i].metric, _id, _p1array[i].obis, _p1array[i].unit, _p1array[i].value);
      strcat(prom, help);
      strcat(prom, type);
      strcat(prom, data);
    }
  }
  
  char buf[100] = "";
  strcat(prom, "# TYPE esp_heap gauge\n");
  sprintf(buf, "esp_heap{sensor=\"p1\", id=\"%s\"}%d\n", _id, ESP.getFreeHeap());
  strcat(prom, buf);
  
  strcat(prom, "# TYPE esp_uptime counter\n");
  sprintf(buf, "esp_uptime{sensor=\"p1\", id=\"%s\"}%d\n", _id, millis());
  strcat(prom, buf);


  if (_debug) {
    Serial.println(prom);
    Serial.printf("Size of prometheus[]: %d\n", strlen(prom));
  }
  return true;
}

// process _p1array and create json formatted string
bool P1::getJson(char *json) {
  strcpy(json,"[");
  int buf_len = sizeof(_p1array) / sizeof(_p1array[0]);
  char line[150] = "";
  for (int i = 0; i < buf_len; i++) {
    if (_p1array[i].obis[0] != '\0') {
      memset(line,0,150);
      #if defined(ESP8266)
      sprintf(line, "{sensor=\"p1\", id=\"%s\", obis=\"%s\", value:\"%s\", unit=\"%s\", description=\"%s\"},\n", _id, _p1array[i].obis, _p1array[i].value, _p1array[i].unit, _p1array[i].desc);
      #elif defined(ESP32)
      sprintf(line, "{sensor=\"p1\", id=\"%s\", obis=\"%s\", value:\"%s\", unit=\"%s\", description=\"%s\"},\n", _id, _p1array[i].obis, _p1array[i].value, _p1array[i].unit, _p1array[i].desc);
      #endif
      strcat(json, line);
    }
  }

  // Remove trailing , and end the array with ]
  int l = strlen(json);
  json[l - 2] = '\0';
  strcat(json, "]");

  if (_debug) {
    Serial.println(json);
    Serial.printf("Size of json[]: %d\n", strlen(json));
  }
  return true;
}

bool P1::getSerialData() {
  if (_debug)
    Serial.printf("P1::%s() - reading data from P1 port\n", __func__);

  bool end = false;
  bool start = false;
  char crc[10];
  unsigned int calculatedCRC;
  bool validCRCFound = false;

  char buf[P1_BUFFER_SIZE];
  size_t line;
  char tmp_array[P1_ARRAY_SIZE];

  // Clear temporary array
  memset(tmp_array, 0, P1_ARRAY_SIZE);

  if (_serial->available()) {
    if (_debug)
      Serial.printf("P1::%s() - Serial buffer filled to: %d\n", __func__,  _serial->available());
    while (_serial->available() && !end) {
      // Clear buffer, readline and append new line
      memset(buf, 0, P1_BUFFER_SIZE);
      line = _serial->readBytesUntil('\n', buf, P1_BUFFER_SIZE);
      buf[line] = '\n';

      if (buf[0] == '!') {
        end = true;
        memcpy(crc, buf + 1, (size_t)4);
        crc[4] = 0;
        strcat(tmp_array, "!");
        break;
      }

      if (buf[0] == '/') {
        start = true;
      }

      if (start && !end) {
        strcat(tmp_array, buf);
      }
    }
  }
  if (strlen(tmp_array) > 0) {
    if (_debug)
      Serial.println(tmp_array);

    int len = strlen(tmp_array);
    unsigned int currentCRC = P1CRC16(0x0000, (unsigned char *)tmp_array, len);
    bool validCRCFound = (strtol(crc, NULL, 16) == currentCRC);

    if (validCRCFound) {
      if (_debug) {
        Serial.printf("P1::%s() - CRC validated. Recieved: %x, Calculated: %x\n", __func__, strtol(crc, NULL, 16), currentCRC);
      }
      strcpy(_data, tmp_array);
      return true;

    } else {
      Serial.printf("P1::%s() - CRC INVALID. Recieved: %x, Calculated: %x\n", __func__, strtol(crc, NULL, 16), currentCRC);
      return false;
    }
  } else {
    if (_debug) {
      Serial.printf("P1::%s() - No data recieved\n", __func__);
    }
  }
  return false;
}


int P1::getLineStart(char *input, int start, int end) {

  int len = strlen(input);

  if (start > len || len < end) {
    return -1;
  }

  for (int i = 0; i < end; i++) {

    if (input[start + i] == '\n') {
      return start + i;
    }
  }
  return 0;
}

int P1::getIndexOf(char *input, char find) {
  int len = strlen(input);
  for (int i = 0; i < len; i++) {
    if (input[i] == find) {
      return i;
    }
  }
  return -1;
}


bool P1::parseData() {
  if (_debug)
    Serial.printf("P1::%s() - Parsing data\n", __func__);

  int count = 0;
  int start = 0;
  int end = strlen(_data);
  int nl = 0;
  bool parse = true;

  char buffer[256];

  while (parse) {
    memset(buffer, 0, 256);
    nl = getLineStart(_data, start, end);

    if (nl == -1 || nl == 0) {
      parse = false;
      break;
    }
    memcpy(buffer, _data + start, nl - start);
    buffer[nl - start + 1] = 0;

    if (buffer[0] == '0' || buffer[0] == '1') {
      int a = getIndexOf(buffer, '(');
      int b = getIndexOf(buffer, '*');
      int c = getIndexOf(buffer, ')');

      char obis[40] = "";
      char value[100] = "";
      float fvalue = 0;
      char unit[10] = "";

      memcpy(obis, buffer, (size_t)a);

      if (b > 1) {
        memcpy(value, buffer + a + 1, (b - a) - 1);
        fvalue = atof(value);
        sprintf(value, "%.2f", fvalue);  // Set to 2 decimals
        memcpy(unit, buffer + b + 1, (c - b) - 1);
      } else {
        memcpy(value, buffer + a + 1, (c - a) - 1);
      }

      // Copy data to struct array holding data
      // Truncate serial and date value
      if (strcmp(obis, "0-0:96.1.0") == 0 || strcmp(obis, "0-0:1.0.0") == 0) {
        strcpy(_p1array[count].obis, obis);
        memcpy(_p1array[count].value, value, 12);
      } else {
        strcpy(_p1array[count].obis, obis);
        strcpy(_p1array[count].value, value);
        strcpy(_p1array[count].unit, unit);
      }

      // If additional data is found in _map then populate that as well
      int index = getMapIndex(obis);
      if (index >= 0) {
        strcpy(_p1array[count].type, _map[index].type);
        strcpy(_p1array[count].desc, _map[index].desc);
        strcpy(_p1array[count].metric, _map[index].metric);
      }
    }
    start = nl + 1;
    count++;
  }
  return true;
}


/*

Function and data to test by connecting loop-back RX to TX
Remeber to set P1_SERIAL_INVERTED to "false"

*/

void P1::txSample() {
  _serial->println(sample);
}

char P1::sample[] =  R"(
/AUX5UXXXXXXXXXXXXX

0-0:96.1.0(123456789123456789123456789123456789)
0-0:1.0.0(230121214655W)
1-0:1.8.0(001671.380*kWh)
1-0:2.8.0(000000.000*kWh)
1-0:3.8.0(000000.558*kvarh)
1-0:4.8.0(000406.501*kvarh)
1-0:1.7.0(0000.632*kW)
1-0:2.7.0(0000.000*kW)
1-0:3.7.0(0000.000*kvar)
1-0:4.7.0(0000.419*kvar)
1-0:21.7.0(0000.205*kW)
1-0:22.7.0(0000.000*kW)
1-0:41.7.0(0000.205*kW)
1-0:42.7.0(0000.000*kW)
1-0:61.7.0(0000.221*kW)
1-0:62.7.0(0000.000*kW)
1-0:23.7.0(0000.000*kvar)
1-0:24.7.0(0000.112*kvar)
1-0:43.7.0(0000.000*kvar)
1-0:44.7.0(0000.187*kvar)
1-0:63.7.0(0000.000*kvar)
1-0:64.7.0(0000.118*kvar)
1-0:32.7.0(229.6*V)
1-0:52.7.0(228.7*V)
1-0:72.7.0(228.1*V)
1-0:31.7.0(001.1*A)
1-0:51.7.0(001.3*A)
1-0:71.7.0(001.1*A)
!03AB)";

// CRC16 - Unknown origin but widely used by other similar projects
unsigned int P1::P1CRC16(unsigned int crc, unsigned char *buf, int len) {
  for (int pos = 0; pos < len; pos++) {
    crc ^= (unsigned int)buf[pos];  // * XOR byte into least sig. byte of crc
                                    // * Loop over each bit
    for (int i = 8; i != 0; i--) {
      // * If the LSB is set
      if ((crc & 0x0001) != 0) {
        // * Shift right and XOR 0xA001
        crc >>= 1;
        crc ^= 0xA001;
      }
      // * Else LSB is not set
      else
        // * Just shift right
        crc >>= 1;
    }
  }
  return crc;
}

/*

Enrich the collected data with some meta data.
Currently only cotains obis frmo a Swedish meter from Vattenfall

*/


int P1::getMapIndex(char *input) {
  int index = -1;
  for (int j = 0; j < OBIS_MAP_SIZE; j++) {
    if (_map[j].obis[0] != '\0') {
      if (strcmp(_map[j].obis, input) == 0) {
        index = j;
      }
    }
  }
  return index;
}

// Map to popluate the extracted data with some additional metadata
obis_map P1::_map[OBIS_MAP_SIZE]  = {
  { "0-0:96.1.0", "gauge", "device_serial", "Device Serial" },
  { "0-0:1.0.0", "gauge", "device_date", "Device Date-time stamp of the P1 message - YYMMDDhhmmss" },
  { "1-0:1.8.0", "counter", "active_energy_positive_total", "Positive active energy (A+) total [kWh]" },
  { "1-0:2.8.0", "counter", "active_energy_negative_total", "Negative active energy (A+) total [kWh]" },
  { "1-0:3.8.0", "counter", "reactive_energy_positive_total", "Positive reactive energy (Q+) total [kvarh]" },
  { "1-0:4.8.0", "counter", "reactive_energy_negative_total", "Negative reactive energy (Q-) total [kvarh]" },
  { "1-0:1.7.0", "gauge", "active_power_positive", "Positive active instantaneous power (A+) [kW]" },
  { "1-0:2.7.0", "gauge", "active_power_negative", "Negative active instantaneous power (A-) [kW]" },
  { "1-0:3.7.0", "gauge", "reactive_power_positive", "Positive reactive instantaneous power (Q+) [kvar]" },
  { "1-0:4.7.0", "gauge", "reactive_power_negative", "Negative reactive instantaneous power (Q-) [kvar]" },
  { "1-0:21.7.0", "gauge", "active_power_positive_l1", "Positive active instantaneous power (A+) in phase L1 [kW]" },
  { "1-0:22.7.0", "gauge", "active_power_negative_l1", "Negative active instantaneous power (A-) in phase L1 [kW]" },
  { "1-0:41.7.0", "gauge", "active_power_positive_l2", "Positive active instantaneous power (A+) in phase L2 [kW]" },
  { "1-0:42.7.0", "gauge", "active_power_negative_l2", "Negative active instantaneous power (A-) in phase L2 [kW]" },
  { "1-0:61.7.0", "gauge", "active_power_positive_l3", "Positive active instantaneous power (A+) in phase L3 [kW]" },
  { "1-0:62.7.0", "gauge", "active_power_negative_l3", "Negative active instantaneous power (A-) in phase L3 [kW]" },
  { "1-0:23.7.0", "gauge", "reactive_power_positive_l1", "Positive reactive instantaneous power (Q+) in phase L1 [kvar]" },
  { "1-0:24.7.0", "gauge", "reactive_power_negative_l1", "Negative reactive instantaneous power (Q-) in phase L1 [kvar]" },
  { "1-0:43.7.0", "gauge", "reactive_power_positive_l2", "Positive reactive instantaneous power (Q+) in phase L2 [kvar]" },
  { "1-0:44.7.0", "gauge", "reactive_power_negative_l2", "Negative reactive instantaneous power (Q-) in phase L2 [kvar]" },
  { "1-0:63.7.0", "gauge", "reactive_power_positive_l3", "Positive reactive instantaneous power (Q+) in phase L3 [kvar]" },
  { "1-0:64.7.0", "gauge", "reactive_power_negative_l3", "Negative reactive instantaneous power (Q-) in phase L3 [kvar]" },
  { "1-0:32.7.0", "gauge", "voltage_l1", "Instantaneous voltage (U) in phase L1 [V]" },
  { "1-0:52.7.0", "gauge", "voltage_l2", "Instantaneous voltage (U) in phase L2 [V]" },
  { "1-0:72.7.0", "gauge", "voltage_l3", "Instantaneous voltage (U) in phase L3 [V]" },
  { "1-0:31.7.0", "gauge", "current_l1", "Instantaneous current (I) in phase L1 [A]" },
  { "1-0:51.7.0", "gauge", "current_l2", "Instantaneous current (I) in phase L2 [A]" },
  { "1-0:71.7.0", "gauge", "current_l3", "Instantaneous current (I) in phase L3 [A]" }
};