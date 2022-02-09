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

// Magic 8 Ball strings
String responses[] = {
  "Are you a girl? ",
  "Are you a boy?  ",
  "Do you like aubergines?",

  "It is certain.  ",
  "Decidedly so.   ",
  "Without a doubt.",
  "Definitely.     ",
  "Rely on it.     ",
  "As I see it, yes",
  "Most likely.    ",
  "Do bears poop in the woods?",
  "Yes.            ",
  
  "Im busy. Go away",
  "I defer to dad. ",
  "Ask again later.",
  "Better not say. ",
  "Cannot predict. ",
  "Not in the mood ",
  "Who cares.      ",
  "Errr, say what? ",
  "Does it matter? ",
  
  "Dont count on it",
  "My reply is no. ",
  "Sources say no. ",
  "Nein!           ",
  "Nope.           ",
  "Simon says no.  ",
  "Universe says no",
  "Outlook not good",
  "Very doubtful.  "
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
    randomSeed(analogRead(0)); 
    int pickedNum = random (0, 29);
    String selectedResponse = responses[pickedNum];
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
    delay(5000);
    clear();
  }

  askQuestion();
}
