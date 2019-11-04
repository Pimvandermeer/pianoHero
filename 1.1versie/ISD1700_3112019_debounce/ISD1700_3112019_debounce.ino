#include <ISD1700.h>
ISD1700 chip(10); // Initialize chipcorder with
                  // SS at Arduino's digital pin 10

int playPin = 5;
int recordPin = 6;
int playLedPin = A0;
int recordLedPin = A1;

int playButtonState = HIGH;
int playButtonPrev = HIGH;
unsigned long debouncePlayButtonTime = 0;
const int debouncePlayButtonDelay = 50;

int recButtonState = LOW;
int recButtonPrev = LOW;
unsigned long debounceRecButtonTime = 0;
const int debounceRecButtonDelay = 50;

bool resetState = false;
bool stopState = false;
bool playedState = false;

int apc = 0;

int vol = 0; //volume 0=MAX, 7=min
int startAddr = 0x10;
int endAddr = 0x2DF;
int inInt = 0;

void setup() {
  apc = apc | vol; //D0, D1, D2
  //apc = apc | 0x8; //D3 comment to disable output monitor during record
  //apc = apc | 0x50; // D4& D6 select MIC REC
  apc = apc | 0x00; // D4& D6 select AnaIn REC
  //apc = apc | 0x10; // D4& D6 select MIC + AnaIn REC
  apc = apc | 0x80; // D7 AUX ON, comment enable AUD
  apc = apc | 0x100; // D8 SPK OFF, comment enable SPK
  //apc = apc | 0x200; // D9 Analog OUT OFF, comment enable Analog OUT
  //apc = apc | 0x400; // D10 vAlert OFF, comment enable vAlert
  apc = apc | 0x800; // D11 EOM ON, comment disable EOM

  pinMode(playPin, INPUT_PULLUP);
  pinMode(recordPin, INPUT_PULLUP);

  pinMode(playLedPin, OUTPUT);
  pinMode(recordLedPin, OUTPUT);

  Serial.begin(9600);
  Serial.println("Sketch is starting up");
}

void loop() {
  chip.pu();


  //Debounce playbutton
  int playButtonRead = digitalRead(playPin);
  if (playButtonRead != playButtonPrev) {
    debouncePlayButtonTime = millis();
  }

  if(millis() > (debouncePlayButtonTime + debouncePlayButtonDelay)) {
    if (playButtonRead != playButtonState) {
      playButtonState = playButtonRead; 
    }
  }
  playButtonPrev = playButtonRead;

  //Debounce recordbutton
  int recButtonRead = digitalRead(recordPin);
  if(recButtonRead != recButtonPrev) {
    debounceRecButtonTime = millis();    
  }

  if(millis() > (debounceRecButtonTime + debounceRecButtonDelay)) {
    if (recButtonRead != recButtonState) {
      recButtonState = recButtonRead;
    }
  }
  recButtonPrev = recButtonRead;


  if (playButtonState == LOW) {
      digitalWrite(playLedPin, HIGH);
      chip.play();
      
      Serial.println("play");
  }


  if (recButtonState == LOW && resetState == true && ((chip.RDY()? "RDY": "Not_RDY ") == "RDY")) {
    
    delay(1200);  //heel even wachten voordat die gaat afspelen
    //digitalWrite(playLedPin, HIGH);   //deze nu uitcommenten als test
    //chip.play();
    
    Serial.print("playtest");

    resetState = false;
  }
  

  if (recButtonState == HIGH && stopState == false) {  //Deze moet HIGH zijn als de hoorn wordt gebruikt voor nu als reversed
      chip.erase();  //take care of cahing of the chip

      Serial.println("erase");
      
      chip.wr_apc1(apc);  //change chip register for analog-in
      chip.wr_apc2(apc);
      chip.pu();

      delay(1000);
     // delay(26400);  //First people need to listen to the MP3 instruction
  
      digitalWrite(recordLedPin, HIGH);
      chip.rec();
  
      Serial.println("record");
      resetState = false; //first chip needs to be reset to be able to play
      stopState = true;
  }

  if (recButtonState == LOW && stopState == true) { //Deze moet LOW xijn als de hoorn wordt gebruikt voor nu als reversed
      chip.stop();
      digitalWrite(recordLedPin, LOW);
      resetState = true;
      stopState = false;
      
      chip.reset();
      
      Serial.println("stopped with recording");
      
  }

  if ((chip.RDY()? "RDY": "Not_RDY ") == "RDY") {  //If nothing happens chip is RDY and therefore both bulbs can be down
     // Serial.println("stopped with everything");
      digitalWrite(playLedPin, LOW);
      digitalWrite(recordLedPin, LOW);
  }

}
