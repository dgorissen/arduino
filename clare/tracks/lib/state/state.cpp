#include "state.h"
#include "Arduino.h"

State::State() {
    fStatus = UNKNOWN;
    fMotorL = -999;
    fMotorR = -999;
    fMode = MANUAL;
    fHeadlights = false;
    fEncoderL = -999;
    fEncoderR = -999;
}
    
State::~State(){
}

void State::setStatus(const Status s) {
    fStatus = s;
}

void State::clearStatus() {
    fStatus = SET;
}

bool State::isModified() const {
    return fStatus == MODIFIED;
}

void State::setMotors(const int l, const int r) {
    if(l != fMotorL || r != fMotorR) {
        fMotorL = l;
        fMotorR = r;
        setStatus(MODIFIED);
    }
}

void State::setEncoders(const long l, const long r){
    if(l != fEncoderL || r != fEncoderR) {
        fEncoderL = l;
        fEncoderR = r;
        setStatus(MODIFIED);
    }
}

void State::setHeadlights(const bool h){
    if (fHeadlights != h) {
        fHeadlights = h;
        setStatus(MODIFIED);
    }
}

void State::setMode(const Mode m) {
    if(fMode != m) {
        fMode = m;
        setStatus(MODIFIED);
    }
}

String State::serialise() const {
    String s = String("ML:") + fMotorL + ","
       "MR:" + fMotorR + "," +
       "EL:" + fEncoderL + "," +
       "ER:" + fEncoderR + "," +
       "H:" + fHeadlights + "," +
       "M:" + fMode;
    return s;
}
