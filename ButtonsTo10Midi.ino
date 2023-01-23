#ifdef ButtonsKeyboard

// modify constructor for I2C i/o
float volumeParam = 1.0f;
float semiModifier = 0.5f;
bool noNote = 0;
typedef enum{ IDLE, PRESSED, HOLD, RELEASED } KeyState;
typedef struct  {
  uint8_t pin;
  uint8_t value;
  KeyState state;
  uint8_t note;
} Key;

Key keys[] = {{39, HIGH, IDLE, 27}, {40, HIGH, IDLE, 28}, {41,  HIGH, IDLE, 29}};
uint8_t keysCount = 3;
unsigned long loopCount = 0;
unsigned long startTime = millis();
String msg = "";
uint8_t  keyMod = 40; //the range of value that can be added to the input note number to determine played note


void setupKeyboard() {
  pinMode(keys[0].pin, INPUT_PULLUP);
  pinMode(keys[1].pin, INPUT_PULLUP);
  pinMode(keys[2].pin, INPUT_PULLUP);  
}

// to receive changes from controller adc input see adc_module
void keyboardSetVolume(float value)
{
  volumeParam = value;
}

float getKeyboardVolume(){
  return volumeParam;
}

void keyboardSetSemiModifier(float value)
{
  semiModifier = value;
}
// for more functionality - we have all the keyboard keys to use - so we check the modifier bank button
void serviceKeyboardMatrix() {
  //const int myLIST_MAX = LIST_MAX - 2; //42
  // Fills kpd.key[ ] array with up-to 10 active keys.
  // Returns true if there are ANY active keys.
 

    for (int i=0; i<keysCount; i++)   // Scan the whole key list.  LIST_MAX
    {
      int8_t newValue = HIGH;
      if ( keys[i].value != (newValue = digitalRead(keys[i].pin)))   // Only find keys that have changed state.
      {
         keys[i].value = newValue;

         keys[i].state = (newValue == LOW ? PRESSED : RELEASED);

          Serial.printf("Note %d, %d changed: %d. State: %d\n",i, keys[i].note,newValue, keys[i].state);
        // #ifdef USE_MODIFIER_KEYCOMMANDS
        // if(!commandState() && kpd.key[i].kstate == PRESSED){
        //   keyToCommand(uint8_t(kpd.key[i].kchar));
        //   //arpAllOff();  //test code when it wasn't working well
          
        // }
        // #endif
        // let it also do the keyboard notes playing effect as well as triggering command in previous call/check
         //when the arpeggiator mode is on don't play
        //if(noNote == 0)

        keyToNote(keys[i].note,i); 
        keys[i].state = IDLE;
        

      noNote = 0; // reset flag to enable notes - flag was set because of a modifier keyboard command
     }
   }
}



void keyToNote(uint8_t  keyUS, int i){
  keyUS += keyMod*semiModifier; //semiModifier is set in ADC from one of the pots to a 1.0f value and keyMod is whatever you want as adjust range
  switch (keys[i].state) {  // Report active keystate based on typedef enum{ IDLE, PRESSED, HOLD, RELEASED } KeyState;
      case PRESSED:
          //msg = " PRESSED.";
          if(checkBankValue() != 4)//(!checkArpeggiator())
            Synth_NoteOn(0, keyUS, volumeParam); //unchecked if type works as a note - was defaulted to 1.0f for velocity
          else
            Arp_NoteOn(keyUS);
          break;
      case HOLD:
          //msg = " HOLD.";
         // if(checkArpeggiator()) // this idea - was too slow so I moved the Arp_NoteOn back up to PRESSED status
           // Arp_NoteOn(keyUS);
          break;
      case RELEASED:
          //msg = " RELEASED.";
          if(checkBankValue() != 4)//(checkArpeggiator())
            Arp_NoteOff(keyUS);
          Synth_NoteOff(0, keyUS);
          break;
      case IDLE:    // there are times when idle needs to be calling noteOff because you had notes on hold in arpeggiator
          //msg = " IDLE.";
          //if(checkArpeggiator())
            Arp_NoteOff(keyUS);
  }
  // #ifdef DISPLAY_ST7735
  // miniScreenString(6,1,"N#:"+String(keyUS),HIGH);
  
  // #else
  Serial.printf("Key :/%d, %d\n", keyUS, i);//+String(LIST_MAX));

  //#endif
}
//defunct
void keyToArpMap(uint8_t  keyUS, int i){
  keyUS += keyMod*semiModifier; //semiModifier is set in ADC from one of the pots to a 1.0f value and keyMod is whatever you want as adjust range
  switch (keys[i].state) {  // Report active keystate based on typedef enum{ IDLE, PRESSED, HOLD, RELEASED } KeyState;
      case PRESSED:
          //msg = " PRESSED.";
          Arp_NoteOn(keyUS); //unchecked if type works as a note - was defaulted to 1.0f for velocity
          break;
      case HOLD:
          //msg = " HOLD.";
          break;
      case RELEASED:
          //msg = " RELEASED.";
          Arp_NoteOff(keyUS);
          break;
      case IDLE:
          msg = " IDLE.";
  }
}

void keyToCommand(uint8_t  keyCom){ //assume called after the modifier button was held and a key was released
  
  switch (keyCom) { 
      case 20:
          msg = "B0: vel ADSR"; //use like mousebutton up ... after release trigger
          setBank(0);
          break;
      case 21:
          msg = "B1: filter ADSR"; //use like mousebutton up ... after release trigger
          setBank(1);
          break;
      case 22:
          msg = "B2: filter main"; //use like mousebutton up ... after release trigger
          setBank(2);
          break;
      case 23:
          msg = "B3: delay"; //use like mousebutton up ... after release trigger
          setBank(3);
          break;
      case 24:
          msg = "B4: Arpeggiator";
          setBank(4);
          break;
      case 25:
          msg = "B5: Permormance";
          setBank(5);
          break;
      case 56:
          if(!checkArpHold()){
            arpAllOff();  //i put this here because
            msg = "All Arp Notes off";
          } else {
            delTailSeq();
            msg = "RemoveTail Note";
          }
          break;
  }
  noNote = 1;
  #ifdef DISPLAY_ST7735

  miniScreenString(7,1,msg,HIGH);  //"ArpON+KbdCom:"+String(keyCom)
  
  #else
  Serial.print("Command :/");//
  Serial.print(keyCom);
  Serial.println(msg);
  #endif
}

#endif