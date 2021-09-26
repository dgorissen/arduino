#include "state.h"
#include "Arduino.h"

State::State() {
    reset();
}
    
State::~State(){
}

void State::reset(){
    fTimestamp = -999;
    fStatus = S_NOTSET;
    fMotorL = -999;
    fMotorR = -999;
    fCmdX = -999;
    fCmdY = -999;
    fMode = M_NOTSET;
    fHeadlights = HL_NOTSET;
    fEncoderL = -999;
    fEncoderR = -999;
}

void State::setCurTimestamp() {
    fTimestamp = millis();
}

void State::setTimestamp(const long ts) {
    fTimestamp = ts;
}

long State::getTimestamp() const {
    return fTimestamp;
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

bool State::isSet() const {
    return fStatus == SET;
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

int State::getCmdX() const {
    return fCmdX;
}

int State::getCmdY() const {
    return fCmdY;
}

void State::setCmdX(const int x) {
    fCmdX = x;
}

void State::setCmdY(const int y) {
    fCmdY = y;
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
    String s = String("TS:") + fTimestamp + ","
       "ML:" + fMotorL + ","
       "MR:" + fMotorR + "," +
       "CX:" + fCmdX + ","
       "CY:" + fCmdY + "," +
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

    // reset the state
    state.reset();

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
            }else if(strcmp(cmd, "CX") == 0){
                state.setCmdX(atoi(sep));
            }else if(strcmp(cmd, "CY") == 0){
                state.setCmdY(atoi(sep));
            }else if(strcmp(cmd, "TS") == 0){
                state.setTimestamp(atoi(sep));
            }else if(strcmp(cmd, "H") == 0){
                state.setHeadlights(atoi(sep));
            }else if(strcmp(cmd, "M") == 0){
                if(strcmp(sep, "A") == 0){
                    state.setMode(AUTONOMOUS);
                } else if(strcmp(sep, "RC") == 0){
                    state.setMode(RC_CONTROL);
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
    //TODO: we always override the timestamp?
    state.setCurTimestamp();
    return true;
}
