/*
Voice Visualizer for SoOnCon badge, cpu ATmega32U4 and LEDs WS2812   Bernie Rohde 2013 

To select this function press SW??  Microphone audio is amplified and DC-coupled to cpu 
analog input ADC0, PF0.  Serial data for LED colour is output from PC7.  

Think of the LED array as a scope x-axis (time/div)... colour is the y-axis, but note: 
unlike many visualizers, this program displays signal polarity, not amplitude or volume.  

Colours are your choice of any 2 combinations in the WS2812, or any colour on/off.  Each 
LED is a one-bit switch representing the polarity of one ADC sample.  The sampling rate 
is fast enough to measure instantaneous amplitude, so the array can show audio pitch.  

Colours keep switching when sound is present but they don't reset or disappear when it 
stops - the system keeps a snapshot of the last 8 readable samples.  If you get in tune 
with the sampling rate (??? Hz), it is possible to control the colour by voice.  

ADC0 will be at +2.5 to 3 VDC, depending on Vcc and circuit variations.  The program 
will find this as an average of ADC amplitude measurements.  It updates the variable 
'average' - the reference for discovering polarity of the next 1000 samples.  

Audio is an AC signal riding on the DC level.  A quiet voice, a foot from the mic, will 
typically give you 100 to 200 mV p-p.  Loud sounds will give up to 3.5 V p-p.  

Volume is predicted in the choice of value for 'margin'  - it's a noise margin (a fixed 
lower limit for valid signal strength).  This can also be used to compensate for 
variations in mic and amplifier gain.   

The cpu 10-bit ADC output range is 0 to 1023, roughly corresponding to input 0 to 5 V in 
5 mV steps.  So a reasonable value for 'margin' might be 20, ie 100 mV.  The program adds 
and subtracts 'margin' from 'average' ('margin' means peak, not p-p).  

The program takes one sample per LED pixel, tests it for amplitude and polarity, 
converts it to 8-bit G, R and B values (in that order), and stores those in an array 
of 24 bytes in RAM. The entire LED array is refreshed after every sample.  


this part is wrong

Timebase for pixle
Sampling period = 24 byte refresh + 1 analogRead(A0) + 3 bytes calculation: 

meas 122 uSec/frame for refresh 
meas 129 uSec/frame for refresh + calc
meas 144 uSec/frame for refresh + calc + analogRead using ADCSRA 196

For linearity along the x-axis, time used for calculation should be equal for every 
sample, for every signal amplitude including 0. 

PIXLE displays 1 cycle/frame of signals up to 2.9 KHz.  For N cycles/frame at lower 
frequencies, add (note)uSec delay to each line, F in Hz: (note) = (N*10^6/18F) - 19.2 
Assign a value to (note)or manually adjust DC input to analogRead(A1), once per frame.  
0 to +5V -> 10-bit ADC value 0 to 1023 -> uSec value for (note).  

*/

#include <SimpleTimer.h>

int sample;                             // ADC measurement, instantaneous amplitude 
int average;                            // average of previous ADC measurements 
long total = 0;                         // sum of current ADC measurements 
int avcount = 1000;                     // number of samples for averaging
int count;                              // loop counter for ADC measurements
byte margin = 40;                       // volume threshold is +/- 200 mVp
                                        // could be +/- ???  (noise is ??? mVp)

byte ecount = 0;                        // element counter for GRB array
byte channel;                           // 

uint32_t grbled [8] =                // G, R, B values for 8 LED's colour
{
  0,0,0,0,0,0,0,0
}; 

uint32_t t1 = 0, t2 = 0;

void TaskAudio_setup() 
{                                              // for ATmega32U4                                             
//  DDRC = B10000000;                            // PC7 set to output 
  //  DDRD = B00001111;                            // PORTD<3:0> set to output
  
  //  PORTB &= B11111000; 
  //cli();                                       // interrupt causes jitter
  //analogReference(EXTERNAL);
  pinMode(A0, INPUT);
  t2 = t1 = micros();
}

void TaskAudio_loop() 
{
   uint8_t pixel;
   
   for (count = 0; count < avcount; count++)    // get many samples for DC level calc
   { 
     // optional adjustment to put sampling rate into audio range (not for Nyquist) 
     //  PORTB |= B00000001;                          // scope HI
     // delayMicroseconds(2000);
     //  PORTB &= B11111110;                          // scope LO 

     sample = analogRead(A0);                       // signal amplitude 10-bit ADC value 
  
     total += sample;                         // to calculate input DC level
    
     if (sample < (average - margin))         // for signal polarity negative 
     {                                        // values are for example 
        grbled[ecount++] = 0x00FF00;
     }
     else if (sample > (average + margin))         // for signal polarity positive 
     {                                        // values are for example 
        grbled[ecount++] = 0x0000FF;
     }
  
     if (ecount == 8) ecount = 0;            // grbled is filled up, start over
     
     // refresh entire LED array after every sample ...
     // send first 3 bytes from grbled array 
     // reset pulse LO min 50 usec 
     // send next 3 bytes, etc 
     // don't change ecount - its value is unknown at this point 
   
     for (pixel = 0; pixel < 8; pixel++) {
       ledStrip.setPixelColor(pixel, grbled[pixel]);
     }

     // Update every 20ms
     t1 = micros();  
     if ((t1 - t2) > 20000) {
       t2 = t1;
       ledStrip.show();
       input();
     }
   
     //ledStrip.show();
   }   
   average = total / avcount;    // calc new DC level after 'avcount' samples
   total = 0;                    // reset for the next calculation
}


