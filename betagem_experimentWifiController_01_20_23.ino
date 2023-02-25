#include <WiFiNINA.h>
#include "secrets.h"
#include <Arduino.h>
#include <wiring_private.h>

const int NUM_PARAMETERS = 6;
int Parameters[NUM_PARAMETERS] = {0, 180,5,10,300,10};
String pLabels[NUM_PARAMETERS] = {"servoPosStart" , "servoPosEnd", "delta" ,"NumDataPoints" ,"EXPOSURETIME", "MOVEMENTTIME"};
//^above info is for the experiment itself
int WifiStatusPin = 6;
int WifiDataPin = 7;
String html ="<!DOCTYPE html> <html lang=\"en\"> <head> <meta charset=\"utf-8\"> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, shrink-to-fit=no\"> <title>Betagem Experiment Dashboard</title> </head> <body> <form> <div class=\"row\"> <div class=\"col\"> <h1>Pollux Technologies: Air Filter Monitoring Experiment Dashboard</h1> <div class=\"row\"> <div class=\"col text-center\"> <button class=\"btn btn-primary\" type=\"submit\" name=\"send\">Send Parameters</button> <button class=\"btn btn-primary\" style=\"visibility:hidden\" type=\"button\"></button> <button class=\"btn btn-primary\" type=\"submit\" name=\"begin\">Begin Experiment</button></div> </div> </div> </div> <div class=\"container\"> <div class=\"row\"> <div class=\"col-md-6\"><label class=\"col-form-label\" for=\"start\">Servo Start Position (0-180)</label></div> <div class=\"col-md-6\"><input type=\"text\" id=\"start\" name=\"start\" value=\"/startValue/\"></div> </div> </div> <div class=\"container\"> <div class=\"row\"> <div class=\"col-md-6\"><label class=\"col-form-label\" for=\"end\">Servo End Position (0-180)</label></div> <div class=\"col-md-6\"><input type=\"text\" id=\"end\" name=\"end\" value=\"/endValue/\"></div> </div> </div> <div class=\"container\"> <div class=\"row\"> <div class=\"col-md-6\"><label class=\"col-form-label\" for=\"delta\">Delta (Value to move armature)</label></div> <div class=\"col-md-6\"><input type=\"text\" id=\"delta\" name=\"delta\" value=\"/deltaValue/\"></div> </div> </div> <div class=\"container\"> <div class=\"row\"> <div class=\"col-md-6\"><label class=\"col-form-label\" for=\"NumData\">Number of data points to take</label></div> <div class=\"col-md-6\"><input type=\"text\" id=\"NumData\" name=\"NumData\" value=\"/NumDataValue/\"></div> </div> </div> <div class=\"container\"> <div class=\"row\"> <div class=\"col-md-6\"><label class=\"col-form-label\" for=\"between\">Delay between data points</label></div> <div class=\"col-md-6\"><input type=\"text\" id=\"between\" name=\"between\" value=\"/betweenValue/\"></div> </div> </div> <div class=\"container\"> <div class=\"row\"> <div class=\"col-md-6\"><label class=\"col-form-label\" for=\"exposure\">Exposure Time</label></div> <div class=\"col-md-6\"><input type=\"text\" id=\"exposure\" name=\"exposure\" value=\"/exposureValue/\"></div> </div> </div> </form> <h1>Output : Average Intensity Read from APD</h1> <div class=\"row\"> <div class=\"col\"><button class=\"btn btn-primary\" type=\"button\">Download Results</button></div> </div> </body> </html>";
String html_to_serve="";
char ssid[] = mySSID;        // your network SSID (name)
char pass[] = myPASS;    // your network password (use for WPA, or use as key for WEP)
int status = WL_IDLE_STATUS;     // the Wifi radio's status
WiFiServer server(80);

//Begin UART definitions
#define PIN_SERIAL_c0_TX       (2ul)                // Pin description number for PIO_SERCOM on D2
#define PIN_SERIAL_c0_RX       (3ul)                // Pin description number for PIO_SERCOM on D3
#define PAD_SERIAL_c0_TX       (UART_TX_PAD_2)      // SERCOM pad 2 TX
#define PAD_SERIAL_c0_RX       (SERCOM_RX_PAD_3)    // SERCOM pad 3 RX
Uart Serial_c0(&sercom0, PIN_SERIAL_c0_RX, PIN_SERIAL_c0_TX, PAD_SERIAL_c0_RX, PAD_SERIAL_c0_TX);

void SERCOM0_Handler()    // Interrupt handler for SERCOM0
{
  Serial_c0.IrqHandler();
}

//End UART definitions

