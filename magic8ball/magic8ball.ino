/*   Magic 8 Ball with a 16x2 LCD + Tilt switch */
// Tilt pin
#define TILT 19

#include <LiquidCrystal.h>

// Setup the LCD
const int rs = 12, en = 11, d4 = 6, d5 = 7, d6 = 8, d7 = 9;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Is the switch on/off
bool switchState = false;
bool prevState = false;
bool tiltReading = false;
bool prevReading = false;

long lastTime = 0;
long debounce = 200;

typedef struct {
   char description [35];
} msgType;


// Magic 8 Ball strings
const msgType responses[] PROGMEM = {
  {"Are you a girl? "},
  {"Are you a boy?  "},
  {"Does Lucas like raddishes?"},
  {"Do you like aubergines?"},

  {"It is certain.  "},
  {"Decidedly so.   "},
  {"Without a doubt."},
  {"Definitely.     "},
  {"Rely on it.     "},
  {"Absolutely.     "},
  {"As I see it, yes"},
  {"Most likely.    "},
  {"Do bears poop in the woods?"},
  {"Yep.            "},
  {"Yes.            "},
  {"Because its you, yes"},
  {"Affirmative.    "},
  {"Obviously yes.  "},

  {"Im busy. Go away"},
  {"I defer to dad. "},
  {"Ask again later."},
  {"Better not say. "},
  {"Cannot predict. "},
  {"Silly question, Next!"},
  {"Not in the mood "},
  {"Who cares.      "},
  {"Euuu...what was the quesiton agian?"},
  {"Im too tired for this"},
  {"Oh look there!  "},
  {"Meh             "},
  {"What would dad say?"},
  {"Does it matter? "},
  
  {"Dont count on it"},
  {"My reply is no. "},
  {"Sources say no. "},
  {"Oh God, no      "},
  {"Nein!           "},
  {"Nope.           "},
  {"Simon says no.  "},
  {"Hermoine says no"},
  {"Universe says no"},
  {"Request rejected"},
  {"I would be lying if I said yes."},
  {"Do fishes fly?  "},
  {"Outlook not good"},
  {"Very doubtful.  "}
};

void setup() {
  
  Serial.begin(9600);
    
  // Activate the built-in pullup
  pinMode(TILT, INPUT_PULLUP);
  
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

  // Print a message to the LCD.
  lcd.setCursor(0, 0);
  lcd.print("Question?");   
}


void showText(const int row, const String text) {
  if(text.length() <= 16) {
     lcd.setCursor(0,row);
     lcd.print(text);
     return;
  }

  int i, j=0;
  for (i =0; i<text.length(); i++) {
      if(i<15) {
          lcd.setCursor(i,row);
          lcd.print(text.charAt(i));
          delay(150);
      } else {
           for(i=16;i<text.length();i++) {   
               j++;
               lcd.setCursor(0,row);
               lcd.print(text.substring(j,j+16));
             delay(250);
           }
       }
  }
}

void clear(){
   lcd.setCursor(0, 0);
   lcd.print("                                ");
   lcd.setCursor(0, 1);
   lcd.print("                                ");

}
void showAnswer(){
    clear();
    // Random choice
    randomSeed(analogRead(0)*analogRead(1)); 
    int pickedNum = random (0, 29);
    
    msgType msgToShow;
    memcpy_P (&msgToShow, &responses [pickedNum], sizeof msgToShow);
    
    const char* selectedResponse = msgToShow.description;
    // Delay a bit before outputting answer
    delay(500);

    // Output ready
    lcd.setCursor(0, 0);
    lcd.print("Answer:");

    showText(1, selectedResponse);
    Serial.println(selectedResponse);
}

void thinking() {
    lcd.setCursor(0, 0);
    lcd.print("Listening to the");    
    lcd.setCursor(0, 1);
    lcd.print("universe...     ");
}

void askQuestion(){
    lcd.setCursor(0, 0);
    lcd.print("Question?       ");
}

void loop()
{
  // Are we tilted?
  tiltReading = digitalRead(TILT);

  // Well, the pin changed so lets keep track when it happened
  if (tiltReading != prevReading) {
    // reset the debouncing timer
    lastTime = millis();
  } 

  // Has it happened long enough ago
  if ((millis() - lastTime) > debounce) {
     // Yep, seems so, so its pretty stable, lets trust it
     // But keep track of the previous trusted state.
     prevState = switchState;
     switchState = tiltReading;
   } else {
     // Its still jiggling about, dont change
   }

  // Save the last tilt reading
  prevReading = tiltReading;  
 
  // Only answer if our new stable state is tilted
  // and its different from the previos state
  if (switchState && (switchState != prevState)) {
    thinking();
    delay(2000);
    showAnswer();
    delay(4000);
    clear();
  }

  askQuestion();
}
