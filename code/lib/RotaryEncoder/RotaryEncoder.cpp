/* Quadrature Encoders output Gray code (00, 01, 11, 10...)
   State table indices are combined old value (row) and current value (column)
   Which means you must shift the old value two spots to the left.
   Credit: http://letsmakerobots.com/content/how-use-quadrature-encoder
   error state == 2, handle appropriately
*/
#include "RotaryEncoder.hpp"
const int8_t RotaryEncoder::stateTransitionTable[16]
{
   0, -1,  1,  2,
   1,  0,  2, -1,
  -1,  2,  0,  1,
   2,  1, -1,  0
};

int8_t RotaryEncoder::_position{0}; // static

// constructor/destructor
RotaryEncoder::RotaryEncoder(uint8_t pinA, uint8_t pinB):_pinA(pinA),_pinB(pinB)
{
  pinMode(_pinA, INPUT_PULLUP);
  pinMode(_pinB, INPUT_PULLUP);
};

// High-level functions
void RotaryEncoder::update(void){_knobPos=position()/4;}

int8_t RotaryEncoder::direction(void)
{
  int8_t change=_knobPos-_oldKnobPos;
  if (change){
    _oldKnobPos=_knobPos;
    return ((change > 0)?1:-1);
  }
  return 0;
}

int8_t RotaryEncoder::position(void)
{ // Divide by 4 or take modulus for "detent"-based steps.
  int8_t val=read();
  if(val!=2)// ignore error state
    _position+=val;
  return _position;
}

int8_t RotaryEncoder::read(void)
{
  oldState=currentState;
  currentState=digitalRead(_pinA)*2+digitalRead(_pinB); // store as 0bXXXXXXAB
  return stateTransitionTable[oldState*4+currentState];
}

// getters and setters
void RotaryEncoder::setKnobPosition(int8_t knobPos=0){_knobPos=knobPos;}
int8_t RotaryEncoder::getKnobPosition(void){return _knobPos;}
