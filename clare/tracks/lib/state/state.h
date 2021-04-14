#ifndef STATE_H
#define STATE_H

#include "Arduino.h"

enum Mode {MANUAL=1, AUTONOMOUS=2};
enum Status {UNKNOWN=1, SET=2, MODIFIED=3};

class State {
private:
    /* data */
    bool fHeadlights;
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
   void setMotors(const int l, const int r);
   void setEncoders(const long l, const long r);
   void setHeadlights(const bool h);
   void setMode(const Mode m);
   String serialise() const;
   void clearStatus();
   bool isModified() const;
};

#endif