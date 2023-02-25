#include "Servo.h"

int LEDprobe = 10;
int servoPin = 9;
int apdPin = A0;
int recDatapin = 11;
Servo myServo;
int pos=0;

const int NUM_PARAMETERS = 6;
int Parameters[NUM_PARAMETERS] = {0, 180,5,10,300,10};
String pLabels[NUM_PARAMETERS] = {"servoPosStart" , "servoPosEnd", "delta" ,"NumDataPoints" ,"EXPOSURETIME", "MOVEMENTTIME"};
//Begin Individual Params
int servoPosStart = Parameters[0];
int servoPosEnd = Parameters[1];
int delta=Parameters[2];
int Ndata=Parameters[3];
int EXPOSURETIME = Parameters[4]; //ms
int MovementTime = Parameters[5]; //ms
//End params

void setup() {
  pinMode(LEDprobe, OUTPUT);
  pinMode(recDatapin, OUTPUT);
  pinMode(apdPin, INPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.begin(9600);  // open the serial port 
  Serial1.begin(4800);
  myServo.attach(servoPin);
  myServo.write(servoPosStart);
}
  


const int COMMANDSIZE = 50;
void loop() {
  char incomingByte = 0;
  char command[COMMANDSIZE];
  int rlen = 0;
  // put your main code here, to run repeatedly:
  if(Serial1.available() > 0){
   rlen = Serial1.readBytesUntil("\n",command,COMMANDSIZE);
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
    }
  }
}

String info = "";
//as long as everything is sent in the parameter order it should automatically assign everything to where it needs to go
void updateParameters(char command[], int rlen){
  Serial.println("Updating Parameters");
  int pCounter = 0; //counter for which parameter we are filling out
  int UpdateInfoStartIndex = 5; //since command is 4 and then we know this string starts with a "/"
  for(int i = UpdateInfoStartIndex; i < rlen; i++){
    if(command[i] == '/'){
      //skip and reset the info
      Serial.print(pLabels[pCounter]);
      Serial.print("Changed to ");
      Serial.print(info.toInt());
      Serial.print(" From ");
      Serial.print(Parameters[pCounter]);
      Serial.println();
      Parameters[pCounter] = info.toInt(); //put the first token into paramters to update
      info = ""; //reset info for the next
      pCounter += 1;
    }else{
     info += command[i]; //add the current char to the info string
    }
  }
  //reassign to individual data
  servoPosStart = Parameters[0];
  servoPosEnd = Parameters[1];
  delta=Parameters[2];
  Ndata=Parameters[3];
  EXPOSURETIME = Parameters[4]; //ms
  MovementTime = Parameters[5]; //ms
}

String cmd_to_send = "";
char TOSEND[COMMANDSIZE];
//This method gets the current parameters and sends them to the other arduino board
void getParameters(){
  Serial.println("Getting Parameters");
  cmd_to_send = "retp/";
  for(int x = 0; x < NUM_PARAMETERS; x++){
    //cmd_to_send += pLabels[x]+":" + String(Parameters[x]) + "/";
    cmd_to_send += String(Parameters[x]) + "/";
  }
  cmd_to_send += "\n";
  for(int i = 0; i < cmd_to_send.length(); i++){
    TOSEND[i] = cmd_to_send[i]; //copy everything over into char array
  }
  Serial1.write(TOSEND);
  Serial.write(TOSEND);  
}

void runExperiment(){
  double averageIntensityData[Ndata * 2];
  pos = servoPosStart;
  bool LEDSTATE = true;
  for(int counter = 0; counter < Ndata * 2; counter++){
    double readVal = 0;
    double AvIntensity = 0;
    pos += delta;
    myServo.write(pos);
    for(int x = 0; x < 2; x++){

      for(int i = 0; i < Ndata; i++){
        digitalWrite(LEDprobe, (LEDSTATE == true));
        readVal = analogRead(apdPin);
        AvIntensity += readVal;
        LEDSTATE = !LEDSTATE;
        delay(EXPOSURETIME);
      }
      AvIntensity = AvIntensity/Ndata;
      averageIntensityData[counter] = AvIntensity;
    }
    delay(MovementTime);
  }
}
