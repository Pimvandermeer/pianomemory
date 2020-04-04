#include<Arduino.h>
#include <SdFat.h>
#include <MIDI.h>
#include <MD_MIDIFile.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN 6
#define NUM_LEDS 16
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

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

#define  SD_SELECT  4  // SD chip select pin for SPI comms. Arduino Ethernet shield, pin 4.

#define WAIT_DELAY    2000 // ms

MIDI_CREATE_DEFAULT_INSTANCE();

int playSongNumber = 0;
int listenToInput = 1;

//Used for faster  debugging songs plays songs for shorter time
long playTime = 0;
long stoppedPlayTime = 50;

const char *tuneList[] = {
  "ABBAb.mid",
  "ABBAb.mid",  // simplest and shortest file  
  "ABBAe.mid",
  "ALLb.mid",
  "ALLe.mid",
  "JINb.mid",
  "JINe.mid",
  "SILENTb.mid",
  "SILENTe.mid",
  "SOb.mid",
  "SOe.mid",
  "WEWISHb.mid",
  "WEWISHe.mid",
  "WHAMb.mid",
  "WHAMe.mid",
  "WHITEb.mid",
  "WHITEe.mid",  
};

SdFat SD;
MD_MIDIFile SMF;

class Button {
  public:
    boolean flagPress;
    boolean flagClick;
    boolean finished = 0;
    boolean first;
    boolean scnd;
    void scanState(); // method for checking the signal state
    void setButton(byte pin, byte sequence, char const* type, byte led, byte songNumber); // method of setting the pin number and time
    char _type;
    byte _led;
    byte _sequence;
    byte _songNumber;
    byte _pin;
  private:
};

Button button1, button2, button3, button4, button5, button6, button7, button8, button9, button10, button11, button12, button13, button14, button15, button16;

//for reference
Button first;
Button scnd;
Button firstReset;
Button scndReset;
Button shuffleTemp;

Button *btnsArr[] = {&button1, &button2, &button3, &button4, &button5, &button6, &button7, &button8, &button9, &button10, &button11, &button12, &button13, &button14, &button15, &button16}; ///array for destructering

const unsigned sizebtnsArr = (sizeof(btnsArr) / sizeof(btnsArr[1]));     //sizeof  voor de forloops
bool secondClick = 0;
bool checkStatus = 0;
int allFinished = 0;

byte gameMode = 0;

/////Led color combination;-------------------------------------------------------------------------------------
void setAll(byte red, byte green, byte blue) {
  for (int i = 0; i < NUM_LEDS; i++ ) {
    strip.setPixelColor(i, red, green, blue);
  }
  strip.show();
};

void beginStateLed (byte red, byte green, byte blue, int Count, int SpeedDelay, boolean OnlyOne) {
  setAll(0, 0, 0);
  for (int i = 0; i < Count; i++) {
    strip.setPixelColor(random(NUM_LEDS), random(0, 255), random(0, 255), random(0, 255));
    strip.show();
    delay(SpeedDelay);
    if (OnlyOne) {
      setAll(0, 0, 0);
    }
  }
  delay(SpeedDelay);
};

///MIDI LIBRARY FUNCtions-----------------------------------------------------------------------------
void midiCallback(midi_event *pev) {
#if USE_MIDI
  if ((pev->data[0] >= 0x80) && (pev->data[0] <= 0xe0))
  {
    Serial.write(pev->data[0] | pev->channel);
    Serial.write(&pev->data[1], pev->size - 1);
  }
  else
    Serial.write(pev->data, pev->size);
#endif
  DEBUG("\n");
  DEBUG(millis());
  DEBUG("\tM T");
  DEBUG(pev->track);
  DEBUG(":  Ch ");
  DEBUG(pev->channel + 1);
  DEBUG(" Data ");
  for (uint8_t i = 0; i < pev->size; i++)
  {
    DEBUGX(pev->data[i]);
    DEBUG(' ');
  }
};

void sysexCallback(sysex_event *pev) {
  DEBUG("\nS T");
  DEBUG(pev->track);
  DEBUG(": Data ");
  for (uint8_t i = 0; i < pev->size; i++)
  {
    DEBUGX(pev->data[i]);
    DEBUG(' ');
  }
};

void midiSilence(void) {
  midi_event ev;
  ev.size = 0;
  ev.data[ev.size++] = 0xb0;
  ev.data[ev.size++] = 120;
  ev.data[ev.size++] = 0;

  for (ev.channel = 0; ev.channel < 16; ev.channel++)
    midiCallback(&ev);
};

