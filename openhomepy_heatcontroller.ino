#include "MQTT.h"

void callback(char* topic, byte* payload, unsigned int length);
/**
 * if want to use IP address,
 * byte server[] = { XXX,XXX,XXX,XXX };
 * MQTT client(server, 1883, callback);
 * want to use domain name,
 * MQTT client("www.sample.com", 1883, callback);
 **/

// controller id
#define GUID "heat_controller_1"
#define PUMP_DELAY 10000

byte server[] = { 192,168,43,234 };
MQTT client(server, 1883, callback);

// define devices to expose
String DEVICES[] = {"SWITCH1", "SWITCH2", "SWITCH3"};
String STATUSES[] = {"off", "off", "off"};
int TIMES[] = {0, 0, 0};
int PINS[] = {D3, D4, D5};

int PUMP_PIN = D6;

// devices/guid/device/status

// recieve message
void callback(char* topic, byte* payload, unsigned int length) {
    char p[length + 1];
    memcpy(p, payload, length);
    p[length] = NULL;
    String command(p);
    String thetopic(topic);

    String device_id = thetopic.substring(thetopic.lastIndexOf("/", thetopic.lastIndexOf("/")-1)+1, thetopic.lastIndexOf("/"));

    int device_num = getDeviceNum(device_id);

    processCommand(device_num, command);

    delay(1000);
}

int getDeviceNum(String device_id){
  if (device_id==DEVICES[0]){
    return 0;
  } else if (device_id==DEVICES[1]){
    return 1;
  } else if (device_id==DEVICES[2]){
    return 2;
  }
}

void setup() {
    RGB.control(true);

    // initialize relays
    pinMode(PINS[0], OUTPUT);
    pinMode(PINS[1], OUTPUT);
    pinMode(PINS[2], OUTPUT);
    pinMode(PUMP_PIN, OUTPUT);

    digitalWrite(PINS[0], LOW);
    digitalWrite(PINS[1], LOW);
    digitalWrite(PINS[2], LOW);
    digitalWrite(PUMP_PIN, LOW);

    // connect to the server
    client.connect(GUID);

    if (client.isConnected()) {
        // subscribe
        client.subscribe(String("devices/") + GUID + "/+/command");

        // send init messages to hub
        init();
    }
}

void init() {
  client.publish(String("devices/")+GUID+"/"+DEVICES[0]+"/init", "switch");
  client.publish(String("devices/")+GUID+"/"+DEVICES[1]+"/init", "switch");
  client.publish(String("devices/")+GUID+"/"+DEVICES[2]+"/init", "switch");
}

void processCommand(int device_num, String command) {
  if(command=="on"){
    digitalWrite(PINS[device_num], HIGH);
    STATUSES[device_num]="on";
    TIMES[device_num]=millis();
    sendDeviceStatus(device_num);
  } else if(command=="off"){
    digitalWrite(PINS[device_num], LOW);
    STATUSES[device_num]="off";
    sendDeviceStatus(device_num);
  } else if(command=="toggle"){
    if(STATUSES[device_num]=="off"){
      digitalWrite(PINS[device_num], HIGH);
      STATUSES[device_num]="on";
      TIMES[device_num]=millis();
      sendDeviceStatus(device_num);
    }
    else{
      digitalWrite(PINS[device_num], LOW);
      STATUSES[device_num]="off";
      sendDeviceStatus(device_num);
    }
  } else if(command=="status"){
    sendDeviceStatus(device_num);
  }
}

void sendDeviceStatus(int device_num){
  client.publish(String("devices/")+GUID+"/"+DEVICES[device_num]+"/status", STATUSES[device_num]);
}

void loop() {
    if (client.isConnected())
        client.loop();
    else
        client.connect(GUID);

    int i;
    bool pump_on = false;
    for (i=0;i<3;i=i+1){
      if(STATUSES[i]=="on" && millis()-TIMES[i]>PUMP_DELAY)
          pump_on = true;
    }
    if (pump_on)
        digitalWrite(PUMP_PIN, HIGH);
    else
        digitalWrite(PUMP_PIN, LOW);
}
