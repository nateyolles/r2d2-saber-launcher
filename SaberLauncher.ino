/**
 * R2-D2 Lightsaber Launcher
 *
 * This code is for the Warp Core Saber Launcher designed by Warpcell on the
 * Astromech forums.
 *
 * By Nate Yolles: https://www.github.com/nateyolles/r2-d2-saber-launcher
 * 
 * See: Warp Core Base Plate: https://astromech.net/forums/showthread.php?40578-Warp-Core-Base-Plate-v2-0-BC-Approved-Continuous-250-(Dec-2020)-Open
 * See: Saber Launcher details: https://astromech.net/forums/showthread.php?40598-Warp-Core-Dome-System-FAQ
 * See: Actuonix "L16-R Miniature Linear Servos for RC & Arduino 100mm 35:1 6 volts" found at: https://www.actuonix.com/l16-100-35-6-r
 */

#include <Servo.h>

/**
 * The locking/launching servo is a Turnigy TGY-50090M Metal Gear 9g Analog Servo.
 * The open and close positions can be found with a servo tester and will be
 * unique to your particular launcher build and assembly, especially since there
 * isn't a standard servo linkage arm.
 */
const int TRIGGER_PIN = 9;
const int TRIGGER_CLOSED = 1200;
const int TRIGGER_OPEN = 1850;

/**
 * Time in milliseconds to stay open before closing. Give yourself enough time to
 * start the process and press the launcher down.
 */
const int LOAD_TIME = 4000;

/**
 * The max value when using the command saber:aim-step:x. The step values are 1
 * through STEP_VALUE_MAX. The step value maps to the range of the linear servo
 * range in microseconds. Steps allow for predefined stops at an even
 * distribution through the full range. The higher the STEP_VALUE_MAX, the more
 * steps available at smaller increments of movement. Steps aren't as precise as
 * micro adjustments, but maybe a little easier.
 */
const int STEP_VALUE_MAX = 10;

/**
 * The linear servo goes from fully contracted at 1000 to fully expanded at 2000. 
 * The servo will not reach the full limits when attached to the saber launcher.
 * The limits are probably not as variable as the locking/launching servo since there
 * isn't a linkage arm, but you'll need to find the limits in your build with a
 * servo tester. The servo fully contracted at 1000 places the launcher in its
 * most horizontal position while the upper limit places the launcher in its most
 * vertical position.
 */
const int AIM_PIN = 6;
const int AIM_UP_LIMIT = 1340;
const int AIM_DOWN_LIMIT = 1000;

/**
 * The ACTION constants create the languange to communicate to the saber launcher
 * from other devices via serial communication.
 *
 *  load: opens the lock for the LOAD_TIME in milliseconds and then closes the lock
 *        presumably with the launch pad in the lock arming the launcher.
 *
 *  launch: opens the lock to launch the lightsaber.
 *
 *  lock: close the lock if needed outside of load and launch.
 *
 *  aim-up/aim-down: aim the launcher to the full up/down positions.
 *
 *  aim-step: aim the launcher at intervals between 1 and STEP_VALUE_MAX. (e.g. 1,2,3...10)
 *            For example, saber:aim-step:7
 *
 *  aim-micro: aim the launcher at precise microseconds between AIM_DOWN_LIMIT and
 *             AIM_UP_LIMIT. For example: saber:aim-micro:1240
 */
const String ACTION_LOAD = "saber:load";
const String ACTION_LAUNCH = "saber:launch";
const String ACTION_LOCK = "saber:lock";
const String ACTION_AIM_UP = "saber:aim-up";
const String ACTION_AIM_DOWN = "saber:aim-down";
const String ACTION_AIM_STEP = "saber:aim-step:";
const String ACTION_AIM_MICRO = "saber:aim-micro:";

// Create the servos
Servo trigger;
Servo aim;

// Create a string to hold the incoming data
String inputString = "";
bool stringComplete = false;

// Track the status of the servos
bool lockOpen = true;
int aimAngle = AIM_UP_LIMIT;

