#ifdef ButtonsMulKeyboard

#define MUL_BUTTONS 16
#define BTN_MUL_S0_PIN 39
#define BTN_MUL_S1_PIN 40
#define BTN_MUL_S2_PIN 41
#define BTN_MUL_S3_PIN 42
#define BTN_MUL_SIG_PIN 36
#define BTN_MUL_EN_PIN 37
// modify constructor for I2C i/o
float volumeParam = 1.0f;
float semiModifier = 0.5f;
bool noNote = 0;
bool fnMode = 0;
unsigned long loopCount = 0;
unsigned long startTime = millis();
String msg = "";
//uint8_t  keyMod = 40; //the range of value that can be added to the input note number to determine played note
typedef enum { IDLE,
               PRESSED,
               HOLD,
               RELEASED } KeyState;
typedef struct {
  uint8_t id;
  uint8_t value;
  KeyState state;
  uint8_t note;
} Key;

Key keys[] = {
  { 0, HIGH, IDLE, 35 },
  { 1, HIGH, IDLE, 36 },
  { 2, HIGH, IDLE, 38 },
  { 3, HIGH, IDLE, 39 },
  { 4, HIGH, IDLE, 40 },
  { 5, HIGH, IDLE, 41 },
  { 6, HIGH, IDLE, 42 },
  { 7, HIGH, IDLE, 43 },
  { 8, HIGH, IDLE, 44 },
  { 9, HIGH, IDLE, 45 },
  { 10, HIGH, IDLE, 46 },
  { 11, HIGH, IDLE, 47 },
  { 12, HIGH, IDLE, 48 },
  { 13, HIGH, IDLE, 49 },
  { 14, HIGH, IDLE, 50 },
  { 15, HIGH, IDLE, 51 }
};
void setupKeyboard() {
  // pinMode(keys[0].pin, INPUT_PULLUP);
  // pinMode(keys[1].pin, INPUT_PULLUP);
  // pinMode(keys[2].pin, INPUT_PULLUP);
  ButtonsMul_Init();
}

// to receive changes from controller adc input see adc_module
void keyboardSetVolume(float value) {
  volumeParam = value;
}

float getKeyboardVolume() {
  return volumeParam;
}

void keyboardSetSemiModifier(float value) {
  semiModifier = value;
}
// for more functionality - we have all the keyboard keys to use - so we check the modifier bank button
void serviceKeyboardMatrix() {
  ButtonMul_Process();
}

