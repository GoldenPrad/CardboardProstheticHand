# Hand Tracker — Camera-Controlled Robotic Hand

A camera-controlled robotic hand. A webcam watches your real hand, and a cardboard hand
copies it in real time.

A Python script uses MediaPipe to track which fingers are open or closed, then sends a
6-character string over USB serial to an Arduino UNO (Elegoo). The Arduino drives five
servos that pull strings attached to the fingers of a cardboard hand. A sixth servo acts
as a "web shooter" — it releases a rubber band when the middle and ring fingers close
together (the Spider-Man gesture).

## Files

| File | What it is |
| --- | --- |
| `bridge.py` | Python side. Webcam → MediaPipe → serial packets. |
| `prosthesis/prosthesis.ino` | Arduino sketch. Serial packets → servo angles. |
| `tracker.html` | Standalone browser visualizer. Shows the hand skeleton and all 21 landmark coordinates. No Arduino needed — useful for debugging the finger detection on its own. |

## How it works

### The packet

Every frame, `bridge.py` builds a 6-character string and writes it to the serial port
followed by a newline:

```
1 0 0 1 1 0
│ │ │ │ │ └── web shooter: 1 = fire
│ │ │ │ └──── pinky
│ │ │ └────── ring
│ │ └──────── middle
│ └────────── index
└──────────── thumb
```

Each finger character is `1` for extended, `0` for curled. The web-shooter bit is `1`
whenever the middle **and** ring fingers are both curled.

The packet is only transmitted when it changes, so the serial line stays quiet while your
hand is still.

### Finger detection

MediaPipe returns 21 landmarks per hand. A finger counts as extended when its tip
landmark sits above its MCP knuckle (smaller `y`). The thumb is the exception — it moves
sideways, so it compares `x` instead, and the comparison flips depending on whether the
detected hand is Left or Right.

### The Arduino side

The Arduino sketch reads a line, checks it is exactly 6 characters, and writes servo angles.
It tracks the last state of each character and only moves a servo whose character actually
changed, which keeps the servos from buzzing.

Angle constants live at the top of the sketch and are meant to be tuned once the hand is
physically built:

```cpp
const int ANGLE_UP   = 0;   // finger extended
const int ANGLE_DOWN = 90;  // finger curled (pulls the string)
const int WEB_HOLD    = 0;  // holds the rubber band
const int WEB_RELEASE = 90; // lets it fly
```

## Hardware

- Arduino UNO (Elegoo)
- 6 × hobby servos (SG90 or similar) — five fingers plus the web shooter
- **A 5V supply rated at least 2A for the servos** — see the power section below, this is
  the single most common thing to get wrong
- Optional but recommended: a 470µF–1000µF electrolytic capacitor across the servo rails
- Cardboard hand with string running through each finger to a servo horn
- Rubber band for the web shooter
- Webcam
- USB cable

### Wiring

| Servo | Arduino pin |
| --- | --- |
| Thumb | D3 |
| Index | D5 |
| Middle | D7 |
| Web shooter | D9 |
| Ring | D11 |
| Pinky | D12 |

### Power — read this before blaming the code

Six servos need real current. An SG90 draws 500–700mA while pulling against a load, and
a stalled one draws more. Budget **2–3A at 5V**.

Wiring:

- Servo **signal** wires → the Arduino pins above
- Servo **V+** (red) → the `+` rail of a dedicated 5V supply
- Servo **GND** (brown/black) → the `−` rail of that supply
- **Jumper the `−` rail to an Arduino `GND` pin.** Without this common ground the servos
  twitch or do nothing.

Good supplies:

- A 5V phone charger rated 2A+, via a USB breakout to the rails
- 4× AA batteries wired straight to the rails (6V alkaline or 4.8V NiMH)
- A 5V 3A barrel-jack wall adapter

**Do not use these:**

| Supply | Why it fails |
| --- | --- |
| 9V battery (PP3) | ~1.5–3Ω internal resistance, ~100–200mA realistic output. The rail collapses the moment a servo moves. |
| Arduino `5V` pin | USB limits the whole board to ~500mA. One servo exceeds it. |
| 9V battery + breadboard regulator module | The module's linear regulator burns `(9−5)×I` watts as heat and thermally shuts down, on top of the battery already being too weak. |

**Never connect 9V directly to a servo's red wire.** These servos are rated 4.8–6V; 9V
destroys the motor and control IC.

Symptom to recognise: a servo that **buzzes or hums but does not turn** is receiving its
signal correctly and stalling for lack of current. That is always a power problem (or a
mechanically jammed horn), never a wiring or code problem.

