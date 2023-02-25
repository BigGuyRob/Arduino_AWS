#include <Arduino.h>
#include <wiring_private.h>

#define PIN_SERIAL_c0_TX       (2ul)                // Pin description number for PIO_SERCOM on D2
#define PIN_SERIAL_c0_RX       (3ul)                // Pin description number for PIO_SERCOM on D3
#define PAD_SERIAL_c0_TX       (UART_TX_PAD_2)      // SERCOM pad 2 TX
#define PAD_SERIAL_c0_RX       (SERCOM_RX_PAD_3)    // SERCOM pad 3 RX
const int NUM_PARAMETERS = 6;
int Parameters[NUM_PARAMETERS] = {0, 180,5,10,300,10};
String pLabels[NUM_PARAMETERS] = {"servoPosStart" , "servoPosEnd", "delta" ,"NumDataPoints" ,"EXPOSURETIME", "MOVEMENTTIME"};


Uart Serial_c0(&sercom0, PIN_SERIAL_c0_RX, PIN_SERIAL_c0_TX, PAD_SERIAL_c0_RX, PAD_SERIAL_c0_TX);

void SERCOM0_Handler()    // Interrupt handler for SERCOM0
{
  Serial_c0.IrqHandler();
}

void setup(){
    pinPeripheral(2, PIO_SERCOM);   //Sercom Multiplexing
    pinPeripheral(3, PIO_SERCOM);   //for alternative pads, PIO_SERCOM_ALT has to be used
    Serial_c0.begin(4800);
    Serial.begin(9600);
}

void loop(){
  Serial.println("Updating");
  updateParameters();
  delay(3000);
  Serial.println("Getting");
  getParameters();
  delay(3000);
  Serial_c0.write("strt\n");
  Serial.println("Starting");
  delay(3000);
  //just boomerang
  Serial_c0.write("invalidcommand\n");
  Serial.println("Sending Invalid Command");
  delay(3000);
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
