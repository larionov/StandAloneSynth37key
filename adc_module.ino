/*
 * This module is run adc with a multiplexer
 * tested with ESP32 Audio Kit V2.2
 * Only tested with 8 inputs
 *
 * Define your adc mapping in the lookup table
 *
 * Author: Marcel Licence
 *
 * Reference: https://youtu.be/l8GrNxElRkc
 */

/* Notes by Michael - hack version of this 
 * Requires Boardmanager 1.0.4 or earlier of ESP32
 * doing math
*/

// from easySynth module code definitions
#define SYNTH_PARAM_VEL_ENV_ATTACK  0
#define SYNTH_PARAM_VEL_ENV_DECAY 1
#define SYNTH_PARAM_VEL_ENV_SUSTAIN 2
#define SYNTH_PARAM_VEL_ENV_RELEASE 3
#define SYNTH_PARAM_FIL_ENV_ATTACK  4
#define SYNTH_PARAM_FIL_ENV_DECAY 5
#define SYNTH_PARAM_FIL_ENV_SUSTAIN 6
#define SYNTH_PARAM_FIL_ENV_RELEASE 7
#ifdef USE_UNISON
#define SYNTH_PARAM_DETUNE_1    8
#define SYNTH_PARAM_UNISON_2    9
#else
#define SYNTH_PARAM_WAVEFORM_1    8
#define SYNTH_PARAM_WAVEFORM_2    9
#endif
#define SYNTH_PARAM_MAIN_FILT_CUTOFF  10
#define SYNTH_PARAM_MAIN_FILT_RESO    11
#define SYNTH_PARAM_VOICE_FILT_RESO   12
#define SYNTH_PARAM_VOICE_NOISE_LEVEL 13
#define WAVEFORM_TYPE_COUNT  7

//new customer parameters not handled by modules as forked
#define CONTROL_PARAM_MAX_VOL  14
//You can also call the simple_delay module 
// eg: void Delay_SetLength(uint8_t unused, float value)

/* FRom here down these parameters are created for this fork as I expand functionality above basics to enjoy 
 *  the synth
 */


//  in z_config midi-controller parameters the function calls are encoded as Delay_SetLength, 2   -Delay_SetLevel, 3    -Delay_SetFeedback, 4
#define CONTROL_DELAY_SET_LENGTH 15
#define CONTROL_DELAY_SET_LEVEL 16
#define CONTROL_DELAY_SET_FEEDBACK 17
#define CONTROL_SEMITONES 18  // keyboard note modifier
// Arpeggiator parameters  
#define ARP_STATE 20
#define ARP_VARIATION 21
#define ARP_HOLD 22
#define ARP_NOTE_LEN 23
#define ARP_BPM 24
// Performance Parameters
#define PERF_SEMITONES 25   //master semitone tuning
#define PERF_SWING 26       //adjustment for beat spacing to create swing effect (triple skip like)
#define PERF_LFO_PARAM 27   //assign a parameter to sweep with LFO  (low freq oscillator is (usually between .1 and 250Hz) 
#define PERF_LFO_PERIOD 28  //Frequency of LFO wave
#define PERF_BEAT_SKIP 29   //put rests in some pattern into arpeggion - set of variations?


#ifdef extraButtons
#define bankButton 4 // for use with a single POT to select which parameter
#define downButton 4 // ditto
#else
#define bankButton LED_PIN // if not in use just define something else and keep pins free
#define downButton LED_PIN // ditto
#endif

#define NUMDIRECTPOTS   5  // 4 potentiometers wired to pings by position TL 35, TR 34, BL 39, BR 36
#define NUMBANKS        5
uint8_t adcSimplePins[NUMDIRECTPOTS] = { ADC_DIRECT_TL, ADC_DIRECT_TR, ADC_DIRECT_BL, ADC_DIRECT_BR, ADC_FADER_ONE};// pot pins defined in config.h by location 
                        // extraButtons (above) used to change parameter type related to this pot
                     
