#ifndef LEDMASKS_H
#define LEDMASKS_H

// Drop this code into Arduino development environment		
		
#define LeftNeutral1 0		
#define RightNeutral1 1		
#define LeftNeutral2 2		
#define RightNeutral2 3		
#define LeftNeutral3 4		
#define RightNeutral3 5		
#define LeftNeutral4 6		
#define RightNeutral4 7		
#define LeftAngry1 8		
#define RightAngry1 9		
#define LeftAngry2 10		
#define RightAngry2 11		
#define LeftAngry3 12		
#define RightAngry3 13		
#define LeftAngry4 14		
#define RightAngry4 15		
#define LeftRight1 16		
#define RightLeft1 17		
#define LeftRight2 18		
#define RightLeft2 19		
#define LeftRight3 20		
#define RightLeft3 21		
#define LeftRightSmall1 22		
#define RightLeftSmall1 23		
#define LeftRightSmall3 24		
#define RightLeftSmall3 25		
#define LeftUp1 26		
#define LeftUp2 27		
#define LeftUp3 28		
#define LeftDown1 29		
#define LeftDown2 30		
#define LeftDown3 31		
#define LeftSad3 32		
#define RightSad3 33		
#define LeftSad2 34		
#define RightSad2 35		
#define LeftSad1 36		
#define RightSad1 37		
#define LeftNeutral5 38		
#define RightNeutral5 39		
#define LeftAngry5 40		
#define RightAngry5 41		
#define LeftRightSmall2 42		
#define RightLeftSmall2 43		
#define LeftSad4 44		
#define RightSad4 45		
#define LeftSmile 46		
#define RightChevron 47		
#define LeftChevron 48		
#define LeftMouthSmile 49		
#define RightMouthSmile 50		
#define LeftMouthSad 51		
#define RightMouthSad 52		
#define LeftMouthSilly 53		
#define RightMouthSilly 54		
#define LeftMouthSquiggle 55		
#define RightMouthSquiggle 56		
#define LeftMouthO 57		
#define RightMouthO 58		
#define LeftMouthX 59		
#define RightMouthX 60		
		
typedef struct {		
	byte array1[8];	
} binaryArrayType;		
		
typedef struct {			
	int frameCount;		// index pointer into binaryArray signifying animation frame
	int frameDelay;	    // Approx delay in MilliSeconds to hold display this animated frame
    int frameLuminance;	// 0 ... 15, The intensity of the led matrix for a given frame
} frameType;


#endif