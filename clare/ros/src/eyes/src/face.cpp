#include "face.h"
#include "utils.h"

const int animTime = 50;
binaryArrayType binaryArray[61] =		
{		
	{ // LeftNeutral1, 0	
		B01111110,
		B10000001,
		B10000001,
		B10011001,
		B10011001,
		B10000001,
		B10000001,
		B01111110
	},	
	{ // RightNeutral1, 1	
		B01111110,
		B10000001,
		B10000001,
		B10011001,
		B10011001,
		B10000001,
		B10000001,
		B01111110
	},	
	{ // LeftNeutral2, 2	
		B00000000,
		B00111100,
		B01000010,
		B01011010,
		B01011010,
		B01000010,
		B00111100,
		B00000000
	},	
	{ // RightNeutral2, 3	
		B00000000,
		B00111100,
		B01000010,
		B01011010,
		B01011010,
		B01000010,
		B00111100,
		B00000000
	},	
	{ // LeftNeutral3, 4	
		B00000000,
		B00111100,
		B00100100,
		B00110100,
		B00110100,
		B00100100,
		B00111100,
		B00000000
	},	
	{ // RightNeutral3, 5	
		B00000000,
		B00111100,
		B00100100,
		B00110100,
		B00110100,
		B00100100,
		B00111100,
		B00000000
	},	
	{ // LeftNeutral4, 6	
		B00011000,
		B00011000,
		B00011000,
		B00011000,
		B00011000,
		B00011000,
		B00011000,
		B00000000
	},	
	{ // RightNeutral4, 7	
		B00000000,
		B00011000,
		B00011000,
		B00011000,
		B00011000,
		B00011000,
		B00011000,
		B00011000
	},	
	{ // LeftAngry1, 8	
		B01111110,
		B10000001,
		B10000001,
		B10011001,
		B10011001,
		B10000001,
		B10000010,
		B01111100
	},	
	{ // RightAngry1, 9	
		B01111100,
		B10000010,
		B10000001,
		B10011001,
		B10011001,
		B10000001,
		B10000001,
		B01111110
	},	
	{ // LeftAngry2, 10	
		B01111110,
		B10000001,
		B10000001,
		B10011001,
		B10011001,
		B10000010,
		B10000100,
		B01111000
	},	
	{ // RightAngry2, 11	
		B01111000,
		B10000100,
		B10000010,
		B10011001,
		B10011001,
		B10000001,
		B10000001,
		B01111110
	},	
	{ // LeftAngry3, 12	
		B01111110,
		B11000001,
		B10000001,
		B10011001,
		B10011010,
		B10000100,
		B10001000,
		B01110000
	},	
	{ // RightAngry3, 13	
		B01110000,
		B10001000,
		B10000100,
		B10011010,
		B10011001,
		B10000001,
		B11000001,
		B01111110
	},	
	{ // LeftAngry4, 14	
		B00111110,
		B01000001,
		B10000001,
		B10011001,
		B10011010,
		B10000100,
		B01001000,
		B00110000
	},	
	{ // RightAngry4, 15	
		B00110000,
		B01001000,
		B10000100,
		B10011010,
		B10011001,
		B10000001,
		B01000001,
		B00111110
	},	
	{ // LeftRight1, 16	
		B01111110,
		B10000001,
		B10000001,
		B10000001,
		B10011001,
		B10011001,
		B10000001,
		B01111110
	},	
	{ // RightLeft1, 17	
		B01111110,
		B10000001,
		B10011001,
		B10011001,
		B10000001,
		B10000001,
		B10000001,
		B01111110
	},	
	{ // LeftRight2, 18	
		B01111110,
		B10000001,
		B10000001,
		B10000001,
		B10000001,
		B10011001,
		B10011001,
		B01111110
	},	
	{ // RightLeft2, 19	
		B01111110,
		B10011001,
		B10011001,
		B10000001,
		B10000001,
		B10000001,
		B10000001,
		B01111110
	},	
	{ // LeftRight3, 20	
		B01111110,
		B10000001,
		B10000001,
		B10000001,
		B10000001,
		B10000001,
		B10011001,
		B01111110
	},	
	{ // RightLeft3, 21	
		B01111110,
		B10011001,
		B10000001,
		B10000001,
		B10000001,
		B10000001,
		B10000001,
		B01111110
	},	
	{ // LeftRightSmall1, 22	
		B00000000,
		B00111100,
		B01000010,
		B01000010,
		B01011010,
		B01011010,
		B00111100,
		B00000000
	},	
	{ // RightLeftSmall1, 23	
		B00000000,
		B00111100,
		B01011010,
		B01011010,
		B01000010,
		B01000010,
		B00111100,
		B00000000
	},	
	{ // LeftRightSmall3, 24	
		B00000000,
		B00111100,
		B00100100,
		B00100100,
		B00100100,
		B00111100,
		B00111100,
		B00000000
	},	
	{ // RightLeftSmall3, 25	
		B00000000,
		B00111100,
		B00111100,
		B00100100,
		B00100100,
		B00100100,
		B00111100,
		B00000000
	},	
	{ // LeftUp1, 26	
		B01111110,
		B10000001,
		B10000001,
		B10001101,
		B10001101,
		B10000001,
		B10000001,
		B01111110
	},	
	{ // LeftUp2, 27	
		B01111110,
		B10000001,
		B10000001,
		B10000111,
		B10000111,
		B10000001,
		B10000001,
		B01111110
	},	
	{ // LeftUp3, 28	
		B01111110,
		B10000001,
		B10000001,
		B10000011,
		B10000011,
		B10000001,
		B10000001,
		B01111110
	},	
	{ // LeftDown1, 29	
		B01111110,
		B10000001,
		B10000001,
		B10110001,
		B10110001,
		B10000001,
		B10000001,
		B01111110
	},	
	{ // LeftDown2, 30	
		B01111110,
		B10000001,
		B10000001,
		B11100001,
		B11100001,
		B10000001,
		B10000001,
		B01111110
	},	
	{ // LeftDown3, 31	
		B01111110,
		B10000001,
		B10000001,
		B11000001,
		B11000001,
		B10000001,
		B10000001,
		B01111110
	},	
	{ // LeftSad3, 32	
		B01110000,
		B10001000,
		B10000100,
		B10011010,
		B10011001,
		B10000001,
		B11000001,
		B01111110
	},	
	{ // RightSad3, 33	
		B01111110,
		B11000001,
		B10000001,
		B10011001,
		B10011010,
		B10000100,
		B10001000,
		B01110000
	},	
	{ // LeftSad2, 34	
		B01111000,
		B10000100,
		B10000010,
		B10011001,
		B10011001,
		B10000001,
		B10000001,
		B01111110
	},	
	{ // RightSad2, 35	
		B01111110,
		B10000001,
		B10000001,
		B10011001,
		B10011001,
		B10000010,
		B10000100,
		B01111000
	},	
	{ // LeftSad1, 36	
		B01111100,
		B10000010,
		B10000001,
		B10011001,
		B10011001,
		B10000001,
		B10000001,
		B01111110
	},	
	{ // RightSad1, 37	
		B01111110,
		B10000001,
		B10000001,
		B10011001,
		B10011001,
		B10000001,
		B10000010,
		B01111100
	},	
	{ // LeftNeutral5, 38	
		B00010000,
		B00010000,
		B00010000,
		B00010000,
		B00010000,
		B00010000,
		B00010000,
		B00000000
	},	
	{ // RightNeutral5, 39	
		B00000000,
		B00010000,
		B00010000,
		B00010000,
		B00010000,
		B00010000,
		B00010000,
		B00010000
	},	
	{ // LeftAngry5, 40	
		B00111110,
		B01000001,
		B10000001,
		B10011001,
		B10010010,
		B10000100,
		B01001000,
		B00110000
	},	
	{ // RightAngry5, 41	
		B00110000,
		B01001000,
		B10000100,
		B10010010,
		B10011001,
		B10000001,
		B01000001,
		B00111110
	},	
	{ // LeftRightSmall2, 42	
		B00000000,
		B00111100,
		B01000010,
		B01000010,
		B01000010,
		B01011010,
		B00111100,
		B00000000
	},	
	{ // RightLeftSmall2, 43	
		B00000000,
		B00111100,
		B01011010,
		B01000010,
		B01000010,
		B01000010,
		B00111100,
		B00000000
	},	
	{ // LeftSad4, 44	
		B00110000,
		B01001000,
		B10000100,
		B10011010,
		B10011001,
		B10000001,
		B01000001,
		B00111110
	},	
	{ // RightSad4, 45	
		B00111110,
		B01000001,
		B10000001,
		B10011001,
		B10011010,
		B10000100,
		B01001000,
		B00110000
	},	
	{ // LeftSmile, 46	
		B00000000,
		B00110000,
		B00001100,
		B00000110,
		B00000110,
		B00001100,
		B00110000,
		B00000000
	},	
	{ // RightChevron, 47	
		B00000000,
		B00001000,
		B00010100,
		B00100010,
		B01000001,
		B00000000,
		B00000000,
		B00000000
	},	
	{ // LeftChevron, 48	
		B00000000,
		B00000000,
		B00000000,
		B01000001,
		B00100010,
		B00010100,
		B00001000,
		B00000000
	},	
	{ // LeftMouthSmile, 49	
		B00000011,
		B00000110,
		B00001100,
		B00011000,
		B00010000,
		B00110000,
		B01100000,
		B01100000
	},	
	{ // RightMouthSmile, 50	
		B01100000,
		B01100000,
		B00110000,
		B00010000,
		B00011000,
		B00001100,
		B00000110,
		B00000011
	},	
	{ // LeftMouthSad, 51	
		B11000000,
		B01100000,
		B00110000,
		B00011000,
		B00001000,
		B00001100,
		B00000110,
		B00000110
	},	
	{ // RightMouthSad, 52	
		B00000110,
		B00000110,
		B00001100,
		B00001000,
		B00011000,
		B00110000,
		B01100000,
		B11000000
	},	
	{ // LeftMouthSilly, 53	
		B00000001,
		B00000110,
		B00001100,
		B00011000,
		B00011000,
		B11111000,
		B10011000,
		B11111000
	},	
	{ // RightMouthSilly, 54	
		B00011000,
		B00011000,
		B00011000,
		B00011000,
		B00011000,
		B00001100,
		B00000110,
		B00000001
	},	
	{ // LeftMouthSquiggle, 55	
		B00001000,
		B00010000,
		B00001000,
		B00000100,
		B00001000,
		B00010000,
		B00001000,
		B00000100
	},	
	{ // RightMouthSquiggle, 56	
		B00001000,
		B00010000,
		B00001000,
		B00000100,
		B00001000,
		B00010000,
		B00001000,
		B00000100
	},	
	{ // LeftMouthO, 57	
		B00000000,
		B00000000,
		B00000000,
		B00000000,
		B00011000,
		B00100100,
		B01000010,
		B10000001
	},	
	{ // RightMouthO, 58	
		B10000001,
		B01000010,
		B00100100,
		B00011000,
		B00000000,
		B00000000,
		B00000000,
		B00000000
	},	
	{ // LeftMouthX, 59	
		B00000000,
		B00000000,
		B00000000,
		B00000000,
		B00000000,
		B00000000,
		B00100010,
		B00010100
	},	
	{ // RightMouthX, 60	
		B00001000,
		B00010100,
		B00100010,
		B00000000,
		B00000000,
		B00000000,
		B00000000,
		B00000000
	}	
};		

