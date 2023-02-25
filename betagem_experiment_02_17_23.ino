#include "Servo.h"
#include <ArduinoBearSSL.h>
#include <ArduinoECCX08.h>
#include <ArduinoMqttClient.h>
#include <WiFiNINA.h> // change to #include <WiFi101.h> for MKR1000
#include "arduino_secrets.h"

int LEDprobe = 3;
int servoPin = 4;
int apdPin = A5;
Servo myServo;
int pos=0;

const int wifi_status_LED = 6;
const int aws_status_LED = 7;

const int NUM_PARAMETERS = 6;
int Parameters[NUM_PARAMETERS] = {0, 90,18,10,2,2};
String pLabels[NUM_PARAMETERS] = {"servoPosStart" , "servoPosEnd", "delta" ,"NumDataPoints" ,"EXPOSURETIME", "MOVEMENTTIME"};
//Begin Individual Params
int servoPosStart = Parameters[0];
int servoPosEnd = Parameters[1];
int delta=Parameters[2];
int Ndata=Parameters[3];
int EXPOSURETIME = Parameters[4]; //ms
int MovementTime = Parameters[5]; //ms
//End params
const int buttonPin = A4;


/////// Enter your sensitive data in arduino_secrets.h
const char ssid[]        = SECRET_SSID;
const char pass[]        = SECRET_PASS;
const char broker[]      = SECRET_BROKER;
const char* certificate  = R"EOF(-----BEGIN CERTIFICATE-----
MIIChDCCAWygAwIBAgIUKEC8InoM228xglrxlqqW/BL3bhYwDQYJKoZIhvcNAQEL
BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g
SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTIzMDIxNzE0NTMw
M1oXDTQ5MTIzMTIzNTk1OVowFDESMBAGA1UEAxMJTUtSX1dJRkkxMFkwEwYHKoZI
zj0CAQYIKoZIzj0DAQcDQgAExDVbHtWEkRdv6M1sng2nyeUpwlHsO/ayWwXc1aV1
DcZrJpAVQ7lvms+rFafrMciAbECM0TcThKgC8PYhO79Z0aNgMF4wHwYDVR0jBBgw
FoAUyGYvGdJ4iTS5LZrniapRAstGwfgwHQYDVR0OBBYEFDSOFI5rH1McXmpvde6z
pU4FVS/gMAwGA1UdEwEB/wQCMAAwDgYDVR0PAQH/BAQDAgeAMA0GCSqGSIb3DQEB
CwUAA4IBAQBlpYcMADj4J0WHxkIYyIvezYEq4mYB9aUNykR/D2oGgez7WI/lLVYL
08hgD6ELC8XEiHw7fzuNMkw1/imTd940H2IRhmrNYwIpWgA4OdkAgIlqIkVCFHnv
uLLBsLYLZ2QKveAn2RB0qJBgQHPmN39XTMV1q33Dnwo9DgEbhs8LoksxLVRUniff
OLwNbJCXMarbXy85Y316ebR7acaYWCslN/9MDYvpOH45gAv3s4ufiGm6vVVeA8Iu
syOjSl5Dgs46WXDduDPcUUKucUyx1r/NYsu+BN1UqxRAkvE6jtv0ndbZrp1chc/k
ai7GSObvUOWLK98YxHqiAipI4BzfoBCj
-----END CERTIFICATE-----
)EOF";

WiFiClient wifiClient;            // Used for the TCP socket connection
BearSSLClient sslClient(wifiClient); // Used for SSL/TLS connection, integrates with ECC508
MqttClient    mqttClient(sslClient);

String publishTopic = "MKR_WIFI_experiment"; //should send data to the same place
String subscribeTopic = "MKR_WIFI1"; //should only get commands for it
unsigned long lastMillis = 0;


void setup() {
  pinMode(LEDprobe, OUTPUT);
  pinMode(apdPin, INPUT);
  pinMode(buttonPin, INPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  pinMode(wifi_status_LED, OUTPUT);
  pinMode(aws_status_LED, OUTPUT);
  Serial.begin(9600);  // open the serial port
  ArduinoBearSSL.onGetTime(getTime); 
  sslClient.setEccSlot(2, certificate);
  mqttClient.onMessage(onMessageReceived);
  myServo.attach(servoPin);
  moveServo(servoPosStart);
}


const int COMMANDSIZE = 50;
char command[COMMANDSIZE];
int rlen = 0;

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(wifi_status_LED, LOW);
    connectWiFi();
  }else{
    digitalWrite(wifi_status_LED, HIGH);
  }

  if (!mqttClient.connected()) {
    // MQTT client is disconnected, connect
    digitalWrite(aws_status_LED, LOW);
    connectMQTT();
  }else{
    digitalWrite(aws_status_LED, HIGH);
    mqttClient.poll();
  }
  char incomingByte = 0;
  // put your main code here, to run repeatedly:
  if(Serial.available() > 0){
   rlen = Serial.readBytesUntil('\n',command,COMMANDSIZE);
    //get character received 
  }
  if(rlen != 0){
    String cmd = "";
    for(int i = 0; i < 4; i++){
      cmd += command[i];
    }
    
    if(cmd == "updt"){
      updateParameters(command, rlen);
    }else if(cmd == "getp"){
      getParameters();
    }else if(cmd == "strt"){
      runExperiment();
    }else{
      //Do nothing and ignore invalid commands
      //Throw them away 
      Serial.println(cmd);
    }
  }
  
}

