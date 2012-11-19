// Autonomouse
// TODO: disable interrupts
// TODO: scroll test isn't fully baked yet and needs work

// pin settings
const int photocellPin = 5; // analog pin

// general test vars
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

// commands for controlling the device via serial USB
static const char CLICKTEST = 'a';
static const char SCROLLTEST = 'b';
static const char CALIBRATE = 'd';
static const char PRINTRESULTS = 'e';
static const char MOVEMOUSEUP = '1';
static const char MOVEMOUSEDOWN = '2';
static const char MOVEMOUSELEFT = '3';
static const char MOVEMOUSERIGHT = '4';

// click test vars
const int clickTestRuns = 5;
unsigned clickTimes[clickTestRuns];
const int clickTestDelay = 503; // delay between tests. Make sure this won't sync on vsync! (aka not a multiple of 16.67ms)

// scroll test vars
const int scrollDelay = 19; // ms, time between mouse events
const int scrollBufferSize = 150; // number of scroll events to record
unsigned scrollTimes[scrollBufferSize];

// photo state vars
static int PHOTO_ON = 1;
static int PHOTO_OFF = -1;
static int PHOTO_IDK = 0;

void setup(void) {
  Serial.begin(115200);
  Serial1.begin(115200);
}

void loop(void) {
  Serial.println("Autonomouse");
  Serial.print(CLICKTEST); Serial.println(": click test.");
  Serial.print(SCROLLTEST); Serial.println(": scroll test.");
  Serial.print(CALIBRATE); Serial.println(": calibrate sensor.");
  Serial.print(PRINTRESULTS); Serial.println(": print results.");
  Serial.print(MOVEMOUSEUP); Serial.print("-"); Serial.print(MOVEMOUSERIGHT); Serial.println(": move mouse.");

  while(true) {
    delay(100); // Don't waste power in menus
    if (thereIsUserInput()) {
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
        case (CALIBRATE):
          calibrate();
          return;
        case (PRINTRESULTS):
          showResults();
          return;
        case (MOVEMOUSEUP): moveUp(); break;
        case (MOVEMOUSEDOWN): moveDown(); break;
        case (MOVEMOUSELEFT): moveLeft(); break;
        case (MOVEMOUSERIGHT): moveRight(); break;
        default:
          Serial.print("unknown command:");
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

  // capture the initial state, a click should register the opposite state
  // (for testing black->white clicks and white->black)
  int initialPhotoState = photoState();

  // use Knuth's online variance algorithm
  // (see http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance#Online_algorithm)
  double mean = 0;
  double m2 = 0;
  double delta;

  for (int test = 0; test < clickTestRuns; test++) {
    delay(clickTestDelay);

    if (thereIsUserInput()) {
      Serial.println("Stopping test.");
      return;
    }

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

    delta = time - mean;
    mean += + delta / (test + 1);
    m2 += + delta * (time - mean);

    Serial.print("  run ("); Serial.print(test, DEC); Serial.print("/"); Serial.print(clickTestRuns, DEC);
    Serial.print("): ");Serial.print(time, DEC); Serial.println("ms");
  }

  double stddev = sqrt(m2 / (clickTestRuns - 1));
  Serial.print("mean: "); Serial.print(mean); Serial.println("ms");
  Serial.print("stddev: "); Serial.println(stddev);
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
  while(!thereIsUserInput()) {
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

// Move mouse commands
// useful for positioning the mouse over our click region.
void moveDown() { Serial1.write(mouseMoveDown, 7); }
void moveUp() { Serial1.write(mouseMoveUp, 7); }
void moveLeft() { Serial1.write(mouseMoveLeft, 7); }
void moveRight() { Serial1.write(mouseMoveRight, 7); }

// Return true if the user input anything
boolean thereIsUserInput() {
  return (Serial.available() > 0);
}
