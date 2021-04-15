#include "state.h"
#include "Arduino.h"

State::State() {
    reset();
}
    
State::~State(){
}

void State::reset(){
    fStatus = S_NOTSET;
    fMotorL = -999;
    fMotorR = -999;
    fMode = M_NOTSET;
    fHeadlights = HL_NOTSET;
    fEncoderL = -999;
    fEncoderR = -999;
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
    setMotorL(l);
    setMotorR(r);
}

void State::setMotorL(const int v) {
    if(v != fMotorL) {
        fMotorL = v;
        setStatus(MODIFIED);
    }
}

void State::setMotorR(const int v) {
    if(v != fMotorR) {
        fMotorR = v;
        setStatus(MODIFIED);
    }
}

bool State::motorsSet() const {
    return (fMotorR != -999) && (fMotorL != -999);
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
        fHeadlights = h ? HL_ON : HL_OFF;
        setStatus(MODIFIED);
    }
}

Headlights State::getHeadlights() const {
    return fHeadlights;
}

void State::setMode(const Mode m) {
    if(fMode != m) {
        fMode = m;
        setStatus(MODIFIED);
    }
}

Mode State::getMode() const {
    return fMode;
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

bool State::parseState(String s, State &state) {
    const int size = 50;
    char ca[size];
    s.toCharArray(ca, size);

    //https://arduino.stackexchange.com/questions/1013/how-do-i-split-an-incoming-string
    // Read each command pair 
    char* cmd = strtok(ca, ",");
    while (cmd != 0) {
        // Split the command in two values
        char* sep = strchr(cmd, ':');
        if (sep != 0)
        {
            // Actually split the string in 2: replace ':' with 0
            *sep = 0;
            ++sep;

            if(strcmp(cmd, "ML") == 0) {
                state.setMotorL(atoi(sep));
            }else if(strcmp(cmd, "MR") == 0){
                state.setMotorR(atoi(sep));
            }else if(strcmp(cmd, "H") == 0){
                state.setHeadlights(atoi(sep));
            }else if(strcmp(cmd, "M") == 0){
                if(strcmp(sep, "A") == 0){
                    state.setMode(AUTONOMOUS);
                } else if(strcmp(sep, "M") == 0){
                    state.setMode(MANUAL);
                } else {
                    // Invalid msg
                    return false;
                }
            }else {
                // Invalid msg
                return false;
            }
        }
        // Find the next command in input string
        cmd = strtok(0, ",");
    }

    state.setStatus(MODIFIED);
    return true;
}
