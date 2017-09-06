/* main.cpp -- application
   This file is part of the Motz PixelTone project:
   https://github.iu.edu/PBS-TSG/Motz-PixelTone

   This is C++ code intended to run on a Teensy LC microcontroller,
   using the Arduino framework, with build/upload handled by PlatformIO.

   Hardware connections:

   (2) photodiodes, biased into photoconductive mode
   (2) touch-sensitive pins, used to generate "fake" photodiode signals
   (1) stereo output jack
   (1) 2-digit display for the volume setting

   What it does:

   When not playing a tone, scan the inputs and update internal state.
   Based on the internal state, play a waveform, or react to user inputs if not playing a waveform.
*/
#include "waveforms.hpp" /* provides noise waveforms and attenuation ratios */
#include "ADC.h"
#include "Aswitch.hpp"
#include "RotaryEncoder.hpp"

ADC *adc=new ADC();

/* pins */
//Aswitch BUTTON_PNK{3,touchRead},BUTTON_WHT{4,touchRead};/* capacitive pushbutton input up/down */
Aswitch PD_PNK{A0},PD_WHT{A1};/* photodiode corresponding to white/pink noise trigger */
const uint8_t MONITOR_PNK{5};/* scope monitor output */
const uint8_t MONITOR_WHT{6};/* scope monitor output */
const uint8_t VREF_PIN{A8};/* used to provide voltage reference to audio amplifier */
RotaryEncoder ROTARY_ENCODER{11,12};/* volume knob */

/* I/O throttling */
const int NOISE_FLOOR{730};/* noise is below this number when diode unplugged */
const uint32_t READ_INTERVAL{10};
const uint32_t WRITE_INTERVAL{150};/* don't play another tone for at least this many microseconds */
elapsedMillis t0,t1; /* self-updating timers */

/* volume control and display */
const std::array<uint8_t,4>digit_tens{{7,8,9,10}};
const std::array<uint8_t,4>digit_ones{{17,18,19,20}};
int8_t volume{volumes.size()-5};/* index into decibel volume amplitude array (volumes, defined in waveforms.hpp) */

/* audio */
const uint32_t FRAME{11};/* to play 1474 samples in 1/60 of a second, each sample must be about 11.3071 microseconds */
const uint32_t BEGIN_HOLDOFF{31880};/* hard-coded delay to force latency to be a multiple of 1/60 second */
const uint16_t HALF_VCC{2048};/* used to generate 1/2-scale DAC output */
bool play_pnk,play_wht;/* flags to allow one of the tones to play */


/* the following functions are templated to avoid having to type
   std::array<int16_t,1474> over and over again, and to permit
   sample arrays of different lengths, should the need arise. */

/* helper function for casting the const out of a (templated) type */
template<typename T>
using remove_cv_t=typename std::remove_cv<T>::type;

/* Cache the waveform (at the current volume setting) prior to playback,
   because generating ALL of them beforehand takes too much memory. */
template<typename T>
T prescale(T&a, double v){
    remove_cv_t<T> b;
    for(uint32_t i=0;i<a.size();++i){b[i]=HALF_VCC+a[i]*v;}
    return b;
}

/* Play the sample until done.
   Note: this function BLOCKS EVERYTHING ELSE until playback finishes! */
template<typename T>
void play_sample(uint32_t start,T&x){
    while(micros()<start);/* delay start of playback until we line up with a 120Hz video frame */
    for(const auto&s:x){
        uint32_t now=micros();
        analogWrite(A12,s);
        while(micros()<now+FRAME);/* busy-wait to achieve proper sample rate */
    }
}

/* There is a way to write a digital word in parallel on Teensy, but it requires a weird pin layout.
   Since we don't need the speed, we can arrange the pins in a straight line. */
void write_digit(char c,const std::array<uint8_t,4>&digit){
    for(uint8_t i=0;i<digit.size();++i){digitalWrite(digit[i],(c-'0')&(1<<i));}
}

/* Write a 2-digit volume setting (max out at 99). */
void display_volume(int n){
    if(n<0){n=0;}else if(n>=100){n=99;}
    char buf[3]; sprintf(buf,"%02d",n);/* display with single leading 0 */
    write_digit(buf[0],digit_tens);
    write_digit(buf[1],digit_ones);
}