uint8_t potBank[NUMBANKS][NUMDIRECTPOTS] = { {SYNTH_PARAM_VEL_ENV_ATTACK,SYNTH_PARAM_VEL_ENV_DECAY,
                                        SYNTH_PARAM_VEL_ENV_SUSTAIN,SYNTH_PARAM_VEL_ENV_RELEASE,SYNTH_PARAM_WAVEFORM_1},  //end bank 0 Vel envelope
                                        {SYNTH_PARAM_FIL_ENV_ATTACK,SYNTH_PARAM_FIL_ENV_DECAY,
                                         SYNTH_PARAM_FIL_ENV_SUSTAIN,SYNTH_PARAM_FIL_ENV_RELEASE, SYNTH_PARAM_WAVEFORM_2 }, //end bank 1 Filter env
                                         {SYNTH_PARAM_MAIN_FILT_RESO,SYNTH_PARAM_MAIN_FILT_CUTOFF,
                                        SYNTH_PARAM_VOICE_FILT_RESO ,SYNTH_PARAM_VOICE_NOISE_LEVEL,CONTROL_PARAM_MAX_VOL}, //end bank 2  Res Cut Noise Vol
                                        {CONTROL_DELAY_SET_LENGTH,CONTROL_DELAY_SET_LEVEL,
                                         CONTROL_DELAY_SET_FEEDBACK,SYNTH_PARAM_MAIN_FILT_CUTOFF, CONTROL_SEMITONES },//end bank 3  Delay and Pitch
                                         {ARP_STATE,ARP_VARIATION,ARP_HOLD,ARP_NOTE_LEN, ARP_BPM }}; //end bank 4
                                         



bool bankButtonState, downButtonState, lastBankButtonState, lastDownButtonState;
uint8_t  bankValue;
unsigned long lastUBDebounceTime,lastDBDebounceTime;
unsigned long debounceDelay = 50; 


struct adc_to_midi_s
{
    uint8_t ch;
    uint8_t cc;
};
extern int  analogueParamSet = 0;
extern int  waveformParamSet = 0;
const int maxParameterVal = 13; //the highest numbered synthparameter to avoid sending unhandled parameter
unsigned int adcSingleMin[NUMDIRECTPOTS],adcSingleMax[NUMDIRECTPOTS];
float adcSingle[NUMDIRECTPOTS],adcSingleAve[NUMDIRECTPOTS]; // single pins of ESP32 dedicated to pots ...
float adcSetpoint[NUMDIRECTPOTS];
extern struct adc_to_midi_s adcToMidiLookUp[]; /* definition in z_config.ino */

uint8_t lastSendVal[ADC_TO_MIDI_LOOKUP_SIZE];  /* define ADC_TO_MIDI_LOOKUP_SIZE in top level file */

//#define ADC_INVERT
//#define ADC_THRESHOLD       (1.0f/200.0f)
//#define ADC_OVERSAMPLING    2048

//#define ADC_DYNAMIC_RANGE
//#define ADC_DEBUG_CHANNEL0_DATA

static float adcChannelValue[NUMDIRECTPOTS];


float *AdcMul_GetValues(void)
{
    return adcChannelValue;
}

void readSimplePots(){
  for(int i=0; i<NUMDIRECTPOTS; i++)
    adcSimple(i);

}
                                                //void setTextColor(uint16_t color);
                                                //void setTextColor(uint16_t color, uint16_t backgroundcolor);

uint8_t checkBankValue(){  //this is now called by the blink module because it does some tempo related graphics for arpeggiator bank
  return bankValue;
}