Face::Face(int le, int re, int lm, int rm) {
  fleftEyeIdx = le;
  frightEyeIdx = re;
  fleftMouthIdx = lm;
  frightMouthIdx = rm;
}

void Face::reset() {
  for (int x = 0; x < NUM_DEVICES; x++) {
    // The MAX72XX is in power-saving mode on startup
	flc.shutdown(x, false);
    // Set the brightness to default value
	flc.setIntensity(x, 1);
    flc.clearDisplay(x);
  }
}

void Face::setTuple(const frameType fi, const frameType fj, const int i,
                    const int j) {
  for (int row = 0; row < 8; row++) {
    flc.setRow(i, row, binaryArray[fi.frameCount].array1[row]);
    flc.setRow(j, row, binaryArray[fj.frameCount].array1[row]);
    flc.setIntensity(i, fi.frameLuminance);
    flc.setIntensity(j, fj.frameLuminance);
  }
}

void Face::setEyes(const frameType leftEye, const frameType rightEye) {
  setTuple(leftEye, rightEye, fleftEyeIdx, frightEyeIdx);
}

void Face::setMouth(const frameType left, const frameType right) {
  setTuple(left, right, fleftMouthIdx, frightMouthIdx);
}

void Face::setEyeExpression(const frameType expression[], const int size) {
  for (int a = 0; a < size; a = a + 2) {
    setEyes(expression[a], expression[a + 1]);
    delayMillis(expression[a].frameDelay);
  }
}

