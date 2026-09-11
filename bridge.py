import cv2
import mediapipe as mp
import serial
import serial.tools.list_ports
import time

# --- Find Arduino port automatically ---
def find_arduino():
    ports = serial.tools.list_ports.comports()
    for port in ports:
        if 'Arduino' in port.description or 'CH340' in port.description or 'USB Serial' in port.description:
            return port.device
    return None

# --- Finger up/down detection ---
FINGER_TIPS = {'Thumb': 4, 'Index': 8, 'Middle': 12, 'Ring': 16, 'Pinky': 20}
FINGER_MCP  = {'Thumb': 2, 'Index': 5, 'Middle': 9,  'Ring': 13, 'Pinky': 17}

def is_finger_up(landmarks, tip, mcp, hand_label):
    if tip == 4:  # Thumb moves sideways, so compare x instead of y
        return landmarks[tip].x > landmarks[mcp].x if hand_label == 'Left' \
               else landmarks[tip].x < landmarks[mcp].x
    return landmarks[tip].y < landmarks[mcp].y  # tip above knuckle = extended

# --- Connect to Arduino ---
port = find_arduino()
arduino = None
if port:
    print(f"Arduino found on {port}")
    arduino = serial.Serial(port, 9600, timeout=1)
    time.sleep(2)  # wait for the board to reset after the port opens
else:
    print("No Arduino found - running in preview mode (no serial)")

# --- MediaPipe setup ---
mp_hands = mp.solutions.hands
mp_draw  = mp.solutions.drawing_utils
hands    = mp_hands.Hands(max_num_hands=1,
                          min_detection_confidence=0.7,
                          min_tracking_confidence=0.5)

cap = cv2.VideoCapture(0)
print("Camera running. Press Q to quit.")

last_sent = None  # last packet we transmitted; used to only resend on change

while True:
    ret, frame = cap.read()
    if not ret:
        break

    frame  = cv2.flip(frame, 1)  # mirror so movement feels natural
    rgb    = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    result = hands.process(rgb)

    if result.multi_hand_landmarks:
        landmarks  = result.multi_hand_landmarks[0].landmark
        hand_label = result.multi_handedness[0].classification[0].label

        mp_draw.draw_landmarks(frame, result.multi_hand_landmarks[0],
                               mp_hands.HAND_CONNECTIONS)

        # Build finger states [Thumb, Index, Middle, Ring, Pinky]; 1 = extended, 0 = curled
        finger_states = []
        for name, tip in FINGER_TIPS.items():
            up = is_finger_up(landmarks, tip, FINGER_MCP[name], hand_label)
            finger_states.append(1 if up else 0)

        # Web shooter: fire only when Middle (idx 2) AND Ring (idx 3) are both curled
        web = 1 if (finger_states[2] == 0 and finger_states[3] == 0) else 0

        # Compose the 6-char packet, e.g. "100101"
        data = ''.join(map(str, finger_states)) + str(web)

        # On-screen readout
        state_str = ' '.join(f"{'TIMRP'[i]}:{'U' if s else 'D'}"
                             for i, s in enumerate(finger_states))
        state_str += f"  WEB:{'FIRE' if web else '-'}"
        cv2.putText(frame, state_str, (10, 30),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 100), 2)

        # Only transmit when the packet actually changed
        if data != last_sent:
            print(f"Sending: {data}")
            if arduino:
                arduino.write((data + '\n').encode())
            last_sent = data

    cv2.imshow('Hand Tracker Bridge', frame)
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()
if arduino:
    arduino.close()