// the text for what parameter settings are mapped to the pots and slider
// unlike the basicSynth version of the project this idea of banks of functions for the onboard pots/slider is managed here
// whereas in the other project I over-rode the easysynth module's serial debug output to write to zones of a bigger screen
// I'm writing here because I got confused about that 
// Originally I uses the bank button to rotate through but as I developed more functionality I made the bankButton a
// modifier key - then keys on the keyboard can bring on banks
void screenLabelPotBank(){

  uint8_t color;
  if(bankValue < 4 && !checkArpHold()){

    useArpToggle(LOW);

    //arpAllOff();

  }
    

  switch(bankValue){
    case 0:


       color = 1;
       miniScreenString(0,color,"Attack",HIGH);
       miniScreenString(1,color,"Decay",HIGH);
       miniScreenString(2,color,"Sustain",HIGH);
       miniScreenString(3,color,"Release",HIGH);
       miniScreenString(5,color,"Waveform1>",HIGH);
       break;
    case 1:

       color = 1;
       miniScreenString(0,color,"F.Attack",HIGH);
       miniScreenString(1,color,"F.Decay",HIGH);
       miniScreenString(2,color,"F.Sust",HIGH);
       miniScreenString(3,color,"F.Rels",HIGH);
       miniScreenString(5,color,"Waveform2>",HIGH);
       break;
    case 2:

       color = 1;
       miniScreenString(0,color,"M.Reson",HIGH);
       miniScreenString(1,color,"M.Cutoff",HIGH);
       miniScreenString(2,color,"Voice-Res",HIGH);
       miniScreenString(3,color,"Noise-Lev",HIGH);
       miniScreenString(5,color,"Max-Volume>",HIGH);
       break;
     case 3:

       color = 1;
       miniScreenString(0,color,"Delay-Len",HIGH);
       miniScreenString(1,color,"Del-Level",HIGH);
       miniScreenString(2,color,"D.Feedback",HIGH);
       miniScreenString(3,color,"M.Cutoff",HIGH);
       miniScreenString(5,color,"Semitones",HIGH);
       break;
     case 4:

       miniScreenString(0,color,"Arpeg-On!",HIGH);
       miniScreenString(1,color,"Variation",HIGH);
       if(checkArpHold())         //previous wrote miniScreenString(2,color,"HOLD",HIGH); but switching back to this bank should indicate state on/off
         setArpHold(0.6);     //above 0.5 is high
       else
         setArpHold(0.4);     //below 0.5 is low - it defaults to this during it's initialization so this would be first message                        
       miniScreenString(3,color,"NoteLength",HIGH);
       miniScreenString(5,color,"BPM",HIGH);
       useArpToggle(HIGH);
       break;
  }
} 