## Install

Requires the **CH340 USB driver** for the board. Windows often will not enumerate an
Elegoo UNO as a COM port until CH340 is installed — if no port shows up in Device Manager,
that is the usual cause.

### Python 3.12 is required — this is not optional

`bridge.py` uses MediaPipe's legacy `mp.solutions.hands` API. That API **only ships in
MediaPipe wheels built for Python 3.9–3.12.** The wheels for Python 3.13 and 3.14 contain
only the newer Tasks API — `mediapipe/solutions/` is simply not in the package, so no
amount of version pinning will make it work there. On 3.14 you get:

```
AttributeError: module 'mediapipe' has no attribute 'solutions'
```

If you are on a newer Python, install 3.12 alongside it (they coexist fine):

```powershell
winget install --id Python.Python.3.12 --exact
```

### Create the virtual environment

Build the venv explicitly against 3.12 — on Windows use the `py` launcher so you don't
accidentally get your default interpreter:

```powershell
py -3.12 -m venv handtracker
.\handtracker\Scripts\Activate.ps1
pip install -r requirements.txt
```

macOS / Linux:

```bash
python3.12 -m venv handtracker
source handtracker/bin/activate
pip install -r requirements.txt
```

If PowerShell blocks activation with an execution-policy error, run this once:

```powershell
Set-ExecutionPolicy -Scope CurrentUser RemoteSigned
```

### Verify

```powershell
python -c "import sys, cv2, mediapipe as mp, serial; print(sys.version.split()[0], cv2.__version__, mp.__version__, serial.__version__); mp.solutions.hands.Hands(); print('solutions API OK')"
```

Expected: `3.12.10 4.11.0 0.10.21 3.5` followed by `solutions API OK`.

MediaPipe prints `INFO: Created TensorFlow Lite XNNPACK delegate for CPU` and a couple of
`W0000 ... inference_feedback_manager` warnings on startup. Those are normal and harmless.

## Upload the Arduino sketch

1. Open `prosthesis/prosthesis.ino` in the Arduino IDE. (The Arduino IDE requires the
   sketch to live in a folder of the same name.)
2. **Tools → Board → Arduino Uno**.
3. **Tools → Port →** your CH340 port (e.g. `COM3`).
4. Upload.

The sketch listens at **9600 baud**, which is what `bridge.py` uses.

## Run

With the venv active and the Arduino plugged in:

```bash
python bridge.py
```

`bridge.py` scans the serial ports and auto-connects to the first one whose description
contains `Arduino`, `CH340`, or `USB Serial`. On success it prints:

```
Arduino found on COM3
Camera running. Press Q to quit.
```

If no board is found it prints `No Arduino found - running in preview mode (no serial)`
and still opens the camera window — handy for testing gesture detection without hardware.

A window opens showing the camera feed with the hand skeleton drawn on it and a readout
along the top:

```
T:U I:U M:D R:D P:U  WEB:FIRE
```

`U` = up/extended, `D` = down/curled. Every packet sent is also echoed to the terminal.

Press **Q** to quit. The script releases the camera and closes the serial port on exit.

### Browser visualizer

Open `tracker.html` in a browser for a live view of the hand skeleton plus a table of all
21 landmark `x, y, z` values. It runs MediaPipe from a CDN, so it needs an internet
connection, and it does not talk to the Arduino. Useful when you want to see why a finger
is being read as up or down.

## Troubleshooting

**No Arduino found** — Install the CH340 driver, then check Device Manager for a COM port.
Close the Arduino IDE's Serial Monitor; it holds the port open and blocks Python from
using it.

**Servos buzz, jitter, or reset the board** — Almost always power. See the power section.
A 9V battery cannot run servos. Confirm the servos are on a 2A+ 5V supply and that its
ground is tied to Arduino GND.

**Only one servo works and the rest hum** — Same cause. The rail can supply just enough
for the least-loaded servo. Swapping servos between pins to confirm the fault follows the
servo rather than the pin is a useful check, but the underlying issue is the supply.

**Fingers move backwards** — Swap `ANGLE_UP` and `ANGLE_DOWN` in the sketch, or re-seat
the servo horn.

**`module 'mediapipe' has no attribute 'solutions'`** — Your venv is on Python 3.13/3.14.
Rebuild it on 3.12; see the install section above.

**Thumb detection is inverted** — The thumb test depends on the Left/Right handedness
label from MediaPipe. Try the other hand, and remember the preview is mirrored.

**Camera does not open** — Another app may be holding it. Or change the index in
`cv2.VideoCapture(0)` to `1` if you have more than one camera.