///PLAY MIDI FUNTION-------------------------------------------------------------------------------------
void playMidi() {

  static enum { S_IDLE, S_PLAYING, S_END, S_WAIT_BETWEEN } state = S_IDLE;
  static uint32_t timeStart;
  switch (state)  {
    case S_IDLE:    // now idle, set up the next tune
      {
        int err;
        DEBUGS("\nS_IDLE");
        DEBUG(tuneList[playSongNumber]);
        SMF.setFilename(tuneList[playSongNumber]);
        err = SMF.load();
        
        if (err != -1) {
          DEBUG(" - SMF load Error ");
          DEBUG(err);
          timeStart = millis();
          state = S_WAIT_BETWEEN;
          DEBUGS("\nWAIT_BETWEEN");
        }
        else {
          listenToInput = 0; 

          DEBUGS("\nS_PLAYING");
          state = S_PLAYING;
     //     playTime = millis();        
        }
      }
      break;

    case S_PLAYING: // play the file
      DEBUGS("\nS_PLAYING");
//          if ((millis() - playTime) > stoppedPlayTime) {
//            state = S_END;
//            Serial.println("Muziek gestopt");
//      
//          }
      if (!SMF.isEOF()) {
        if (SMF.getNextEvent());
          
      }
      else
        state = S_END;
      break;

    case S_END:   // done with this one
      DEBUGS("\nS_END");
      // midiSilence();
      SMF.close();
      midiSilence();
      listenToInput = 1;
      playSongNumber = 0;  //set number to 0 so statement play is false
      timeStart = millis();
      state = S_WAIT_BETWEEN;
      DEBUGS("\nWAIT_BETWEEN");
      break;

    case S_WAIT_BETWEEN:    // signal finish LED with a dignified pause
      if (millis() - timeStart >= WAIT_DELAY)
        state = S_IDLE;
      break;

    default:
      state = S_IDLE;
      break;
  }
};

void succesTune() {
  delay(300);
  MIDI.sendNoteOn(77, 127, 1);    
  delay(100);
  MIDI.sendNoteOff(77, 127, 1);
  MIDI.sendNoteOn(81, 127, 1);
  delay(100);
  MIDI.sendNoteOff(81, 127, 1); 
  MIDI.sendNoteOn(84, 127, 1);
  delay(100);
  MIDI.sendNoteOff(84, 127, 1); 
  MIDI.sendNoteOn(89, 127, 1);    
  delay(100);
  MIDI.sendNoteOff(89, 127, 1);
  delay(200);

  
  MIDI.sendNoteOn(84, 127, 1);
  delay(200);
  MIDI.sendNoteOff(84, 127, 1); 
  MIDI.sendNoteOn(89, 127, 1);    
  delay(600);
  MIDI.sendNoteOff(89, 127, 1);
};


void failureTune() {
  delay(300);
  MIDI.sendNoteOn(42, 127, 1);  
  delay(400);              
  MIDI.sendNoteOff(42, 0, 1);     
  MIDI.sendNoteOn(36, 127, 1);
  delay(1400);
  MIDI.sendNoteOff(36, 127, 1);
};

