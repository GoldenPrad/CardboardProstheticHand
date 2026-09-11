// Diagnostic sketch — not part of the normal build.
//
// Drives ONE servo at a time through its full travel and announces which one
// it is on the serial monitor (9600 baud). Use this to find out whether a
// dead finger is a wiring problem or a power problem:
//
//   - A servo that stays dead here is a wiring/servo fault on that channel.
//   - Servos that work here but die in prosthesis.ino means the supply cannot
//     hold up under several servos moving at once.
//
// Open Tools > Serial Monitor at 9600 baud and watch which name is printed as
// each servo moves.

#include <Servo.h>

const int PINS[6] = {3, 5, 7, 9, 11, 12};
const char *NAMES[6] = {"Thumb", "Index", "Middle", "Web", "Ring", "Pinky"};

const int ANGLE_UP = 0;
const int ANGLE_DOWN = 90;

Servo servos[6];

void setup()
{
  Serial.begin(9600);
  while (!Serial)
  {
  } // wait for the monitor on boards that need it

  // NOTE: servos are deliberately NOT attached here. Attaching holds the servo
  // under torque, which draws current continuously. Attaching all six would put
  // a constant load on the supply and mask the very problem we are hunting.
  // Each servo is attached only while it is being tested, then detached.

  Serial.println();
  Serial.println("=== BOOT ===  Servo sweep test, one servo at a time.");
  Serial.println("If you see '=== BOOT ===' more than once, the board is");
  Serial.println("RESETTING. That is a power problem, not a wiring problem.");
  Serial.println();
  delay(1000);
}

void loop()
{
  for (int i = 0; i < 6; i++)
  {
    Serial.print("Pin ");
    Serial.print(PINS[i]);
    Serial.print("  ");
    Serial.print(NAMES[i]);
    Serial.println("  -> moving");
    Serial.flush(); // push the text out BEFORE drawing current, so the log
                    // survives even if this servo browns out the board

    servos[i].attach(PINS[i]);
    servos[i].write(ANGLE_DOWN);
    delay(500);
    servos[i].write(ANGLE_UP);
    delay(700);
    servos[i].write(ANGLE_DOWN);
    delay(700);
    servos[i].detach(); // release it so it stops drawing holding current

    Serial.print("Pin ");
    Serial.print(PINS[i]);
    Serial.println("  -> done");
    Serial.flush();

    delay(800); // pause so the supply can recover before the next one
  }

  Serial.println("--- sweep complete, repeating ---");
  Serial.println();
  delay(1500);
}