//this is analog to digital conversion reading the (at the time of Aug 27) 4 pots we have being sampled and sending the 
//paramater change adjustements according to parameter value assigned by number potBank[bankValue][potNum] defined above
void  adcSimple(uint8_t potNum){
    unsigned int pinValue = 0;  //long int?  adcSingleMin, adcSingleMax to be same type
    float delta, error;
    bool midiMsg = false;
    const int oversample = 20;
    
    //Serial.printf("6 1 %d\n", potNum);
    //read the pin multiple times
    for(int i=0;i<oversample;i++)      pinValue += analogRead(adcSimplePins[potNum]);         
      pinValue = pinValue / oversample;  //if wiring bug value inverted 4096-(pinValue / oversample); 
    if(adcSingleMin[potNum] > pinValue) adcSingleMin[potNum] = (pinValue+adcSingleMin[potNum])/2;
    if(adcSingleMax[potNum] < pinValue) adcSingleMax[potNum] = pinValue;

    adcSingle[potNum] = float(pinValue)/4096.0f; //(pinValue/10)*10

    //Serial.printf("6 2 %d\n", potNum);
    //This "noise reduction in the pot value so its not constantly changing was derived experimentally and needs more analysis
    
    delta = adcSingleAve[potNum] - adcSingle[potNum]; //floating point absolute get rid of signed
    error = 0.009f+(0.012f*(adcSingle[potNum]+0.1));  //previous weird idea error = 0.03*((adcSingle+0.25)*0.75); 
    
    //Serial.printf("6 3 %d\n", potNum);
    //the idea is that if the delta adjustment is larger than the error we pick up significant control reading changes
    //instead of just sending minute voltage reading changes that could be noise and cause un-needed calls to code too often
    if (fabs(delta) > error ){
       if(adcSetpoint[potNum] != adcSingleAve[potNum]) //was there a change - if so then do stuff
        {
          adcSetpoint[potNum] = adcSingleAve[potNum];

          Serial.print(" Param: "+String(potNum));
          adcChannelValue[potNum] = adcSetpoint[potNum];
          //is the parameter setting for the bank within the range of parameter numbers handled by Synth_SetParam 0-13 #defined paramaters
          if(potBank[bankValue][potNum] < 14)
             Synth_SetParam(potBank[bankValue][potNum], adcChannelValue[potNum]);   //see easySynth module
          else
             Custom_SetParam(potBank[bankValue][potNum], adcChannelValue[potNum]);
          if(potBank[bankValue][potNum] == SYNTH_PARAM_WAVEFORM_1) //write name of waveform
            waveFormTextUpdate(1, uint8_t((adcChannelValue[potNum]) * (WAVEFORM_TYPE_COUNT)),5);
          if(potBank[bankValue][potNum] == SYNTH_PARAM_WAVEFORM_2) //write name of waveform
            waveFormTextUpdate(2, uint8_t((adcChannelValue[potNum]) * (WAVEFORM_TYPE_COUNT)),5);
              
          if(potNum<4)
            miniScreenBarSize(potNum, adcChannelValue[potNum]); //display a bar in the text area to show the current value
          else  //i added this because 4 was used for NoteNumber and the slider is closer to right side of screen so pot 4 is zone 5
            miniScreenBarSize(5,adcChannelValue[potNum]);
          midiMsg = true;
          
        } 
    
    }

    //Serial.printf("6 4 %d\n", potNum);
    adcSingleAve[potNum] = (adcSingleAve[potNum]+adcSingle[potNum])/2;
    
/*
    if (midiMsg)
    {
        uint32_t midiValueU7 = (adcChannelValue[analogueParamSet] * 127.999);
        if (analogueParamSet < ADC_TO_MIDI_LOOKUP_SIZE)
        {
            #ifdef ADC_INVERT
            uint8_t idx = (ADC_INPUTS - 1) -analogueParamSet;
            #else
            uint8_t idx = analogueParamSet;waveFormTextUpdate
            #endif
            if (lastSendVal[idx] != midiValueU7)
            {
                Midi_ControlChange(adcToMidiLookUp[idx].ch, adcToMidiLookUp[idx].cc, midiValueU7);
                lastSendVal[idx] = midiValueU7;
            }
        }
    } */
}

void Custom_SetParam(uint8_t slider, float value)
{

  switch(slider){
    case CONTROL_PARAM_MAX_VOL:
      keyboardSetVolume(value);  //see multikeyTo37Midi module where keyboard entry calls notes on/off
      break;
    case CONTROL_DELAY_SET_LENGTH:
      Delay_SetLength(0, value); //see simple_delay module
      break;
    case CONTROL_DELAY_SET_LEVEL:
      Delay_SetLevel(0, value);  //see simple_delay module
      break;
    case CONTROL_DELAY_SET_FEEDBACK:
      Delay_SetFeedback(0, value);  //see simple_delay module
      break;
    case CONTROL_SEMITONES:
      keyboardSetSemiModifier(value);
      break;
    case ARP_STATE:
      setArpState(value);  //to be coded
      break;
    case ARP_VARIATION:
      setArpVariation(value); //to be coded
      break;
    case ARP_HOLD:
      setArpHold(value);  //to be coded
      break;
    case ARP_NOTE_LEN:
      setArpNoteLength(value);  //to be coded
      break;
    case ARP_BPM:
      setBPM(value);    //set beats per minute
      miniScreenString(6,1,String(checkBPM()),HIGH);
      break;
  }
}

