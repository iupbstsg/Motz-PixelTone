/* RotaryEncoder.h -- polling-based rotary encoder reader
   Author: Alex Shroyer
   Copyright (c) 2014,2015,2016 Trustees of Indiana University
*/
#pragma once
#include "Arduino.h"
class RotaryEncoder
{
public:
    RotaryEncoder(uint8_t pinA, uint8_t pinB);
    void update(void);// read inputs
    int8_t position(void);// current (relative) position
    int8_t direction(void);// (-1, 0, 1) for (left, unchanged, right)
    int8_t getKnobPosition(void);
    void setKnobPosition(int8_t knobPos);

private:
    uint8_t _pinA, _pinB, oldState=0, currentState=0;
    int8_t read(void);
    static const int8_t stateTransitionTable[16];// initialized in Encoder.cpp
    static int8_t _position;
    int8_t _knobPos, _oldKnobPos;
};