void Face::setMouthExpression(const frameType expression[], const int size) {
  for (int a = 0; a < size; a = a + 2) {
    setMouth(expression[a], expression[a + 1]);
    delayMillis(expression[a].frameDelay);
  }
}

void Face::blink() {
  frameType neutralBlink[] = {
      {LeftNeutral1, 2000, 1},     {RightNeutral1, 2000, 1},
      {LeftNeutral2, animTime, 1}, {RightNeutral2, animTime, 1},
      {LeftNeutral3, animTime, 1}, {RightNeutral3, animTime, 1},
      {LeftNeutral4, animTime, 1}, {LeftNeutral4, animTime, 1},
  };

  setEyeExpression(neutralBlink, arraySize(neutralBlink));
}

void Face::sadEyes() {
  frameType sadEyes[] = {
      {LeftNeutral1, animTime * 2, 1},
      {RightNeutral1, animTime * 2, 1},
      {LeftSad1, animTime, 1},
      {RightSad1, animTime, 1},
      {LeftSad2, animTime, 1},
      {RightSad2, animTime, 1},
      {LeftSad3, animTime, 1},
      {RightSad2, animTime, 1},
      {LeftSad4, 0, 1},
      {RightSad4, 0, 1},
  };
  setEyeExpression(sadEyes, arraySize(sadEyes));
}

