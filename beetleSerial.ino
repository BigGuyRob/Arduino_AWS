const int NUM_PARAMETERS = 6;
int Parameters[NUM_PARAMETERS] = {0, 180,5,10,300,10};
String pLabels[NUM_PARAMETERS] = {"servoPosStart" , "servoPosEnd", "delta" ,"NumDataPoints" ,"EXPOSURETIME", "MOVEMENTTIME"};

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial1.begin(4800);
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
      startExperiment();
    }else{
      //Do nothing and ignore invalid commands
    }
  }
}


void startExperiment(){
  Serial.println("Started Experiment");
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
