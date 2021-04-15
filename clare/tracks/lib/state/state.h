#ifndef STATE_H
#define STATE_H

#include "Arduino.h"

enum Mode {M_NOTSET=-1, MANUAL=1, AUTONOMOUS=2};
enum Status {S_NOTSET=-1, SET=1, MODIFIED=2};
enum Headlights {HL_NOTSET=-1, HL_OFF=0, HL_ON=1};

class State {
private:
    /* data */
    Headlights fHeadlights;
    int fMotorR;
    int fMotorL;
    Mode fMode;
    long fEncoderR;
    long fEncoderL;
    Status fStatus;

   void setStatus(const Status s);

public:
   State();
   ~State();
   void reset();
   void setMotorL(const int v);
   void setMotorR(const int v);
   void setMotors(const int l, const int r);
   bool motorsSet() const;

   void setEncoders(const long l, const long r);
   
   void setHeadlights(const bool h);
   Headlights getHeadlights() const;
   
   void setMode(const Mode m);
   Mode getMode() const;

   void clearStatus();
   bool isModified() const;

   String serialise() const;
   static bool parseState(String s, State &state);
};

#endif