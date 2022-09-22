#include <PS2Keyboard.h>

const int DataPin = 2;
const int IRQpin =  3;

PS2Keyboard keyboard;

bool X = false;               // constant to the motor designation
bool Y = true;                // constant to the motor designation
bool FAST = true;             // constant to the motor speed designation
bool SLOW = false;            // constant to the motor speed designation
bool ESC = true;              // constant to escape moving
bool OK = false;              // constant to continue moving

bool isHomePushed[] = {false, false};
byte pinHome[] = {15, 16};            // analog endcaps pins
byte PUL[] = {9, 10};                 // digital motors pulse control pins
byte DIR[] = {11, 12};                // digital motors dir control pins
int timeOut[][2] = {{3, 1}, {3, 1}};  // {{xSlow, xFast}, {ySlow, yFast}} timeouts of fast and slow moving at axis
int factor[] = {4*4, 4*20};           // number of pulses in one tenth = reducer factor * driver factor.
int step[] = {44, 3};                 // number of tenths between slots, number of tenths to a dive step
int home[] = {320, 10};               // values of home locations - then endcups get pressing
int border[] = {5040, 140};           // extremely far locations
int widthCutter = 20;                 // the cutter width
volatile int location[] = {300, 142}; // current location values on start

int homeSlotPivot = 500;
int numberOfSlots = 92;

void setup() {
  delay(1000);
  keyboard.begin(DataPin, IRQpin);
  Serial.begin(9600);
  Serial.println("Keyboard Test:");
  
  // init endcaps pins
  pinMode(15, INPUT_PULLUP);
  pinMode(16, INPUT_PULLUP);
  
  // init motors control pins
  pinMode(9, OUTPUT);           
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);
}

void loop() {
  if (keyboard.available()) {
    char c = keyboard.read();
    
    // test to some keys are pressed
    if      (c == PS2_INSERT)     go(X, FAST, location[X] - step[X]);
    else if (c == PS2_DELETE)     go(X, SLOW, location[X] - step[X]);
    else if (c == PS2_HOME)       go(Y, SLOW, border[Y]);
    else if (c == PS2_END)        go(Y, FAST, home[Y]);
    else if (c == PS2_PAGEUP)     go(X, FAST, location[X] + step[X]);
    else if (c == PS2_PAGEDOWN)   go(X, SLOW, location[X] + step[X]);
    else if (c == PS2_F9)         go(X, FAST, home[X]);
    else if (c == PS2_F10)        go(X, SLOW, home[X]);
    else if (c == PS2_F11)        go(X, SLOW, border[X]);
    else if (c == PS2_F12)        go(X, FAST, border[X]);
    else if (c == PS2_UPARROW)    go(Y, FAST, location[Y] + factor[Y]);
    else if (c == PS2_LEFTARROW)  go(X, FAST, location[X] - factor[X]);
    else if (c == PS2_RIGHTARROW) go(X, FAST, location[X] + factor[X]);
    else if (c == PS2_DOWNARROW)  go(Y, FAST, location[Y] - factor[Y]);
    else if (c == 'r')            resetPivotSlotHere(); // change the home slot pivot so that the current location becomes the current slot pivot
    else if (c == 'a')            addLastSlot();        // increment number of slots
    else if (c == 'x')            resetHome(X);
    else if (c == 'y')            resetHome(Y);
    else if (c == 's')            makeSlotsAll();
    else if (c == 'c')            makeTail();
    else if (c == 't')            testShift();
//    else if (c == PS2_ESC)        wait();
//    else if (c == '+')            resque();
    else Serial.println("A command for the button " + String(c) + " is not founded");
  }
}

void testShift() {
  for (int i = 0; i < 100; i++) {
    if (go(X, FAST, location[X] + 15)) return;
    if (go(Y, FAST, location[Y] + 15)) return;
    if (go(X, FAST, location[X] - 15)) return;
    if (go(Y, FAST, location[Y] - 15)) return;
  }
}

bool wait() {
  char c = '0';
  do {            // wait Enter or Esc or '+', right here
    delay(500);
    if (keyboard.available()) 
      c = keyboard.read();
  } while (c != PS2_ENTER && c != PS2_ESC && c != '+');
  
  if (c == PS2_ESC)
    return ESC;
  
  if (c == '+')
    if (resque())
      return ESC;
  
  return OK;
}

bool resque() {
  int marker = location[X];
  if (resetHome(Y))
    return ESC;
  
  char c = '0';
  do {            // wait Enter or Esc, right here
    delay(500);
    if (keyboard.available()) 
      c = keyboard.read();
  } while (c != PS2_ENTER && c != PS2_ESC);
  
  if (c == PS2_ESC)
    return ESC;
  
  if (go(Y, SLOW, marker))
    return ESC;
  
  return OK;
}


// push a tenth and align the axis location value
bool goTenth(bool axis, bool isFast, bool isRise) {
  char c = keyboard.read();
  
  // on Esc - return ESC
  if (c == PS2_ESC)
    return ESC;
  
  // on Enter - call pause
  if (c == PS2_ENTER) // call wait()
    if (wait())       // on ESC from wait() - return ESC
      return ESC;     // escalate if ESC

  if (c == '+')       // call resque()
    if (resque())     // eject the cutter to home[Y], will return to the task if Enter will been folowed
      return ESC;     // on ESC from wait() - return ESC

  // move forward by one tenth
  for (int i = 0; i < factor[axis]; i++)
  {
    digitalWrite(DIR[axis], isRise ? HIGH : LOW);
    digitalWrite(PUL[axis], HIGH);
    digitalWrite(PUL[axis], LOW);
    delay(timeOut[axis, isFast]);
  }

  // align the axis location value
  location[axis] += (isRise ? 1 : -1);
  return OK;
}

