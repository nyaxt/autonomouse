// Lag Buster
// pdr@google
// TODO: disable interrupts

static int PHOTO_ON = 1;
static int PHOTO_OFF = -1;
static int PHOTO_IDK = 0;

// pin settings
const int photocellPin = 5; // analog pin

// general test variables
const unsigned long timeout = 1000000; // microseconds, test timeout
// BlueSmirf's HID mouse commands. See http://goo.gl/FBpHs for sparse details.
// Note: RN42 chip needs to be put into mouse HID mode with "SH,0220".
const byte mouseDown[] = {0xFD, 0x05, 0x02, 0x01, 0x00, 0x00, 0x00};
const byte mouseUp[] = {0xFD, 0x05, 0x02, 0x00, 0x00, 0x00, 0x00};
const byte mouseDownAndMoveDown[] = {0xFD, 0x05, 0x02, 0x01, 0x00, 0x04, 0x00};
const byte mouseMoveUp[] = {0xFD, 0x05, 0x02, 0x00, 0x00, 0xF0, 0x00};
const byte mouseMoveDown[] = {0xFD, 0x05, 0x02, 0x00, 0x00, 0x10, 0x00};
const byte mouseMoveLeft[] = {0xFD, 0x05, 0x02, 0x00, 0xF0, 0x00, 0x00};
const byte mouseMoveRight[] = {0xFD, 0x05, 0x02, 0x00, 0x10, 0x00, 0x00};
static const char CLICKTEST = 'a';
static const char SCROLLTEST = 'b';
static const char RESETBT = 'c';
static const char CALIBRATE = 'd';
static const char RESULTS = 'e';

// click test variables
const int clickTestRuns = 150;
unsigned clickTimes[clickTestRuns];

// scroll test variables
const int scrollDelay = 19; // ms, time between mouse events
const int scrollBufferSize = 150; // number of scroll events to record
unsigned scrollTimes[scrollBufferSize];

void setup(void) {
  Serial.begin(115200);
  Serial1.begin(115200);
}

void loop(void) {
  Serial.println("Lag Buster");
  Serial.println("a: click test.");
  Serial.println("b: scroll test.");
  // TESTING ONLY
  //Serial.println("c: reset bluetooth.");
  Serial.println("d: calibrate sensor.");
  // TESTING ONLY
  //Serial.println("e: show results.");

  while(true) {
    delay(10); // Don't waste power in menus
    if (Serial.available() > 0) {
      char val = Serial.read();
      switch (val) {
        case (CLICKTEST):
          delay(5000); // TESTING ONLY
          clickTest();
          return;
        case (SCROLLTEST):
          delay(5000); // TESTING ONLY
          scrollTest();
          return;
        case (RESETBT):
          Serial.write("resetting bluetooth\n");
          return;
        case (CALIBRATE):
          calibrate();
          return;
        case (RESULTS):
          showResults();
          return;
        case ('1'): moveUp(); break;
        case ('2'): moveDown(); break;
        case ('3'): moveLeft(); break;
        case ('4'): moveRight(); break;
        default:
          Serial.write("unknown command:");
          Serial.print(val);
          while (Serial.peek() != -1) {
            val = Serial.read();
            Serial.print(val);
          }
          Serial.write("\n");
          return;
      }
    }
  }
}

void clickTest() {
  Serial.println("Click test");

  // Zero previous results
  for (int test = 0; test < clickTestRuns; test++)
    clickTimes[test] = 0;

  unsigned long startTime;
  unsigned long endTime;
  int cumulative = 0;
  int initialPhotoState = photoState();
  for (int test = 0; test < clickTestRuns; test++) {
    delay(503); // delay between tests. Note: make sure this won't sync on vsync! (aka not a multiple of 16.67ms)

    startTime = micros();
    clickMouse();
    while(true) {
      endTime = micros();
      if (endTime - startTime > timeout || photoStateDifferent(initialPhotoState, photoState()))
        break;
    }
    releaseMouse();
    int time = (endTime - startTime) / 1000;
    clickTimes[test] = time;
    cumulative += time;
    Serial.print("  run ("); Serial.print(test, DEC); Serial.print("/"); Serial.print(clickTestRuns, DEC);
    Serial.print("): ");Serial.print(time, DEC); Serial.println("ms");
  }
  Serial.print("Average: "); Serial.print(cumulative / clickTestRuns, DEC); Serial.println("ms");
}


void scrollTest() {
  Serial.println("Scroll Test");

  // Zero previous results
  for (int i = 0; i < scrollBufferSize; i++)
    scrollTimes[i] = 0;

  unsigned long startTime;
  int lastPhotoState = photoState();
  int scrollEventCount = 0;
  int mouseTimer = 0; // we can only fire mouse events so fast so this is used for limiting the rate.
  startTime = micros();
  for (int time = 0; time < 15000; time++) {
    delay(1);
    if (++mouseTimer % scrollDelay == 0)
      clickAndMoveMouseDown();
    int currentPhotoState = photoState();
    if (photoStateDifferent(currentPhotoState, lastPhotoState)) {
      lastPhotoState = currentPhotoState;
      scrollTimes[scrollEventCount] = (micros() - startTime) / 1000;
      if(++scrollEventCount >= scrollBufferSize)
        break;

      Serial.print("  "); Serial.print(scrollEventCount, DEC); Serial.print(": ");
      Serial.print(scrollTimes[scrollEventCount - 1], DEC); Serial.println("ms");
    }
  }
  releaseMouse();
  Serial.print("Event count: "); Serial.println(scrollEventCount, DEC);
}

// Show test results via USB.
void showResults() {
  Serial.println("Click test times:");
  for (int i = 0; i < clickTestRuns; i++) {
    if (clickTimes[i] == 0)
      break;
    Serial.println(clickTimes[i]);
  }
  Serial.println("Scroll test times:");
  for (int i = 0; i < scrollBufferSize; i++) {
    if (scrollTimes[i] == 0)
      break;
    Serial.println(scrollTimes[i]);
  }
}

// Calibrate photo sensor
// Just print out values every half second.
void calibrate() {
  Serial.println("Printing photosensor values (press any button to exit)");
  while(!(Serial.available() > 0)) {
    int photoval = analogRead(photocellPin);
    Serial.print("Read ");
    Serial.print(photoval, DEC);
    Serial.print("\n");
    delay(500);
  }
  Serial.read();
}

// Read the photodiode state and return 1 (on), -1 (off), or 0 (unknown).
// The values were found to work well with the curcuit. The unknown state
// adds hysteresis so noise doesn't cause us to bounce between on and off.
int photoState() {
  int photoVoltage = analogRead(photocellPin);
  if (photoVoltage >= 500) return PHOTO_ON;
  if (photoVoltage <= 150) return PHOTO_OFF;
  return PHOTO_IDK;
}

// Return true if a and b have known, different states.
boolean photoStateDifferent(int a, int b) {
  return (a == PHOTO_ON && b == PHOTO_OFF) || (a == PHOTO_OFF && b == PHOTO_ON);
}

// Send mouse press command
void clickMouse() {
  Serial1.write(mouseDown, 7);
}

// Send mouse depress command
void releaseMouse() {
  Serial1.write(mouseUp, 7);
}

// Send mouse click and move down command
void clickAndMoveMouseDown() {
  Serial1.write(mouseDownAndMoveDown, 7);
}

void moveDown() { Serial1.write(mouseMoveDown, 7); }
void moveUp() { Serial1.write(mouseMoveUp, 7); }
void moveLeft() { Serial1.write(mouseMoveLeft, 7); }
void moveRight() { Serial1.write(mouseMoveRight, 7); }
