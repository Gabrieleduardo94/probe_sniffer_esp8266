/*
    This sketch will try to keep a list of all devices in
    the vicinity by signaling probe requests it receives.

    When a device probes for AP's (and it's picked up by our ESP8266),
    the ESP will search a table to see if it's already known.
    If not: it is added to the list, if it's already known, the entry's
    timestanp will be updated.
    When a device is not seen for a certain amount of time, it is
    removed from the list again.


    Output to serial console:
    - When a new device is found
    - When an existing device has left (timed out)
   
    Input from serial console:
    - Any command (or just LF) will show a list of registered devices


    NOTE: This is my first sketch using C++ for the ESP8266 (in real
          life I'm actually a Pascal/Delphi programmer), so don't shoot
          me for any weird pieces of code.
          I hope to be as good as you in the near future.

    Code inspired by the WiFiEvents-example by Ivan Grokhotkov.
    This version of the code was written by Peter Luijer and
    is released into the public domain, or, at your option,
    CC0 licensed.
*/

#include <ESP8266WiFi.h>
#include <stdio.h>

typedef struct
  {
      uint8_t mac[6];
      int rssi;
      unsigned long ms;
  }  client_device;

const char* proberversion = "ESProber v.0.9b by QuickFix, (c)2017";
const int   maxdevices    = 255; // Max. number of devices to keep track off at a time
const int   listtimeout   = 120; // Consider device away if not seen for this numner of seconds
const char* ap_ssid       = "ESProber"; // Name for our AP
const char* ap_password   = "12345678"; // Password for our AP
                                 
WiFiEventHandler probeRequestHandler;
client_device devicelist[maxdevices];
String consolebuffer;


String macToString(const uint8 mac[6]) {
  char buf[20];
  snprintf(buf, sizeof(buf), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}

int adddevice(const uint8 mac[6], int rssi) {
  int d;
  d = -1; 
  int i;
  for (i = 0; i < maxdevices; i = i + 1) {
    if ((devicelist[i].mac[0] == 0) &&
        (devicelist[i].rssi == 0)) {
      d = i;
      devicelist[d].mac[0] = mac[0];
      devicelist[d].mac[1] = mac[1];
      devicelist[d].mac[2] = mac[2];
      devicelist[d].mac[3] = mac[3];
      devicelist[d].mac[4] = mac[4];
      devicelist[d].mac[5] = mac[5];
      devicelist[d].rssi   = rssi;
      devicelist[d].ms     = millis();
      break;
    }
  }
  return d;
}

void updatedevice(int index, int rssi) {
  devicelist[index].rssi = rssi;
  devicelist[index].ms   = millis();
}

void cleardevice(int index) {
  devicelist[index].mac[0] = 0;
  devicelist[index].rssi   = 0;
  devicelist[index].ms     = 0; 
}

void cleardevices() {
  int i;
  for (i = 0; i < maxdevices; i = i + 1) {
    cleardevice(i); 
  } 
}

int countdevices() {
  int d;
  d = 0; 
  int i;
  for (i = 0; i < maxdevices; i = i + 1) {
    if ((devicelist[i].mac[0] != 0) &&
        (devicelist[i].rssi != 0)) {
      d = d + 1;
    }
  }
  return d;
}

int finddevice(const uint8 mac[6]) {
  int d;
  d = -1; 
  int i;
  for (i = 0; i < maxdevices; i = i + 1) {
    if ((devicelist[i].mac[0] == mac[0]) &&
        (devicelist[i].mac[1] == mac[1]) &&
        (devicelist[i].mac[2] == mac[2]) &&
        (devicelist[i].mac[3] == mac[3]) &&
        (devicelist[i].mac[4] == mac[4]) &&
        (devicelist[i].mac[5] == mac[5])) {
      d = i;
      break;
    }   
  }
  return d;
}

void checklist() {
  int i; 
  for (i = 0; i < maxdevices; i = i + 1) {
    if ((devicelist[i].mac[0] != 0) &&
        (devicelist[i].rssi != 0) &&
        ((devicelist[i].ms + (listtimeout * 1000)) < millis())) {
      Serial.print("Device ");
      Serial.print(macToString(devicelist[i].mac));
      Serial.println(" has left");
      cleardevice(i);
    }
  }
}

void printlist() {
  int c;
  c = countdevices();
  Serial.println("");
  Serial.print(c); 
  Serial.print(" registered devices (prober running for ");
  Serial.print(millis() / 1000);
  Serial.println(" seconds):");
 
  if (c > 0) {
    int i;   
    for (i = 0; i < maxdevices; i = i + 1) {
      if ((devicelist[i].mac[0] != 0) &&
          (devicelist[i].rssi != 0))
      {
        Serial.print("MAC: ");
        Serial.print(macToString(devicelist[i].mac));
        Serial.print(" RSSI: ");
        Serial.print(devicelist[i].rssi);     
        Serial.print(" last seen ");
        Serial.print((millis() - devicelist[i].ms) / 1000);
        Serial.println(" seconds ago");
      }   
    }
  } 
}

void checkconsole() {
  char c;
 
  while (Serial.available() > 0) {
    c = Serial.read();
    if (c == '\n') {
      printlist();     
      consolebuffer = "";
    } else {
      consolebuffer +=c;
    }
  }
}

void onProbeRequest(const WiFiEventSoftAPModeProbeRequestReceived& evt) {
  int i;
  i = finddevice(evt.mac);
  if (i < 0) {
    i = adddevice(evt.mac, evt.rssi);
    if (i < 0) {
      Serial.print("Unable to add new device to list");
    } else {
      Serial.print("New device found");
    }
    Serial.print(", MAC: ");
    Serial.print(macToString(evt.mac));
    Serial.print(" RSSI: ");
    Serial.println(evt.rssi);   
  } else {
    updatedevice(i, evt.rssi);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println(proberversion); 
 
  Serial.println("Configuring wifi:");
  WiFi.mode(WIFI_AP);
 
  Serial.print("- SSID: ");
  Serial.println(ap_ssid);
  Serial.print("- Password: ");
  Serial.println(ap_password);
  WiFi.softAP(ap_ssid, ap_password);

  Serial.print("Maximum number of devices: ");
  Serial.println(maxdevices);
  cleardevices();
  Serial.print("Registration time-out (s): ");
  Serial.println(listtimeout);

  probeRequestHandler = WiFi.onSoftAPModeProbeRequestReceived(&onProbeRequest);
  Serial.println("Probing started");
}

void loop() {
  checklist();
  checkconsole(); 
  delay(10);
}