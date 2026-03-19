#include "OneButton.h"


#define PIN_INPUT1 0
#define PIN_INPUT2 5
#define PIN_INPUT3 4


// Setup a new OneButton on pin PIN_INPUT1.
OneButton button1(PIN_INPUT1, true);
// Setup a new OneButton on pin PIN_INPUT2.
OneButton button2(PIN_INPUT2, true);
// Setup a new OneButton on pin PIN_INPUT3.
OneButton button3(PIN_INPUT3, true);

// setup code here, to run once:
void setup() {
  // Setup the Serial port. see http://arduino.cc/en/Serial/IfSerial
  Serial.begin(115200);
  while (!Serial) {
    ;  // wait for serial port to connect. Needed for Leonardo only
  }
  // link the button 1 functions.
  button1.attachClick(click1);
  button1.attachDoubleClick(doubleclick1);
  button1.attachLongPressStart(longPressStart1);
  button1.attachLongPressStop(longPressStop1);
  button1.attachDuringLongPress(longPress1);

  // link the button 2 functions.
  button2.attachClick(click2);
  button2.attachDoubleClick(doubleclick2);
  button2.attachLongPressStart(longPressStart2);
  button2.attachLongPressStop(longPressStop2);
  button2.attachDuringLongPress(longPress2);

  // link the button 3 functions.
  button3.attachClick(click3);
  button3.attachDoubleClick(doubleclick3);
  button3.attachLongPressStart(longPressStart3);
  button3.attachLongPressStop(longPressStop3);
  button3.attachDuringLongPress(longPress3);

}  // setup


// main code here, to run repeatedly:
void loop() {
  // keep watching the push buttons:
  button1.tick();
  button2.tick();
  button3.tick();

  // You can implement other code in here or just wait a while
  delay(10);
}  // loop


// ----- button 1 callback functions

// This function will be called when the button1 was pressed 1 time (and no 2. button press followed).
void click1() {
  Serial.println("Button 1 click.");
}  // click1


// This function will be called when the button1 was pressed 2 times in a short timeframe.
void doubleclick1() {
  Serial.println("Button 1 doubleclick.");
}  // doubleclick1


// This function will be called once, when the button1 is pressed for a long time.
void longPressStart1() {
  Serial.println("Button 1 longPress start");
}  // longPressStart1


// This function will be called often, while the button1 is pressed for a long time.
void longPress1() {
  Serial.println("Button 1 longPress...");
}  // longPress1


// This function will be called once, when the button1 is released after beeing pressed for a long time.
void longPressStop1() {
  Serial.println("Button 1 longPress stop");
}  // longPressStop1


// ... and the same for button 2:

void click2() {
  Serial.println("Button 2 click.");
}  // click2


void doubleclick2() {
  Serial.println("Button 2 doubleclick.");
}  // doubleclick2


void longPressStart2() {
  Serial.println("Button 2 longPress start");
}  // longPressStart2


void longPress2() {
  Serial.println("Button 2 longPress...");
}  // longPress2

void longPressStop2() {
  Serial.println("Button 2 longPress stop");
}  // longPressStop2


// ... and the same for button 3:

void click3() {
  Serial.println("Button 3 click.");
}  // click3


void doubleclick3() {
  Serial.println("Button 3 doubleclick.");
}  // doubleclick3


void longPressStart3() {
  Serial.println("Button 3 longPress start");
}  // longPressStart3


void longPress3() {
  Serial.println("Button 3 longPress...");
}  // longPress3

void longPressStop3() {
  Serial.println("Button 3 longPress stop");
}  // longPressStop3


// End
