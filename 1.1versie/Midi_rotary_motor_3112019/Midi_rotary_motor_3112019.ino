#include <SdFat.h>
#include <MD_MIDIFile.h>

#define USE_MIDI  1   // set to 1 to enable MIDI output, otherwise debug output

#if USE_MIDI // set up for direct MIDI serial output

#define DEBUG(x)
#define DEBUGX(x)
#define DEBUGS(s)
#define SERIAL_RATE 31250

#else // don't use MIDI to allow printing debug statements

#define DEBUG(x)  Serial.print(x)
#define DEBUGX(x) Serial.print(x, HEX)
#define DEBUGS(s) Serial.print(F(s))
#define SERIAL_RATE 57600

#endif // USE_MIDI


// SD chip select pin for SPI comms.
// Arduino Ethernet shield, pin 4.
#define  SD_SELECT  4

#define WAIT_DELAY    2000 // ms

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

int songNumber = 0;
int listenToInput = 1;

//rotaryMotor
int needToPrint = 0;
int count = 0;
int in = 12;
int lastState = LOW;
int trueState = LOW;
long lastStateChangeTime = 0;
int cleared = 0;

// constants

int dialHasFinishedRotatingAfterMs = 100;
int debounceDelay = 10;

long playTime = 0;
long stoppedPlayTime = 46000;

int playLedPin = A0;

// The files in the tune list should be located on the SD card 
// or an error will occur opening the file and the next in the 
// list will be opened (skips errors).
const char *tuneList[] = 
{
  "ALLA.MID",
  "ALLA.MID",  // simplest and shortest file
  "BEAT.MID",
  "CLAIR.MID",
  "IMAG.MID",
  "GYM.MID",
  "SAND.MID",
  "JOEP.MID",
  "IPAN.MID",
  "PREL.MID",
  "PIAN.MID",
};

SdFat SD;
MD_MIDIFile SMF;

void setup(void) {
  Serial.begin(SERIAL_RATE);

  DEBUG("\n[MidiFile Play List]");

  // Initialize SD
  if (!SD.begin(SD_SELECT, SPI_FULL_SPEED))
  {
    DEBUG("\nSD init fail!");
    while (true) ;
  }

  // Initialize MIDIFile
  SMF.begin(&SD);
  SMF.setMidiHandler(midiCallback);
  SMF.setSysexHandler(sysexCallback);

  //rotarymotor;
  pinMode(in, INPUT);
  pinMode(playLedPin, OUTPUT);
}


void loop(void) {
//  if (listenToInput) {
      rotaryMotor();
      if (songNumber) {
        playMidi();
      }  
  //}
  
}

void playMidi() {
    
    static enum { S_IDLE, S_PLAYING, S_END, S_WAIT_BETWEEN } state = S_IDLE;
    static uint32_t timeStart;
  switch (state)  {  
  case S_IDLE:    // now idle, set up the next tune
    {
      int err;

   //   DEBUGS("\nS_IDLE");

      // use the next file name and play it
   //   DEBUG("\nFile: ");
      DEBUG(tuneList[songNumber]);
      SMF.setFilename(tuneList[songNumber]);
      err = SMF.load();
      if (err != -1){
  //      DEBUG(" - SMF load Error ");
        DEBUG(err);
        timeStart = millis();
        state = S_WAIT_BETWEEN;
   //     DEBUGS("\nWAIT_BETWEEN");
      }
      else {
     //   DEBUGS("\nS_PLAYING");
        state = S_PLAYING;
        playTime = millis();
        digitalWrite(playLedPin, HIGH);
        listenToInput = 0;
      }
    }
    break;
   

  
  case S_PLAYING: // play the file
    DEBUGS("\nS_PLAYING");
    if ((millis() - playTime) > stoppedPlayTime) {
      state = S_END;
      Serial.println("Muziek gestopt");
      digitalWrite(playLedPin, LOW);
      listenToInput = 1;
    }
    if (!SMF.isEOF()) {
      if (SMF.getNextEvent());
    }
    else
      state = S_END;
    break;

  case S_END:   // done with this one
    DEBUGS("\nS_END");
    SMF.close();
    midiSilence();
    songNumber = 0;  //set number to 0 so statement play is false
    timeStart = millis();
    state = S_WAIT_BETWEEN;
    DEBUGS("\nWAIT_BETWEEN");
    break;

  case S_WAIT_BETWEEN:    // signal finish LED with a dignified pause
    if (millis() - timeStart >= WAIT_DELAY)
      state = S_IDLE;
    break;

  }
}


void rotaryMotor() {
    int reading = digitalRead(in);
   //  Serial.println(in);

   if ((millis() - lastStateChangeTime) > dialHasFinishedRotatingAfterMs) {
      // the dial isn't being dialed, or has just finished being dialed.
      if (needToPrint) {
      // if it's only just finished being dialed, we need to send the number down the serial
      // line and reset the count. We mod the count by 10 because '0' will send 10 pulses.
          songNumber = count;
          Serial.println(count, DEC);
          Serial.println("NU GAAT HET SPELEN BEGINNEN");
          needToPrint = 0;
          count = 0;
          cleared = 0;
       }
    }

    if (reading != lastState) {
        lastStateChangeTime = millis();
    }

  if ((millis() - lastStateChangeTime) > debounceDelay) {
  // debounce - this happens once it's stablized
      if (reading != trueState) {
       // this means that the switch has either just gone from closed->open or vice versa.
           trueState = reading;
            if (trueState == HIGH) {
             // increment the count of pulses if it's gone high.
                count++;
                needToPrint = 1; // we'll need to print this number (once the dial has finished rotating)
             }
       }
   }
   lastState = reading;
}





void midiCallback(midi_event *pev)
// Called by the MIDIFile library when a file event needs to be processed
// thru the midi communications interface.
// This callback is set up in the setup() function.
{
#if USE_MIDI
  if ((pev->data[0] >= 0x80) && (pev->data[0] <= 0xe0))
  {
    Serial.write(pev->data[0] | pev->channel);
    Serial.write(&pev->data[1], pev->size-1);
  }
  else
    Serial.write(pev->data, pev->size);
#endif
  DEBUG("\n");
  DEBUG(millis());
  DEBUG("\tM T");
  DEBUG(pev->track);
  DEBUG(":  Ch ");
  DEBUG(pev->channel+1);
  DEBUG(" Data ");
  for (uint8_t i=0; i<pev->size; i++)
  {
  DEBUGX(pev->data[i]);
    DEBUG(' ');
  }
}


// Called by the MIDIFile library when a system Exclusive (sysex) file event needs 
// to be processed through the midi communications interface. Most sysex events cannot 
// really be processed, so we just ignore it here.
// This callback is set up in the setup() function.
void sysexCallback(sysex_event *pev) {
  DEBUG("\nS T");
  DEBUG(pev->track);
  DEBUG(": Data ");
  for (uint8_t i=0; i<pev->size; i++)
  {
    DEBUGX(pev->data[i]);
    DEBUG(' ');
  }
}

// Turn everything off on every channel.
// Some midi files are badly behaved and leave notes hanging, so between songs turn
// off all the notes and sound
void midiSilence(void) {
  midi_event ev;
  // All sound off
  // When All Sound Off is received all oscillators will turn off, and their volume
  // envelopes are set to zero as soon as possible.
  ev.size = 0;
  ev.data[ev.size++] = 0xb0;
  ev.data[ev.size++] = 120;
  ev.data[ev.size++] = 0;

  for (ev.channel = 0; ev.channel < 16; ev.channel++)
    midiCallback(&ev);
}
