/*
 * pinout of ESP32 DevKit found here:
 * https://circuits4you.com/2018/12/31/esp32-devkit-esp32-wroom-gpio-pinout/
 *
 * Author: Marcel Licence
 */

#include "config.h"

/*
 * required include files
 */
#include <Arduino.h>
#include <WiFi.h>

/* this is used to add a task to core 0 */
TaskHandle_t  Core0TaskHnd ;
boolean       USBConnected;
uint16_t      task0cycles;
uint32_t      beatcycles,oneSec,noteCycles;

void setup()
{
    /*
     * this code runs once
     */  
     Serial.begin(115200);

    Delay_Init();
    
#ifdef DISPLAY_ST7735
    setupst7735();
#endif
    Serial.printf("Initialize Synth Module\n");
    Synth_Init();
    Serial.printf("Initialize I2S Module\n");

    // setup_reverb();
    USBConnected = LOW;
    //Blink_Setup();
    
#ifdef ESP32_AUDIO_KIT
    ac101_setup();
    Serial.printf("Use AC101 features of A1S variant");
#endif

    //setupKeyboard();
    setup_i2s();
    Serial.printf("Initialize Midi Module\n");
    Midi_Setup();

    Serial.printf("Turn off Wifi/Bluetooth\n");
#if 0
    setup_wifi();
#else
    WiFi.mode(WIFI_OFF);
#endif

#ifndef ESP8266
    btStop();
    // esp_wifi_deinit();
#endif

#ifdef MIDI_VIA_USB_ENABLED
    UsbMidi_Setup();
#endif
    arpeggiatorSetup();
    beatcycles = calcWaitPerBeat();
    noteCycles = noteLengthCycles(); // quarter note default
    oneSec = SAMPLE_RATE;

    Serial.printf("ESP.getFreeHeap() %d\n", ESP.getFreeHeap());
    Serial.printf("ESP.getMinFreeHeap() %d\n", ESP.getMinFreeHeap());
    Serial.printf("ESP.getHeapSize() %d\n", ESP.getHeapSize());
    Serial.printf("ESP.getMaxAllocHeap() %d\n", ESP.getMaxAllocHeap());

    Serial.printf("Firmware started successfully\n");


/* activate this line to get a tone on startup to test the DAC */
    //Synth_NoteOn(0, 64, 1.0f);

#if (defined ADC_TO_MIDI_ENABLED) || (defined MIDI_VIA_USB_ENABLED)
    xTaskCreatePinnedToCore(Core0Task, "Core0Task", 8000, NULL, 999, &Core0TaskHnd, 0);
    Serial.println("Setup Pin To Core 0 loop");
#endif

}


void Core0TaskSetup()
{
    /*
     * init your stuff for core0 here
     */
   setupKeyboard();
   //setupADC_MINMAX();
   setupButtons();
   Serial.println("Simple Analog");
   
   

   task0cycles = 0;
  
#ifdef ADC_TO_MIDI_ENABLED
    //AdcMul_Init();
#endif
}

void Core0TaskLoop()
{
  //Serial.println('loop');
    /*
     * put your loop stuff for core0 here
     */
  #ifdef ADC_TO_MIDI_ENABLED              
    //AdcMul_Process();
  #endif
   readSimplePots();
  
   processButtons();
   
   serviceKeyboardMatrix();

  #ifdef DISPLAY_ST7735
    displayRefresh();
  #endif
}

void Core0Task(void *parameter)
{
    
    Core0TaskSetup();
    Serial.print("Core0 Setup- confirm core:");
    Serial.println(xPortGetCoreID());
    while (true)
    {
        Core0TaskLoop();
       

        delay(1);
        yield();
    }
}

/*
 * use this if something should happen every second
 * - you can drive a blinking LED for example
 */


inline void pulseTempo(void)  // push the tempo - push the tempo
{
   Blink_Process(); //hijacked this to show a kind of bpm and led flash at tempo
}

inline void pulseNote(void)
{
  Arpeggiator_Process();
}

/*
 * our main loop
 * - all is done in a blocking context
 * - do not block the loop otherwise you will get problems with your audio
 */
float fl_sample, fr_sample;

void loop()
{

    static uint32_t loop_cnt_1beat;
    static uint32_t loop_cnt_1note;
    static uint8_t loop_count_u8 = 0;

    loop_count_u8++;

    loop_cnt_1beat++;  // related to tempo or bpm
    loop_cnt_1note++;  // related to divisions per beat or notes triggered by arpeggiator module
    
    //Serial.printf("loop_count_u8: %d, loop_cnt_1beat: %d, loop_cnt_1note: %d\n", loop_count_u8, loop_cnt_1beat, loop_cnt_1note);
    if (loop_cnt_1beat >= beatcycles) // trigger beat - originally for 1hz blink process - now blink light when arpeggiator is on at tempo
    {

        loop_cnt_1beat = 0;
        beatcycles = calcWaitPerBeat();
        noteCycles = noteLengthCycles(); 
        if(checkArpeggiator())
          pulseTempo();
    }
    if (loop_cnt_1beat >= oneSec){ // optional check in case a very slow tempo makes the next beat take a long time - recheck per second
       beatcycles = calcWaitPerBeat();
       noteCycles = noteLengthCycles(); 
    }

    if (loop_cnt_1note >= noteCycles)
    {
      loop_cnt_1note = 0;
       noteCycles = noteLengthCycles(); 
       if(checkArpeggiator())
          pulseNote();
    }
    
#ifdef I2S_NODAC
    if (writeDAC(fl_sample))
    {
        fl_sample = Synth_Process();
    }
#else

    if (i2s_write_stereo_samples(&fl_sample, &fr_sample))
    {
      //Serial.printf("i2s_Write %f, %f\n", fl_sample, fr_sample);
        /* nothing for here */
    }
    Synth_Process(&fl_sample, &fr_sample);
    /*
     * process delay line
     */
    Delay_Process(&fl_sample, &fr_sample);

#endif

    /*
     * Midi does not required to be checked after every processed sample
     * - we divide our operation by 8
     */
    if (loop_count_u8 % 24 == 0)
    {
       
        //if(!USBConnected) {UsbMidi_Loop(); Serial.print(".");}
       //Midi_Process();
      #ifdef MIDI_VIA_USB_ENABLED
        UsbMidi_ProcessSync();
      #endif
        
       

        
    }
    if (loop_count_u8 % 100 == 0)
    {
        #ifdef MIDI_VIA_USB_ENABLED
       UsbMidi_Loop();
       #endif
       #ifdef DISPLAY_ST7735
         //testanimate(logo_bmp, LOGO_WIDTH, LOGO_HEIGHT);
       #endif
    } 

}

    
