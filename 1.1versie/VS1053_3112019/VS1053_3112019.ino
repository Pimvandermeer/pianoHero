#include <SPI.h>
#include <Adafruit_VS1053.h>
#include <SD.h>

#define RESET  8      // VS1053 reset pin (output)
#define CS     6     // VS1053 chip select pin (output)
#define DCS    7      // VS1053 Data/command select pin (output)
#define CARDCS 9     // Card chip select pin
#define DREQ 2       // VS1053 Data request, ideally an Interrupt pin

Adafruit_VS1053_FilePlayer musicPlayer = Adafruit_VS1053_FilePlayer(RESET, CS, DCS, DREQ, CARDCS);

int playPin = 5;  //listens of the horn is picked up

int playButton;
bool nextPickUp = false;

int bel1 = A0;   //solenoid1 ringing the bell
int bel2 = A1;  //solenoid2 ringing the bell

unsigned long previousMillis = 0;
unsigned long previousMillis2 = 0;

long timeNoAction = 120000;     //if there is nobody at the piano ring after 3 minutes
long ringInterval = 30000; //if there is nobody ring every 30 seconds

//debouncing globals
int playButtonState = LOW;
int playButtonPrev = LOW;
unsigned long debouncePlayButtonTime = 0;
const int debouncePlayButtonDelay = 50;




void setup() {
  Serial.begin(9600);
  Serial.println("Adafruit VS1053 Simple Test");

  if (! musicPlayer.begin()) { // initialise the music player
       Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
    while (1);
  }
   Serial.println(F("VS1053 found"));

  if (!SD.begin(CARDCS)) {
        Serial.println(F("SD failed, or not present"));
    while (1);  // don't do anything more
  }

  // Set volume for left, right channels. lower numbers == louder volume!
  musicPlayer.setVolume(80, 0);

  pinMode(playPin, INPUT_PULLUP);
  pinMode(bel1, OUTPUT);
  pinMode(bel2, OUTPUT);
  digitalWrite(bel1, LOW);
  digitalWrite(bel2, LOW);


  Serial.begin(9600);  //set midi baud rate
}

void loop() {
  unsigned long currentMillis = millis();
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

  

  if (playButtonState == HIGH && nextPickUp == true) {  //should be HIGH if the horn is really used
    Serial.println(F("Playing track 001"));
    delay(2000); //horn should be close to the listener
    musicPlayer.playFullFile("/track001.mp3");
    nextPickUp = false;
    previousMillis = currentMillis;  // Remember the time
  }

  if (playButtonState == LOW) {
    //delay(700);
    nextPickUp = true;
  }

  if (playButtonState == LOW && (currentMillis - previousMillis >= timeNoAction )) {
    if (currentMillis - previousMillis2 >= ringInterval) {
      Serial.println("bellen");
      for (int i = 0; i < 15; i++) {
        digitalWrite(bel1, HIGH);
        digitalWrite(bel2, HIGH);
        delay(25);
        digitalWrite(bel1, LOW);
        digitalWrite(bel2, LOW);
        delay(25);
      }
      previousMillis2 = currentMillis;  // Remember the time
    }
  }

}
