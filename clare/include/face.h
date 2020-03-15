#ifndef FACE_H
#define FACE_H

#include "LedControl.h"
#include "ledmasks.h"


/*
pin 2 is connected to the DataIn
pin 4 is connected to the CLK
pin 3 is connected to LOAD
*/
const int DATA_IN_PIN = 2;
const int CLK_PIN = 4;
const int CS_PIN = 3;
const int NUM_DEVICES = 4;

class Face {
    private:
        LedControl flc = LedControl(DATA_IN_PIN, CLK_PIN, CS_PIN, NUM_DEVICES);
        int fleftEyeIdx = -1;
        int frightEyeIdx = -1;
        int fleftMouthIdx = -1;
        int frightMouthIdx = -1;

        void setTuple(const frameType, const frameType, const int i, const int j);
        void setEyes(const frameType, const frameType);
        void setEyeExpression(const frameType expression[], const int size);
        void setMouth(const frameType, const frameType);
        void setMouthExpression(const frameType expression[], const int size);
    public:
        Face(int le, int re, int lm, int rm);
        void reset();
        void blink();
        void sadEyes();
        void angryEyes();
        void ouchEyes();
        void smileEyes();
        void lookLeft();
        void lookRight();
        void lookUp();
        void lookDown();
        void ughMouth();
        void smileMouth();
        void sadMouth();
        void sillyMouth();
        void kissMouth();
        void surpriseMouth();
        void crossEyes();
        void neutralEyes();
        
        void happy();
        void sad();
        void angry();
        void silly();
        void surprised();
        void ugh();
        void confused();
        void kiss();
};

#endif