void setup() {
  pinMode(WifiStatusPin, OUTPUT);
  pinMode(WifiDataPin, OUTPUT);
  Serial.begin(9600);  // open the serial port 
  pinPeripheral(2, PIO_SERCOM);   //Sercom Multiplexing
  pinPeripheral(3, PIO_SERCOM);   //for alternative pads, PIO_SERCOM_ALT has to be used
  Serial_c0.begin(4800); //open this serial port
  status = WiFi.beginAP(ssid,pass);
  //Serial.println("Creating access point: ");
  //Serial.println(ssid);
  if(status != WL_AP_LISTENING){
    //something went wrong
    //Serial.println("Something went wrong creating access point");
  }
  server.begin();
  printWiFiStatus();
}
  

void loop() {
  if (status != WiFi.status()) {
    // it has changed update the variable
    status = WiFi.status();
    if (status == WL_AP_CONNECTED) {
      // a device has connected to the AP
      //Serial.println("Device connected to AP");
      digitalWrite(WifiStatusPin,HIGH);
    } else {
      //Serial.println("Device disconnected from AP");
      digitalWrite(WifiStatusPin,LOW);
    }
  }

  WiFiClient client = server.available();
  
  if(client){
    String currentLine = "";
    while(client.connected()){

      if(client.available()){
        char c = client.read();
        //Serial.write(c);
        if(c == '\n'){
          digitalWrite(WifiDataPin, HIGH);
          if(currentLine.length() == 0){
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
            updatePageValues();
            client.println(html_to_serve);
            client.println();
            digitalWrite(WifiDataPin, LOW);
            break;
          }else{
            route(currentLine);
            currentLine = "";
          }
        }else if(c!= '\r'){
          currentLine +=c;
        }
      }
    }  
  }
}

void printWiFiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);
}

void route(String currentLine){
  
  if((currentLine.indexOf("/?send") > 0)){
    pushNewParameters(currentLine);
  }else if(currentLine.indexOf("/?begin") > 0 ){
    startExperiment();
  }
}

void addtooutput(){
  String stringtoReplace = "%OUTPUT%";
    String currentString = "";
    char data[] = {'a','b','c','d','e'};
    for(int i = 0; i < 5; i++){
      html.replace(stringtoReplace, currentString + "," + data[i]);
      currentString = currentString + ","+ data[i];
      stringtoReplace = currentString;
    }
}

//this method sends the parameters from the Parameters array in the order, seperated by '/'
//the string sent should end in a new line character '\n'

void updateParameters(){
    //This will be more involved because it will involve the webpage integration
    Serial_c0.write("updt/1/170/25/5/300/10\n");
}

//Sends request to experiment microcontroller 
//Decodes request
int rlen = 0;
const int CMDSIZE = 50; // 50 byte buffer
char cmd[CMDSIZE];
String info = "";
//END Function specific globals

void getParameters(){
  Serial_c0.write("getp\n");
  String key = "";
  //wait for response/ microcontroller to do processing
  delay(100);
  if(Serial_c0.available() > 0){
    rlen = Serial_c0.readBytesUntil('\n', cmd, CMDSIZE);
  }
  if(rlen != 0){
    for(int i = 0; i < 4; i++){
      key += cmd[i];
   }
  
  if(key == "retp"){
    Serial.println("Gotten Parameters");
    Serial.println();
    int pCounter = 0; //counter for which parameter we are filling out
    int UpdateInfoStartIndex = 5; //since command is 4 and then we know this string starts with a "/"
    for(int i = UpdateInfoStartIndex; i < rlen; i++){
      if(cmd[i] == '/'){
        //print and reset info
        Serial.print(pLabels[pCounter]);
        Serial.print(" is ");
        Serial.print(info.toInt());
        Serial.println();
        Parameters[pCounter] = info.toInt(); //put the first token into paramters to update
        //change the saved parameters on this side so that its verified;
        info = ""; //reset info for the next
        pCounter += 1;
      }else{
       info += cmd[i]; //add the current char to the info string
      }
    }
   }else{
    Serial.print("received invalid return for getting parameters :");
    Serial.print(key);
    Serial.println();
   }
 }else{
  Serial.println("Response length was 0");
 }
}

void startExperiment(){
  Serial_c0.write("strt\n");
}


//This method is intended to take the data that is in RAM for the 6 parameters and update the fields on the webpage by doing basic string manipulation 
//The keys are the same as long as the global variable 'html' is never changed. 
String keys[NUM_PARAMETERS] = {"/startValue/", "/endValue/", "/deltaValue/", "/NumDataValue/", "/exposureValue/", "/betweenValue/"};
void updatePageValues(){
  getParameters();
  //looking for something like
  //value=\"/startValue/\";
  html_to_serve = html;
  for(int i = 0; i < NUM_PARAMETERS; i++){
    html_to_serve.replace(keys[i], String(Parameters[i]));
  }
}

void pushNewParameters(String line){
  //This is ran when the GET /?Send= HTTP request is received
  //Send ALL traffic /?Send here
  //This is the method where once we have put new parameters in, need to decode the http method and update the RAM values here, and then run updateParameters()
  updateParameters();
}
