#include <Servo.h>

// ---- Finger servos: Thumb, Index, Middle, Ring, Pinky ----
const int PINS[5] = {3, 5, 7, 11, 12};
Servo servos[5];

// ---- Web-shooter servo ----
Servo webShooter;
const int WEB_PIN = 9;
const int WEB_HOLD = 0;     // angle that holds the rubber band
const int WEB_RELEASE = 90; // angle that lets it fly

// ---- Finger angles (tune once built) ----
const int ANGLE_UP = 0;    // finger extended
const int ANGLE_DOWN = 90; // finger curled (pulls the string)

String inputData = "";

// Remembers the last state we applied for each of the 6 characters.
// Initialised to 'x' so every servo gets written once on the first packet.
char lastState[6] = {'x', 'x', 'x', 'x', 'x', 'x'};

void setup()
{
  Serial.begin(9600);
  for (int i = 0; i < 5; i++)
    servos[i].attach(PINS[i]);
  webShooter.attach(WEB_PIN);
  webShooter.write(WEB_HOLD);
}

void loop()
{
  if (Serial.available())
  {
    inputData = Serial.readStringUntil('\n');
    inputData.trim();

    if (inputData.length() == 6)
    {

      // --- Fingers: only move a servo whose character changed ---
      for (int i = 0; i < 5; i++)
      {
        if (inputData[i] != lastState[i])
        {
          int angle = (inputData[i] == '1') ? ANGLE_UP : ANGLE_DOWN;
          servos[i].write(angle);
          lastState[i] = inputData[i];
        }
      }

      // --- Web shooter: only act when the trigger bit changes ---
      // 0 -> 1 releases the band; 1 -> 0 re-arms it once fingers reopen.
      if (inputData[5] != lastState[5])
      {
        webShooter.write(inputData[5] == '1' ? WEB_RELEASE : WEB_HOLD);
        lastState[5] = inputData[5];
      }
    }
  }
}