bool go(bool axis, bool isFast, int target) {
  bool isRise = (target > location[axis]);
  int distance = abs(target - location[axis]);
  
  for (int i = 0; i < distance; i++)
    if (goTenth(axis, isFast, isRise))          // call goTenth()
      return ESC;                               // escalate if ESC
  
  return OK;
}

bool isHomePressed(bool axis) {
  return analogRead(pinHome[axis]) > 1000 ? true : false; // the maximum value is 1023
}

bool resetHome(bool axis) {
  // if it's an executing of reset X, then reset Y at first
  if (axis == X)
    resetHome(Y);

  // cancel the rehoming if we are already at home
  if (location[axis] == home[axis])
    return OK;

  // move by tenths - from rear - until the endcup turn off - if the support is outside the working area
  while (isHomePressed(axis))
    if (go(axis, FAST, location[axis] + 1))
      return ESC;

  // move by tenths - to home - until the endcup turn on - if the support is into the working area
  while (!isHomePressed(axis))
    if (go(axis, FAST, location[axis] - 1))
      return ESC;

  // slowly rollout to the home location - until the endcup turn off
  digitalWrite(DIR[axis], HIGH);        // direction is to the right
  while (isHomePressed(axis)) {         // move by pulses - to the home - until the endcup became on. ESC is useless only at this move
    digitalWrite(PUL[axis], HIGH);
    digitalWrite(PUL[axis], LOW);
    delay(timeOut[axis][SLOW]);
  }

  // reset current location to the axis home value
  location[axis] = home[axis];
  return OK;
}

bool makeSlotEdge(int target) {
  resetHome(Y);
  if (go(X, FAST, target))
    return ESC;
  if (go(Y, SLOW, border[Y]))
    return ESC;
  delay(1000);
  if (resetHome(Y))
    return ESC;
  return OK;
}

bool makeSlotWide(int left, int right) {
  if (resetHome(Y))
    return ESC;
  if (go(X, FAST, left))
    return ESC;

  while ((location[Y] < border[Y] - step[Y])) {
    if (go(Y, SLOW, location[Y] + step[Y]))
      return ESC;
    if (go(X, SLOW, right))
      return ESC;
    if (go(Y, SLOW, location[Y] + step[Y]))
      return ESC;
    if (go(X, SLOW, left))
      return ESC;
  }
  delay(1000);
  if (go(X, SLOW, right))
      return ESC;
  delay(1000);
  if (resetHome(Y))
      return ESC;
  return OK;
}

bool makeSlotsAll() {
  // if the home slot pivot is to the right - move to the home slot pivot
  if (location[X] < homeSlotPivot)
    if (go(X, FAST, homeSlotPivot))
      return ESC;
    
  // let's sloting...
  int curentSlotIndex = (location[X] - homeSlotPivot) / step[X];
  for (int i = curentSlotIndex; i < numberOfSlots; i++) {
    int currentSlotPivot = homeSlotPivot + i * step[X];
    if (makeSlotWide(currentSlotPivot + 5, currentSlotPivot + 18))
      return ESC;
    if (makeSlotEdge(currentSlotPivot))
      return ESC;
    
    // if this is not the home slot, - cut the left edge of the ring to the right
    if (i > 0)
      if (makeSlotEdge(currentSlotPivot - 25))
      return ESC;

    // go 5 tenths to the left from the next slot pivot
    if (go(X, FAST, currentSlotPivot + step[X] + 5))
      return ESC;
  }
  return OK;
}

bool makeTail() {
  // take current X location as pivot of cornering
  int pivot = location[X];
  
  // draft cutting
  if (makeSlotWide(pivot + 5, border[X]))
    return ESC;

  // move to pivot
  if (go(X, FAST, pivot))
    return ESC;
  
  // clean cutting
  if (go(Y, SLOW, border[Y] + 2))
    return ESC;
  if (go(X, SLOW, border[X]))
    return ESC;

  // move to the right edge of the last ring
  if (resetHome(Y))
    return ESC;
  if (go(X, FAST, pivot - 30))
    return ESC;

  return OK;
}

bool resetPivotSlotHere() {
  // this function is colled then sloting gone wrong
  // it resets location X and resets the homeSlotPivot so that the current X location becomes the left edge of the next slot
  // it don't call sloting!!!
  
  // reset X, markering the distance to the home[X]
  int marker = 0;       
  while (!isHomePressed[X]) {
    if (go(X, FAST, location[X] - 1))
      return ESC;
    marker++;
  }
  if (resetHome[X])
    return ESC;

  // return support to the initial location
  if (go(X, FAST, marker))
    return ESC;
  
  // reset the home slot pivot to reinstate slotting from the current location as the left edge of the new slot
  // compute the shift of the current position relative to the left edge of the slot we are holding
  int shift = (location[X] - homeSlotPivot) / step[X];
  
  // if the shift is > 20 tenths - move the slotting to the left, otherwise - to the right
  homeSlotPivot += (shift > 20) ? (shift - step[X]) : shift;

  return OK;
}

void addLastSlot() {
  Serial.println("reorder number of slots from " + String(numberOfSlots) + " to " + String(++numberOfSlots));
}

//----------------EOF------------------------