String info = "";
//as long as everything is sent in the parameter order it should automatically assign everything to where it needs to go
void updateParameters(char command[], int rlen){
  mqttClient.beginMessage(publishTopic);
  int pCounter = 0; //counter for which parameter we are filling out
  int UpdateInfoStartIndex = 5; //since command is 4 and then we know this string starts with a "/"
  for(int i = UpdateInfoStartIndex; i < rlen; i++){
    if(command[i] == '/'){
      //skip and reset the info
      SendOut(pLabels[pCounter]);
      SendOut(" Changed to ");
      SendOut(String(info.toInt()));
      SendOut(" From ");
      SendOut(String(Parameters[pCounter]));
      SendOut("\n");
      Parameters[pCounter] = info.toInt(); //put the first token into paramters to update
      info = ""; //reset info for the next
      pCounter += 1;
    }else{
     info += command[i]; //add the current char to the info string
    }
    mqttClient.endMessage();
  }
  //reassign to individual data
  servoPosStart = Parameters[0];
  servoPosEnd = Parameters[1];
  delta=Parameters[2];
  Ndata=Parameters[3];
  EXPOSURETIME = Parameters[4]; //ms
  MovementTime = Parameters[5]; //ms
  Serial.print("!");
  Serial.flush();
}

String cmd_to_send = "";
char TOSEND[COMMANDSIZE];
//This method gets the current parameters and sends them to the other arduino board
void getParameters(){
  cmd_to_send = "retp/";
  for(int x = 0; x < NUM_PARAMETERS; x++){
    //cmd_to_send += pLabels[x]+":" + String(Parameters[x]) + "/";
    cmd_to_send += String(Parameters[x]) + "/";
  }
  cmd_to_send += "!";
  for(int i = 0; i < cmd_to_send.length(); i++){
    TOSEND[i] = cmd_to_send[i]; //copy everything over into char array
  }  
  mqttClient.beginMessage(publishTopic);
  SendOut(TOSEND);
  mqttClient.endMessage();
  Serial.flush();
}


double MAXSERVOANGLE = 270;
double MINSERVOANGLE = 0;
double MAXMICROSECONDS = 2500;
double MINMICROSECONDS = 500;
void moveServo(double degree){
  double microseconds = map(degree, MINSERVOANGLE,MAXSERVOANGLE,MINMICROSECONDS,MAXMICROSECONDS);
  myServo.writeMicroseconds(microseconds);
}

double AvOnIntensity, AvOffIntensity, Intensity = 0;
int datacounter = 0;
void runExperiment(){
  //Serial.flush();
  mqttClient.beginMessage(publishTopic);
  SendOut("Pos   ");
    SendOut("Av On Intensity   ");
    SendOut("Av Off Intensity");
    SendOut("\n");
    for (pos = servoPosStart + delta; pos <= servoPosEnd; pos += delta) { // goes from 0 degrees to 180 degrees
      // in steps of delta degrees
      // Serial.print("pos= ");
      SendOut(String(pos));
      SendOut("   ");
      moveServo(pos); 
      delay(MovementTime);             // tell servo to go to position in variable 'pos'
       // waits 20 ms (serial port problem if >> 20) for the servo to reach the position;
      digitalWrite(LEDprobe,HIGH);
      AvOnIntensity=0;
      //Serial.print("OnIntensity = ");
      for(datacounter=0; datacounter < Ndata; datacounter++) {
        Intensity=analogRead(apdPin);
        //Serial.print(Intensity);
        delay(EXPOSURETIME);
        AvOnIntensity +=Intensity;
        }
      AvOnIntensity=AvOnIntensity/Ndata;
      //Serial.print("   AvOnIntensity = ");  
      SendOut(String(AvOnIntensity));
      SendOut("           ");
      digitalWrite(LEDprobe,LOW);
      AvOffIntensity=0;
      for(datacounter=0; datacounter < Ndata; datacounter++) {
        Intensity=analogRead(apdPin);
        delay(EXPOSURETIME);
        AvOffIntensity +=Intensity;
        }
      AvOffIntensity=AvOffIntensity/Ndata;
      //Serial.print("   AvOffIntensity = ");  
      SendOut(String(AvOffIntensity));
      SendOut("\n");  
      } //servo steps for end
      SendOut("\n");
      //passFlag++;
  // Serial.println(passFlag);
    // ArduinoCloud.update();
  SendOut("!");
  moveServo(servoPosStart);
  Serial.flush();
  mqttClient.endMessage();
}

void onMessageReceived(int messageSize) {
  // we received a message, print out the topic and contents
  Serial.print("Received a message with topic '");
  Serial.print(mqttClient.messageTopic());
  Serial.print("', length ");
  Serial.print(messageSize);
  Serial.println(" bytes:");

  // use the Stream interface to print the contents
  String str = "";
  while (mqttClient.available()) {
    str += (char)mqttClient.read();
  }
  if(str == "begin"){
    runExperiment();
  }else if(str == "get"){
    getParameters();
  }else if(str.indexOf("updt") != -1){
    updateParameters((char*)str.c_str(), messageSize);
  }
}

unsigned long getTime() {
  // get the current time from the WiFi module  
  return WiFi.getTime();
}

void connectWiFi() {
  Serial.print("Attempting to connect to SSID: ");
  Serial.print(ssid);
  Serial.print(" ");

  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    // failed, retry
    Serial.print(".");
    delay(500);
  }
  Serial.println();

  Serial.println("You're connected to the network");
  Serial.println();
}

void connectMQTT() {
  Serial.print("Attempting to MQTT broker: ");
  Serial.print(broker);
  Serial.println(" ");

  while (!mqttClient.connect(broker, 8883)) {
    // failed, retry
    Serial.print(".");
    delay(500);
  }
  Serial.println();

  Serial.println("You're connected to the MQTT broker");
  Serial.println();

  // subscribe to a topic
  mqttClient.subscribe(subscribeTopic);
}

void SendOut(String str){
  mqttClient.print(str);
  Serial.print(str);
}