void play_memory() {
  for (unsigned int i = 0; i < sizebtnsArr; i++) {
    if (btnsArr[i]->flagClick == 1 && btnsArr[i]->finished == 0 && secondClick == 0) {    //listens if button is pressed and if it is not already finished
      btnsArr[i]->flagClick = 0;        //reset button state
      btnsArr[i]->first = 1;
      first = *btnsArr[i]; //controleren of dit kan;
      DEBUG("\n" + btnsArr[i]->_type);
      DEBUG("\nfirst");

      strip.setPixelColor(btnsArr[i]->_led, 255, 101, 0);      ///SET ORANGE COLOR VALUE
      strip.show();
      playSongNumber = btnsArr[i]->_songNumber + 1;
      secondClick = 1;
      listenToInput = 0;

    } else if
    (btnsArr[i]->flagClick == 1 && btnsArr[i]->finished == 0 && secondClick == 1 && btnsArr[i]->first != 1) {    //listens if button is pressed and if it is not already finished
      btnsArr[i]->flagClick = 0;        //reset button state
      btnsArr[i]->scnd = 1;
      DEBUG("\n" + btnsArr[i]->_type);
      DEBUG("\nsecond");
      scnd = *btnsArr[i]; //controleren of dit kan;
      strip.setPixelColor(btnsArr[i]->_led, 255, 101, 0);      ///SET ORANGE COLOR VALUE
      strip.show();
      playSongNumber = btnsArr[i]->_songNumber + 1;
      checkStatus = 1;
      listenToInput = 0;
    };
  };

  //Faillure maken
  if (checkStatus == 1 && first._type != scnd._type && listenToInput == 1) {
    //succes //set groene lichten disable buttonlistener
    for (unsigned int i = 0; i < sizebtnsArr; i++) {
      if (btnsArr[i]->first == 1) {    //listens if button is pressed and if it is not already finished
        DEBUG("\nZe matchen niet 1");
        btnsArr[i]->first = 0;
      };
      if (btnsArr[i]->scnd == 1) {    //listens if button is pressed and if it is not already finished
        strip.setPixelColor(first._led, 255, 0, 0);    ///SET RED COLOR VALUE
        strip.setPixelColor(scnd._led, 255, 0, 0);
        strip.show();
        failureTune();

        //reset zodat je weer verder kan  2 lege objecten maken
        DEBUG("\nZe matchen niet 2");
        strip.setPixelColor(first._led, 0, 0, 150);    ///SET BACK TO DEFAULT COLOR VALUE
        strip.setPixelColor(scnd._led, 0, 0, 150);
        strip.show();
        delay(250);
        //reset zodat je weer verder kan 2 lege objecten maken
        btnsArr[i]->scnd = 0;
        secondClick = 0;
        scnd = scndReset;
        first = firstReset;
        checkStatus = 0;
      };
    };
  };


  //Rigt combination but not right sequence
  if (checkStatus == 1 && first._type == scnd._type && (first._sequence - scnd._sequence == -1) && listenToInput == 1) {
    //succes //set groene lichten disable buttonlistener
    for (unsigned int i = 0; i < sizebtnsArr; i++) {
      if (btnsArr[i]->first == 1) {    //listens if button is pressed and if it is not already finished
        DEBUG("\nZe matchen niet ivm volgorde");
        btnsArr[i]->first = 0;
      };
      if (btnsArr[i]->scnd == 1) {    //listens if button is pressed and if it is not already finished
        strip.setPixelColor(first._led, 255, 0, 0);    ///SET RED COLOR VALUE
        strip.setPixelColor(scnd._led, 255, 0, 0);
        strip.show();

        failureTune();
        DEBUG("\nZe matchen niet ivm volgorde");
        strip.setPixelColor(first._led, 0, 0, 150);    ///SET BACK TO DEFAULT COLOR VALUE
        strip.setPixelColor(scnd._led, 0, 0, 150);
        strip.show();

        //reset zodat je weer verder kan  2 lege objecten maken
        btnsArr[i]->scnd = 0;
        secondClick = 0;
        first = firstReset;   //clear object to compare again
        scnd = scndReset;    //clear object to compare again
        checkStatus = 0;
      };
    };
  };

  //Succes and right sequence
  if (checkStatus == 1 && first._type == scnd._type && (first._sequence - scnd._sequence == 1) && listenToInput == 1) {
    //succes //set groene lichten disable buttonlistener
    for (unsigned int i = 0; i < sizebtnsArr; i++) {
      if (btnsArr[i]->first == 1) {    //listens if button is pressed and if it is not already finished
        btnsArr[i]->finished = 1;
        btnsArr[i]->first = 0;
        strip.setPixelColor(btnsArr[i]->_led, 0, 255, 0);      ///SET GREEN COLOR VALUE
        strip.show();
        DEBUG("\nZe matchen");
        
        //reset zodat je weer verder kan 2 lege objecten maken
        first = firstReset;  //clear object to compare again
      };
      if (btnsArr[i]->scnd == 1 ) {    //listens if button is pressed and if it is not already finished
        btnsArr[i]->finished = 1;     //reset button state
        btnsArr[i]->scnd = 0;
        strip.setPixelColor(btnsArr[i]->_led, 0, 255, 0);  ///SET GREEN COLOR VALUE
        strip.show();
        succesTune();
        
        //reset zodat je weer verder kan  2 lege objecten maken
        DEBUG("\nZe matchen");
        secondClick = 0;
        scnd = scndReset;   //clear object to compare again
        checkStatus = 0;
        allFinished = allFinished + 2;
      };
    };
  };
};

//FINISH FUNTION AND RESET----------------------------------------------------------------------------------
void playerWinsAndReset(byte red, byte green, byte blue, int StrobeCount, int FlashDelay, int EndPause) {
  //Serial.println("gewonnen");
  for (int j = 0; j < StrobeCount; j++) {
    setAll(red, green, blue);
    strip.show();
    delay(FlashDelay);
    setAll(0, 0, 0);
    strip.show();
    delay(FlashDelay);
  };
  delay(EndPause);
  setAll(0, 0, 0);
  strip.show();
  for (unsigned int i = 0; i < sizebtnsArr; i++) {
    btnsArr[i]->finished = 0;
    btnsArr[i]->first = 0;
    btnsArr[i]->scnd = 0;
  }
  gameMode = 0;
  allFinished = 0;
};