void keyToNote(uint8_t keyUS, int i) {
  //keyUS += keyMod*semiModifier; //semiModifier is set in ADC from one of the pots to a 1.0f value and keyMod is whatever you want as adjust range
  switch (keys[i].state) {  // Report active keystate based on typedef enum{ IDLE, PRESSED, HOLD, RELEASED } KeyState;
    case PRESSED:
      //msg = " PRESSED.";
      if (checkBankValue() != 4)              //(!checkArpeggiator())
        Synth_NoteOn(0, keyUS, volumeParam);  //unchecked if type works as a note - was defaulted to 1.0f for velocity
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
      if (checkBankValue() != 4)  //(checkArpeggiator())
        Arp_NoteOff(keyUS);
      Synth_NoteOff(0, keyUS);
      break;
    case IDLE:  // there are times when idle needs to be calling noteOff because you had notes on hold in arpeggiator
                //msg = " IDLE.";
                //if(checkArpeggiator())
      Arp_NoteOff(keyUS);
  }
  // #ifdef DISPLAY_ST7735
  // miniScreenString(6,1,"N#:"+String(keyUS),HIGH);

  // #else
  Serial.printf("Key :/%d, %d\n", keyUS, i);  //+String(LIST_MAX));

  //#endif
}
//defunct
void keyToArpMap(uint8_t keyUS, int i) {
  //keyUS += keyMod*semiModifier; //semiModifier is set in ADC from one of the pots to a 1.0f value and keyMod is whatever you want as adjust range
  switch (keys[i].state) {  // Report active keystate based on typedef enum{ IDLE, PRESSED, HOLD, RELEASED } KeyState;
    case PRESSED:
      //msg = " PRESSED.";
      Arp_NoteOn(keyUS);  //unchecked if type works as a note - was defaulted to 1.0f for velocity
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

void keyToCommand(uint8_t keyCom) {  //assume called after the modifier button was held and a key was released

  switch (keyCom) {
    case 6:
      msg = "B0: vel ADSR";  //use like mousebutton up ... after release trigger
      setBank(0);
      break;
    case 7:
      msg = "B1: filter ADSR";  //use like mousebutton up ... after release trigger
      setBank(1);
      break;
    case 9:
      msg = "B2: filter main";  //use like mousebutton up ... after release trigger
      setBank(2);
      break;
    case 11:
      msg = "B3: delay";  //use like mousebutton up ... after release trigger
      setBank(3);
      break;
    case 13:
      msg = "B4: Arpeggiator";
      setBank(4);
      break;
    case 14:
      msg = "B5: Permormance";
      setBank(5);
      break;
  }

#ifdef DISPLAY_ST7735

  miniScreenString(7, 1, msg, HIGH);  //"ArpON+KbdCom:"+String(keyCom)

#else
  Serial.print("Command :/");  //
  Serial.print(keyCom);
  Serial.println(msg);
#endif
}

void ButtonsMul_Init(void) {
  Serial.print("Init");
  //memset(lastSendVal, 0xFF, sizeof(lastSendVal));

  pinMode(BTN_MUL_SIG_PIN, INPUT);
  pinMode(BTN_MUL_EN_PIN, OUTPUT);

  digitalWrite(BTN_MUL_EN_PIN, LOW);

  pinMode(BTN_MUL_S0_PIN, OUTPUT);
#if MUL_BUTTONS > 2
  pinMode(BTN_MUL_S1_PIN, OUTPUT);
#endif
#if MUL_BUTTONS > 4
  pinMode(BTN_MUL_S2_PIN, OUTPUT);
#endif
#if MUL_BUTTONS > 8
  pinMode(BTN_MUL_S3_PIN, OUTPUT);
#endif
}

void ButtonMul_Process(void) {
  for (int i = 0; i < MUL_BUTTONS; i++) {
    digitalWrite(BTN_MUL_S0_PIN, ((i & (1 << 0)) > 0) ? HIGH : LOW);
#if MUL_BUTTONS > 2
    digitalWrite(BTN_MUL_S1_PIN, ((i & (1 << 1)) > 0) ? HIGH : LOW);
#endif
#if MUL_BUTTONS > 4
    digitalWrite(BTN_MUL_S2_PIN, ((i & (1 << 2)) > 0) ? HIGH : LOW);
#endif
#if MUL_BUTTONS > 8
    digitalWrite(BTN_MUL_S3_PIN, ((i & (1 << 3)) > 0) ? HIGH : LOW);
#endif
    // digitalWrite(BTN_MUL_S0_PIN, LOW);
    // digitalWrite(BTN_MUL_S1_PIN, LOW);
    // digitalWrite(BTN_MUL_S2_PIN, LOW);
    // digitalWrite(BTN_MUL_S3_PIN, LOW);

    // give some time for transition
    delay(1);

    int8_t newValue = HIGH;
    if (keys[i].value != (newValue = digitalRead(BTN_MUL_SIG_PIN)))  // Only find keys that have changed state.
    {
      keys[i].value = newValue;

      keys[i].state = (newValue == LOW ? PRESSED : RELEASED);

      if (i == 0) {

        if (keys[i].state == PRESSED) {
          fnMode = true;
          //miniScreenString(1, 1, "CMD", HIGH);
        } else {
          fnMode = false;
          //miniScreenString(1, 1, "     ", HIGH);
        }
      } else {
        //miniScreenString(1, 2, "BTN: " + String(i), HIGH);
        if (fnMode && keys[i].state == PRESSED) {
          Serial.printf("CMD %d, %d changed: %d. State: %d\n", i, keys[i].note, newValue, keys[i].state);
          keyToCommand(i);
          keys[i].state = IDLE;
        } else {

          Serial.printf("Note %d, %d changed: %d. State: %d\n", i, keys[i].note, newValue, keys[i].state);
          keyToNote(keys[i].note, i);
          keys[i].state = IDLE;
        }
      }      


      noNote = 0;  // reset flag to enable notes - flag was set because of a modifier keyboard command
    }
  }
}
#endif