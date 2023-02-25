//Non-Iot Version
// #include "arduino_secrets.h"

// ArduinoIoTCloud - Version: 1.7.0
// #include <ArduinoIoTCloud.h>
// #include "thingProperties.h"
#include "Servo.h"

int LED = 3;
int servoPin=4;
int servoPosStart = 0;
int servoPosEnd = 180;

int apdPin = A5;
int buttonPin = A4;

double pos=0;
int delta=5;
Servo myServo;
double Intensity, AvOnIntensity, AvOffIntensity, datacounter;
int Ndata=10;


void setup() {
  pinMode(LED, OUTPUT);
  pinMode(buttonPin, INPUT);
  pinMode(apdPin, INPUT);
  Serial.begin(9600);  // open the serial port 
  //all your definitions  
  myServo.attach(servoPin);
  moveServo(servoPosStart);
  }
  

void loop() {
    int val = digitalRead(buttonPin);
    while(!val){
      //spin lock
      val = digitalRead(buttonPin);
      delay(200);
    }
    runExperiment();
    
    //Serial.println(val);
  }


double MAXSERVOANGLE = 270;
double MINSERVOANGLE = 0;
double MAXMICROSECONDS = 2500;
double MINMICROSECONDS = 500;
void moveServo(double degree){
  double microseconds = map(degree, MINSERVOANGLE,MAXSERVOANGLE,MINMICROSECONDS,MAXMICROSECONDS);
  myServo.writeMicroseconds(microseconds);
}

void runExperiment(){
  Serial.print("Pos   ");
    Serial.print("Av On Intensity   ");
    Serial.println("Av Off Intensity");
    for (pos = servoPosStart + delta; pos <= servoPosEnd; pos += delta) { // goes from 0 degrees to 180 degrees
      // in steps of delta degrees
      // Serial.print("pos= ");
      Serial.print(pos);
      Serial.print("   ");
      moveServo(pos);              // tell servo to go to position in variable 'pos'
       // waits 20 ms (serial port problem if >> 20) for the servo to reach the position;
      digitalWrite(LED,HIGH);
      AvOnIntensity=0;
      //Serial.print("OnIntensity = ");
      for(datacounter=0; datacounter < Ndata; datacounter++) {
        Intensity=analogRead(apdPin);
        //Serial.print(Intensity);
        delay(5);
        AvOnIntensity +=Intensity;
        }
      AvOnIntensity=AvOnIntensity/Ndata;
      //Serial.print("   AvOnIntensity = ");  
      Serial.print(AvOnIntensity);
      Serial.print("           ");
      digitalWrite(LED,LOW);
      AvOffIntensity=0;
      for(datacounter=0; datacounter < Ndata; datacounter++) {
        Intensity=analogRead(apdPin);
        delay(5);
        AvOffIntensity +=Intensity;
        }
      AvOffIntensity=AvOffIntensity/Ndata;
      //Serial.print("   AvOffIntensity = ");  
      Serial.println(AvOffIntensity);  
      } //servo steps for end
      Serial.println();
      //passFlag++;
  // Serial.println(passFlag);
    // ArduinoCloud.update();
  moveServo(servoPosStart);
  Serial.print('!');
  Serial.flush();
}


  