void setupButtons(){
  #ifdef extraButtons
  pinMode(bankButton, INPUT_PULLUP);  //pinMode(2, INPUT_PULLUP);
 // pinMode(downButton, INPUT_PULLUP);  //this is from the prototype simple synth - currently unused
  pinMode(LED_PIN, OUTPUT);  //use for mode toggle for pot params

  miniScreenString(0,1,"Button_in"+String(bankButton),HIGH);
  #endif
  //pinMode(adcSimplePin, INPUT);
  bankValue = 0;
  screenLabelPotBank();
}

void setupADC_MINMAX(){
  for(int i=0; i<NUMDIRECTPOTS; i++){
    adcSingleMin[i] = 0000;
    adcSingleMax[i] = 4095;
  }
}
void rotateBank(){
  bankValue = (bankValue+1)% NUMBANKS;
   
  screenLabelPotBank();
  miniScreenString(4,1,"Bank: "+ String(bankValue),HIGH);
}

void setBank(int bank){ //wrote this to be called from a modifier key plus keyboard keys
  bankValue = bank;
  screenLabelPotBank();
  miniScreenString(4,1,"Bank: "+ String(bankValue),HIGH);
}

void toggleBankButton(){
   rotateBank();
   
}

bool commandState(){
  return bankButtonState;
}

void waveFormTextUpdate(uint8_t waveChannel, uint8_t selWaveForm, int potBank){ //two wave channels 0, 1 and 
  switch(selWaveForm){
  /*reference: "easySynth.ino module"
    waveFormLookUp[0] = sine;
    waveFormLookUp[1] = saw;
    waveFormLookUp[2] = square;
    waveFormLookUp[3] = pulse;
    waveFormLookUp[4] = tri;
    waveFormLookUp[5] = crappy_noise;
    waveFormLookUp[6] = silence;*/
    case 0: 
       msg="sine";
       break;
    case 1: 
       msg="saw";
       break;
    case 2: 
       msg="square";
       break;
    case 3: 
       msg="pulse";
       break;
    case 4: 
       msg="triangle";
       break;
    case 5: 
       msg="crapnoise";
       break;
    case 6: 
       msg="sil-usr";
       break;
  }
   miniScreenString(potBank,1,String(waveChannel)+":"+msg,HIGH);
}
// added by Michael to this library which is really for processing knobs and buttons - this is buttons
// Before Aug 27 2020 there was only one button for the alone synth for changing bank for the 4 pots
void processButtons(){

  // read the state of the switch into a local variable:
  int readBankButton = digitalRead(bankButton);
  //int readDownButton = digitalRead(downButton);  //unused
  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH), and you've waited long enough
  // since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (readBankButton != lastBankButtonState) {
    // reset the debouncing timer
    lastUBDebounceTime = millis();
  }
  /*
   if (readDownButton != lastDownButtonState) {
    // reset the debouncing timer
    lastDBDebounceTime = millis();
  }*/
  //process the bank button
  if ((millis() - lastUBDebounceTime) > debounceDelay) {  

    // if the button state has changed:
    if (readBankButton != bankButtonState) {
      bankButtonState = readBankButton;
      
    #ifdef USE_MODIFIER_KEYCOMMANDS
    digitalWrite(LED_PIN, !bankButtonState);
    #else //rotate banks
      // only toggle the LED if the new button state is HIGH
      if (bankButtonState == LOW) {
         toggleBankButton();
         waveformParamSet = waveformParamSet + 1; //analogueParamSet++;
         
      }
    #endif
    }
  }/*
  if ((millis() - lastDBDebounceTime) > debounceDelay) {
     
    // if the button state has changed:
    if (0){//(readDownButton != downButtonState) {
      downButtonState = readDownButton;

      // only toggle the LED if the new button state is HIGH
      if (downButtonState == HIGH) {
         analogueParamSet = analogueParamSet - 1;
         if (analogueParamSet < 0) analogueParamSet=7;
         if (analogueParamSet > 7) analogueParamSet=0;
         Serial.print("ParamSet: ");

      }
    }
  } */
    // save the reading. Next time through the loop, it'll be the lastButtonState:
  lastBankButtonState = readBankButton;
  //lastDownButtonState = readDownButton;

}
 