void Face::angryEyes() {
  frameType angryEyes[] = {
      {LeftNeutral1, animTime * 2, 1}, {RightNeutral1, animTime * 2, 1},
      {LeftAngry1, animTime, 1},       {RightAngry1, animTime, 1},
      {LeftAngry2, animTime, 1},       {RightAngry2, animTime, 1},
      {LeftAngry3, animTime, 1},       {RightAngry3, animTime, 1},
      {LeftAngry4, animTime, 1},       {RightAngry4, animTime, 1},
      {LeftAngry5, 0, 1},           {RightAngry5, 0, 1},
  };
  setEyeExpression(angryEyes, arraySize(angryEyes));
}

void Face::lookRight() {
  frameType lookRight[] = {
      {LeftNeutral1, animTime * 2, 1},
      {RightNeutral1, animTime * 2, 1},
      {LeftRight1, animTime, 1},
      {LeftRight1, animTime, 1},
      {LeftRight2, animTime, 1},
      {LeftRight2, animTime, 1},
      {LeftRight3, 500, 1},
      {LeftRight3, 500, 1},
      {LeftRight2, animTime, 1},
      {LeftRight2, animTime, 1},
      {LeftRight1, animTime, 1},
      {LeftRight1, animTime, 1},
      {LeftNeutral1, animTime * 2, 1},
      {RightNeutral1, animTime * 2, 1},
  };
  setEyeExpression(lookRight, arraySize(lookRight));
}

void Face::lookLeft() {
  frameType lookLeft[] = {
      {LeftNeutral1, animTime * 2, 1},
      {RightNeutral1, animTime * 2, 1},
      {RightLeft1, animTime, 1},
      {RightLeft1, animTime, 1},
      {RightLeft2, animTime, 1},
      {RightLeft2, animTime, 1},
      {RightLeft3, 500, 1},
      {RightLeft3, 500, 1},
      {RightLeft2, animTime, 1},
      {RightLeft2, animTime, 1},
      {RightLeft1, animTime, 1},
      {RightLeft1, animTime, 1},
      {LeftNeutral1, animTime * 2, 1},
      {RightNeutral1, animTime * 2, 1},
  };
  setEyeExpression(lookLeft, arraySize(lookLeft));
}

void Face::crossEyes() {
  frameType lookLeft[] = {
      {LeftNeutral1, animTime * 2, 1},
      {RightNeutral1, animTime * 2, 1},
      {LeftRight1, animTime, 1},
      {RightLeft1, animTime, 1},
      {LeftRight2, animTime, 1},
      {RightLeft2, animTime, 1},
      {LeftRight3, 0, 1},
      {RightLeft3, 0, 1},
  };
  setEyeExpression(lookLeft, arraySize(lookLeft));
}

