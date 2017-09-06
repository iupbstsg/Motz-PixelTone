/* Aswitch.hpp -- pushbutton behavior from an analog pin
   Author: Alex Shroyer

   The place to use this library is when you want to sample a relatively-slow-moving
   value with some low-frequency noise or ripple, but which has occasional short-duration
   spikes corresponding to the "switch" you want to react to.

   This originated as a wrapper for a touch-sensitive pin on at TeensyLC, which I wanted
   to use as a pushbutton.  To use it like this, pass in `touchRead` as the callback function.

   Another use case is reading a regular analog pin - pass in `analogRead` instead.
*/
#pragma once
#include <Arduino.h>

class Aswitch{
public:
    /* @param p: analog-enabled pin
       @param f: either `analogRead` or `touchRead`
       NOTE: touchRead is a Teensy 3.x (and Teensy LC) built-in
    */
    Aswitch(uint8_t p):pin(p),threshold(127){}

    void update(int newvalue){
        //value=func(pin);
        value=newvalue;
        if(accept_next){
            accept_next=false;
            average=iir(value,average);
            current_state=(average-value>threshold)||!(value-average>threshold);
            hilo=prev_state&&!current_state;/* falling edge reset in hl() */
            lohi=current_state&&!prev_state;/* rising edge reset in lh() */
        }
        prev_state=current_state;
    }

    bool hl(void){return true_once(hilo);}
    bool lh(void){return true_once(lohi);}

    int get_value(void){return value;}
    int get_average(void){return average;}

    int get_thresh(void){return threshold;}
    double get_alpha(void){return alpha;}

    void set_thresh(int n){if(n<1){n=1;}else if(n>4095){n=4095;}else{threshold=n;}}
    void set_alpha(double n){if(n>=1){n=1.0;}else if(n<=0.01){n=0.01;}else{alpha=n;}}

private:
    bool true_once(bool &edge){
        bool t=edge;
        edge=false;
        accept_next=true;
        return t;
    }

    int16_t iir(int n,int o){
        return static_cast<int16_t>(alpha*(n-o)+o);
    }

    uint8_t pin;
    int (*func)(uint8_t);
    int threshold,value,average;
    double alpha{0.1};
    bool current_state,prev_state,hilo,lohi,accept_next{true};
};