void setup(){

    /* setup voltage reference for audio amplifier */
    analogWriteResolution(12);
    analogWriteFrequency(VREF_PIN,11718.75);/* set vref pwm frequency to 12kHz */
    analogWrite(VREF_PIN,HALF_VCC);/* 1.65V reference, for audio amplifier's positive inputs */
    analogWrite(A12,HALF_VCC);/* set the audio output to the midpoint to avoid clicks */

    /* setup ADC */
    adc->setAveraging(8);
    adc->setResolution(12);
    adc->setSamplingSpeed(ADC_LOW_SPEED);
    adc->setConversionSpeed(ADC_LOW_SPEED);

    /* set up threshold and damping for photodiode filtering */
    PD_WHT.set_alpha(0.1);/* arrived at by trial and error */
    PD_WHT.set_thresh(100);/* arrived at by trial and error */
    PD_PNK.set_alpha(0.1);/* arrived at by trial and error */
    PD_PNK.set_thresh(100);/* arrived at by trial and error */
    for(uint8_t i=0;i<digit_tens.size();++i){pinMode(digit_tens[i],OUTPUT);}
    for(uint8_t i=0;i<digit_ones.size();++i){pinMode(digit_ones[i],OUTPUT);}
    pinMode(MONITOR_PNK,OUTPUT);
    pinMode(MONITOR_WHT,OUTPUT);
    display_volume(5*volume);/* display normally only changes when rotary encoder changes */
}

void loop(){

    /* Input */
    uint32_t now{micros()};
    /* update photodiodes */
    PD_PNK.update(adc->analogRead(A0));
    PD_WHT.update(adc->analogRead(A1));

    /* start playing one of the tones? */
    play_pnk=PD_PNK.hl();//&&(PD_PNK.get_average()>NOISE_FLOOR)
    play_wht=PD_WHT.hl();//&&(PD_WHT.get_average()>NOISE_FLOOR)

    /* update not-as-critical inputs */
    if(((t0>READ_INTERVAL)&&!(play_wht||play_pnk))){
        t0-=READ_INTERVAL;
        //BUTTON_PNK.update();
        //BUTTON_WHT.update();
        ROTARY_ENCODER.update();
        int maybe_new_vol=ROTARY_ENCODER.direction();/* direction is one of -1, 0, or +1 */
        if(maybe_new_vol){/* nonzero means the knob rotated */
            volume-=maybe_new_vol;
            if(volume<0){volume=0;}
            if(volume>=int16_t(volumes.size())){volume=volumes.size()-1;}
            display_volume(5*volume);/* display 5dB steps */
        }
        if(Serial.available()>0){
            int ib=Serial.read();
            switch(ib){
            case'q':Serial.println("");break;
            case'w':PD_PNK.set_alpha(PD_PNK.get_alpha()*1.1);
                Serial.print("ALPHA: ");
                Serial.println(PD_PNK.get_alpha());
                break;
            case'a':PD_PNK.set_thresh(PD_PNK.get_thresh()-1);
                Serial.print("THRESH: ");
                Serial.println(PD_PNK.get_thresh());
                break;
            case's':PD_PNK.set_alpha(PD_PNK.get_alpha()*0.9);
                Serial.print("ALPHA: ");
                Serial.println(PD_PNK.get_alpha());
                break;
            case'd':PD_PNK.set_thresh(PD_PNK.get_thresh()+1);
                Serial.print("THRESH: ");
                Serial.println(PD_PNK.get_thresh());
                break;
            case'i':PD_WHT.set_alpha(PD_WHT.get_alpha()*1.1);
                Serial.print("ALPHA: ");
                Serial.println(PD_WHT.get_alpha());
                break;
            case'j':PD_WHT.set_thresh(PD_WHT.get_thresh()-1);
                Serial.print("THRESH: ");
                Serial.println(PD_WHT.get_thresh());
                break;
            case'k':PD_WHT.set_alpha(PD_WHT.get_alpha()*0.9);
                Serial.print("ALPHA: ");
                Serial.println(PD_WHT.get_alpha());
                break;
            case'l':PD_WHT.set_thresh(PD_WHT.get_thresh()+1);
                Serial.print("THRESH: ");
                Serial.println(PD_WHT.get_thresh());
                break;
            default:
                Serial.print(PD_PNK.get_value());
                Serial.print(" ");
                Serial.print(PD_PNK.get_average());
                Serial.print(" ");
                Serial.print(PD_WHT.get_value());
                Serial.print(" ");
                Serial.println(PD_WHT.get_average());
                break;
            }
        }
    }
    else{
        /* Output */
        if(play_pnk){
            play_pnk=false;
            if(t1>WRITE_INTERVAL){
                t1=0;
                play_sample(now+BEGIN_HOLDOFF,prescale(pink,volumes[volume]));
            }
        }
        else if(play_wht){
            play_wht=false;
            if(t1>WRITE_INTERVAL){
                t1=0;
                play_sample(now+BEGIN_HOLDOFF,prescale(white,volumes[volume]));
            }
        }
    }
    digitalWrite(MONITOR_PNK,play_pnk);
    digitalWrite(MONITOR_WHT,play_wht);
}