void shuffleButtons () {
  randomSeed(analogRead(A1));  //takes care of every time a different result
  for (size_t i = sizebtnsArr - 1; i > 0; i--) {
      long j = random() % (i + 1);

      shuffleTemp._songNumber = btnsArr[i]->_songNumber;
      shuffleTemp._type = btnsArr[i]->_type;
      shuffleTemp._sequence = btnsArr[i]->_sequence;
      
      btnsArr[i]->_songNumber = btnsArr[j]->_songNumber;
      btnsArr[i]->_type = btnsArr[j]->_type;
      btnsArr[i]->_sequence = btnsArr[j]->_sequence;
      
      DEBUG("\n I ");
      DEBUG(btnsArr[i]->_songNumber);
      DEBUG("\n J ");
          DEBUG(btnsArr[j]->_songNumber);
      DEBUG("\n j type");    
      DEBUG(j);
      
      btnsArr[j]->_songNumber = shuffleTemp._songNumber;
      btnsArr[j]->_type = shuffleTemp._type;
      btnsArr[j]->_sequence = shuffleTemp._sequence;

  }
};



/////CLASS BUTTON ----FUNCTIONS-----------------------------------------------------------------------------
void Button::scanState() {
  if (flagPress == (! digitalRead(_pin))) {
  } else {
    flagPress = !flagPress;
    ///hier de functie aanroepen om het nummer te gaan spelen.
    if (flagPress == true) flagClick = true;
  };
  delay(10);   ///debouncing
};

void Button::setButton (byte pin, byte sequence, char const* type, byte led, byte songNumber) {
  _pin = pin;
  _sequence = sequence;
  _type = type;
  _led = pin - 30;
  _songNumber = songNumber;
  pinMode(_pin, INPUT_PULLUP);
};




void setup() {
  Serial.begin(SERIAL_RATE);
  DEBUG("\n[MidiFile Play List]");

  // Initialize SD
  if (!SD.begin(SD_SELECT, SPI_FULL_SPEED))
  {
    DEBUG("\nSD init fail!");
    while (true) ;
  };

  // Initialize MIDIFile
  SMF.begin(&SD);
  SMF.looping(false);
  SMF.setMidiHandler(midiCallback);
  SMF.setSysexHandler(sysexCallback);

  //Initialize Leds
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  button1.setButton(30, 5, "A", 0, 0);
  button2.setButton(31, 4, "A", 0, 1);
  button3.setButton(32, 5, "B", 0, 2);
  button4.setButton(33, 4, "B", 0, 3);
  button5.setButton(34, 5, "C", 0, 4);
  button6.setButton(35, 4, "C", 0, 5);
  button7.setButton(36, 5, "D", 0, 6);
  button8.setButton(37, 4, "D", 0, 7);
  button9.setButton(38, 5, "E", 0, 8);
  button10.setButton(39, 4, "E", 0, 9);
  button11.setButton(40, 5, "F", 0, 10);
  button12.setButton(41, 4, "F", 0, 11);
  button13.setButton(42, 5, "G", 0, 12);
  button14.setButton(43, 4, "G", 0, 13);
  button15.setButton(44, 5, "H", 0, 14);
  button16.setButton(45, 4, "H", 0, 15);
}

void loop() {
  if (listenToInput == 1) {
    button1.scanState();
    button2.scanState();
    button3.scanState();
    button4.scanState();
    button5.scanState();
    button6.scanState();
    button7.scanState();
    button8.scanState();
    button9.scanState();
    button10.scanState();
    button11.scanState();
    button12.scanState();
    button13.scanState();
    button14.scanState();
    button15.scanState();
    button16.scanState();
  };

  if (!gameMode) {
    beginStateLed(0xff, 0, 0, 6, 100, false);
  };    ///twinkle begin state

  for (unsigned int i = 0; i < sizebtnsArr; i++) {
    if (btnsArr[i]->flagClick == 1 && gameMode == 0) {    //listens if button is pressed
      ///Start the game
      setAll(0, 0, 150);
      strip.show();
      shuffleButtons();
      btnsArr[i]->flagClick = 0;        //reset button state
      DEBUGS("\nStart the game");
      gameMode = 1;
    };
  };

  if (gameMode) {
    if (listenToInput) {
      play_memory();
    };
  };

  if (playSongNumber >= 1) {
    playMidi();
  }

  if (allFinished == 16 && listenToInput == 1) {     ///hoeveel combinaties mogelijk dan naar de winnaars modus
    playerWinsAndReset(0, 255, 0, 25, 50, 2000);
  };
};