void Face::lookUp() {
  frameType lookUp[] = {
      {LeftNeutral1, animTime * 2, 1},
      {RightNeutral1, animTime * 2, 1},
      {LeftUp1, animTime, 1},
      {LeftUp1, animTime, 1},
      {LeftUp2, animTime, 1},
      {LeftUp2, animTime, 1},
      {LeftUp3, 500, 1},
      {LeftUp3, 500, 1},
      {LeftUp2, animTime, 1},
      {LeftUp2, animTime, 1},
      {LeftUp1, animTime, 1},
      {LeftUp1, animTime, 1},
      {LeftNeutral1, animTime * 2, 1},
      {RightNeutral1, animTime * 2, 1},
  };
  setEyeExpression(lookUp, arraySize(lookUp));
}

void Face::lookDown() {
  frameType lookDown[] = {
      {LeftNeutral1, animTime * 2, 1},
      {RightNeutral1, animTime * 2, 1},
      {LeftDown1, animTime, 1},
      {LeftDown1, animTime, 1},
      {LeftDown2, animTime, 1},
      {LeftDown2, animTime, 1},
      {LeftDown3, 500, 1},
      {LeftDown3, 500, 1},
      {LeftDown2, animTime, 1},
      {LeftDown2, animTime, 1},
      {LeftDown1, animTime, 1},
      {LeftDown1, animTime, 1},
      {LeftNeutral1, animTime * 2, 1},
      {RightNeutral1, animTime * 2, 1},
  };
  setEyeExpression(lookDown, arraySize(lookDown));
}

void Face::neutralEyes() {
  frameType neutralEyes[] = {
      {LeftNeutral1, 0, 1},
      {RightNeutral1, 0, 1},
  };
  setEyeExpression(neutralEyes, arraySize(neutralEyes));
}

void Face::ouchEyes() {
  frameType ouchEyes[] = {
      {LeftChevron, 0, 1},
      {RightChevron, 0, 1},
  };
  setEyeExpression(ouchEyes, arraySize(ouchEyes));
}

void Face::smileEyes() {
  frameType smileEyes[] = {
      {LeftSmile, 0, 1},
      {LeftSmile, 0, 1},
  };
  setEyeExpression(smileEyes, arraySize(smileEyes));
}

void Face::smileMouth() {
  frameType smile[] = {
      {LeftMouthSmile, 0, 1},
      {RightMouthSmile, 0, 1},
  };
  setMouthExpression(smile, arraySize(smile));
}

void Face::sadMouth() {
  frameType sad[] = {
      {LeftMouthSad, 0, 1},
      {RightMouthSad, 0, 1},
  };
  setMouthExpression(sad, arraySize(sad));
}

void Face::kissMouth() {
  frameType kiss[] = {
      {LeftMouthX, 0, 1},
      {RightMouthX, 0, 1},
  };
  setMouthExpression(kiss, arraySize(kiss));
}

void Face::sillyMouth() {
  frameType silly[] = {
      {LeftMouthSilly, 0, 1},
      {RightMouthSilly, 0, 1},
  };
  setMouthExpression(silly, arraySize(silly));
}

void Face::surpriseMouth() {
  frameType surprise[] = {
      {LeftMouthO, 0, 1},
      {RightMouthO, 0, 1},
  };
  setMouthExpression(surprise, arraySize(surprise));
}

void Face::ughMouth() {
  frameType ugh[] = {
      {LeftMouthSquiggle, 0, 1},
      {RightMouthSquiggle, 0, 1},
  };
  setMouthExpression(ugh, arraySize(ugh));
}

void Face::happy(){
    smileEyes();
    smileMouth();
}

void Face::sad(){
    sadEyes();
    sadMouth();
}

void Face::angry(){
    angryEyes();
    sadMouth();
}

void Face::silly(){
    smileEyes();
    sillyMouth();
}

void Face::surprised(){
    neutralEyes();
    surpriseMouth();
}

void Face::ugh(){
    ouchEyes();
    ughMouth();
}

void Face::confused(){
    crossEyes();
    ughMouth();
}

void Face::kiss(){
    smileEyes();
    kissMouth();
}