void setup() {
  trigger.attach(TRIGGER_PIN);
  trigger.writeMicroseconds(TRIGGER_OPEN);

  aim.attach(AIM_PIN);
  aim.writeMicroseconds(AIM_UP_LIMIT);
  
  Serial.begin(9600);
  inputString.reserve(200);
}

void loop() {
  // print the string when a newline arrives:
  if (stringComplete) {
    inputString.trim();

    if (inputString.equals(ACTION_LOAD)) {
      load();
    } else if (inputString.equals(ACTION_LAUNCH)) {
      launch();
    } else if (inputString.equals(ACTION_LOCK)) {
      lock();
    } else if (inputString.equals(ACTION_AIM_UP)) {
      aimUpFull();
    } else if (inputString.equals(ACTION_AIM_DOWN)) {
      aimDownFull();
    } else if (inputString.startsWith(ACTION_AIM_MICRO)) {
      String microValueStr = inputString.substring(ACTION_AIM_MICRO.length());
      aimMicro(microValueStr.toInt());
    } else if (inputString.startsWith(ACTION_AIM_STEP)) {
      String stepValueStr = inputString.substring(ACTION_AIM_STEP.length());
      int microValue = convertStepToMicro(stepValueStr.toInt());
      aimMicro(microValue);
    };

    // Reset for the next serial command
    inputString = "";
    stringComplete = false;
  }
}

/**
 * Load the launcher by opening the lock for the LOAD_TIME and then closing
 * the lock, presumably with the launch pad in place.
 */
void load() {
  Serial.println("Intiating loading sequence");
  trigger.writeMicroseconds(TRIGGER_OPEN);
  lockOpen = true;
  delay(LOAD_TIME);
  trigger.writeMicroseconds(TRIGGER_CLOSED);
  Serial.println("Saber loaded");
  lockOpen = false;
}

/**
 * Launch the saber by opening the lock.
 */
void launch() {
  if (!lockOpen) {
    Serial.println("Saber launched");
    trigger.writeMicroseconds(TRIGGER_OPEN);
    lockOpen = true;
  } else {
    Serial.println("Can't launch: lock already open");
  }
}

/**
 * Close the lock.
 */
void lock() {
  if (lockOpen) {
    Serial.println("Saber locked");
    trigger.writeMicroseconds(TRIGGER_CLOSED);
    lockOpen = false;
  } else {
    Serial.println("Can't lock: already locked");
  }
}

/**
 * Aim the launcher to its full upright position.
 */
void aimUpFull() {
  if (aimAngle != AIM_UP_LIMIT) {
    aim.writeMicroseconds(AIM_UP_LIMIT);
    Serial.println("Aiming full up");
    aimAngle = AIM_UP_LIMIT;
  } else {
    Serial.println("Can't aim full up: already aimed full up");
  }
}

/**
 * Aim the launcher to its full outright position.
 */
void aimDownFull() {
  if (aimAngle != AIM_DOWN_LIMIT) {
    aim.writeMicroseconds(AIM_DOWN_LIMIT);
    Serial.println("Aiming full down");
    aimAngle = AIM_DOWN_LIMIT;
  } else {
    Serial.println("Can't aim full down: already aimed full down");
  }
}

/**
 * Aim the launcher by microseconds.
 */
void aimMicro(int microValue) {
  if (microValue >= AIM_DOWN_LIMIT && microValue <= AIM_UP_LIMIT) {
    aim.writeMicroseconds(microValue);  
  } else {
    Serial.println("Invalid aiming value");
  }
}

/**
 * Convert steps to microseconds as an optional matter of convenience.
 */
int convertStepToMicro(int stepValue) {
  if (stepValue >= 1 && stepValue <= STEP_VALUE_MAX) {
    return map(stepValue, 1, STEP_VALUE_MAX, AIM_DOWN_LIMIT, AIM_UP_LIMIT);
  } else {
    Serial.println("Invalid step value");
    return 0;
  }
}

/**
 * SerialEvent occurs whenever a new data comes in the hardware serial RX. This
 * routine is run between each time loop() runs, so using delay inside loop can
 * delay response. Multiple bytes of data may be available.
 *
 * See: https://docs.arduino.cc/built-in-examples/communication/SerialEvent/
 */
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}
