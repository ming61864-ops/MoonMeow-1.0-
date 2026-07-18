#!/usr/bin/env python3
"""
MediaPipe 手势识别 → HC-05 蓝牙 → 月薪喵控制
==============================================
用法:
  1. PC 蓝牙配对 HC-05 (密码 1234) → 记下 COM 口编号
  2. python gesture_control.py COM5
     如果不指定 COM 口, 只显示手势不发送

手势映射:
  五指张开 → t (摸头 SURPRISE)
  拇指向上 → f (喂食 HAPPY)
  握拳     → z (睡觉 SLEEP)
  食指向上 → s (查询状态)
  V 手势   → h (帮助)

依赖: pip install mediapipe opencv-python pyserial
"""

import sys
import time
import cv2
import mediapipe as mp
import serial

# ==================== 配置 ====================
COOLDOWN_SEC = 1.5      # 两次发送最小间隔
CONFIRM_FRAMES = 5      # 连续 N 帧相同手势才发送
CAMERA_IDX = 0          # 摄像头编号
FLIP_HORIZONTAL = True  # 镜像翻转
# =============================================

mp_hands = mp.solutions.hands
mp_draw = mp.solutions.drawing_utils
mp_styles = mp.solutions.drawing_styles

# 手指关键点索引
# 指尖: 4(拇指) 8(食指) 12(中指) 16(无名指) 20(小指)
# PIP关节: 3 6 10 14 18
# MCP关节: 2 5 9 13 17
FINGER_TIPS = [4, 8, 12, 16, 20]
FINGER_PIPS = [3, 6, 10, 14, 18]
FINGER_MCPS = [2, 5, 9, 13, 17]
THUMB_TIP, INDEX_TIP, MIDDLE_TIP, RING_TIP, PINKY_TIP = 4, 8, 12, 16, 20
THUMB_IP, INDEX_PIP, MIDDLE_PIP, RING_PIP, PINKY_PIP = 3, 6, 10, 14, 18
INDEX_MCP, MIDDLE_MCP = 5, 9
WRIST = 0

# 手势名称
GESTURE_NAMES = {
    "OPEN_PALM": "Open Palm -> [t] TOUCH",
    "THUMB_UP":  "Thumb Up  -> [f] FEED",
    "FIST":      "Fist      -> [z] SLEEP",
    "POINT_UP":  "Point Up  -> [s] STATUS",
    "VICTORY":   "V Sign    -> [h] HELP",
    "NONE":      "---",
}

# 手势 → 发送字符
GESTURE_CMD = {
    "OPEN_PALM": b't',
    "THUMB_UP":  b'f',
    "FIST":      b'z',
    "POINT_UP":  b's',
    "VICTORY":   b'h',
}


def is_finger_up(lm, tip_idx, pip_idx):
    """指尖在 PIP 关节上方 = 手指伸直"""
    return lm.landmark[tip_idx].y < lm.landmark[pip_idx].y


def is_finger_curled(lm, tip_idx, pip_idx):
    """指尖在 PIP 关节下方 = 手指弯曲"""
    return lm.landmark[tip_idx].y > lm.landmark[pip_idx].y


def classify_gesture(lm):
    """基于 21 个手部关键点判断手势"""
    fingers_up = 0
    for tip, pip in zip(FINGER_TIPS, FINGER_PIPS):
        if is_finger_up(lm, tip, pip):
            fingers_up += 1

    thumb_up = is_finger_up(lm, THUMB_TIP, THUMB_IP)
    index_up = is_finger_up(lm, INDEX_TIP, INDEX_PIP)
    middle_up = is_finger_up(lm, MIDDLE_TIP, MIDDLE_PIP)
    ring_up = is_finger_up(lm, RING_TIP, RING_PIP)
    pinky_up = is_finger_up(lm, PINKY_TIP, PINKY_PIP)

    # 大拇指: 指尖在 IP 上方 且 x 坐标明显外展 (拇指朝向判断)
    thumb_extended = (
        lm.landmark[THUMB_TIP].y < lm.landmark[THUMB_IP].y and
        abs(lm.landmark[THUMB_TIP].x - lm.landmark[INDEX_MCP].x) > 0.08
    )

    # V 手势: 食指+中指伸直, 无名指+小指弯曲
    if index_up and middle_up and not ring_up and not pinky_up:
        return "VICTORY"

    # 食指向上: 仅食指伸直
    if index_up and not middle_up and not ring_up and not pinky_up:
        return "POINT_UP"

    # 拇指向上: 拇指翘起, 其余弯曲
    if thumb_extended and not index_up and not middle_up and not ring_up:
        return "THUMB_UP"

    # 握拳: 全部弯曲
    if fingers_up == 0:
        return "FIST"

    # 五指张开
    if fingers_up >= 4:
        return "OPEN_PALM"

    return "NONE"


def find_com_port():
    """自动查找 HC-05 蓝牙 COM 口 (Windows)"""
    import subprocess
    try:
        out = subprocess.check_output(
            'powershell -c "Get-PnpDevice -Class Bluetooth | '
            'Where-Object {$_.Name -like \\"*HC-05*\\"} | '
            'Select-Object -First 1 | ForEach-Object { '
            '  $dev = $_; '
            '  Get-CimInstance Win32_SerialPort | '
            '  Where-Object {$_.PNPDeviceID -like \\"*$($dev.InstanceId)*\\"} | '
            '  Select-Object -ExpandProperty DeviceID }"',
            shell=True, text=True, timeout=10
        )
        port = out.strip()
        if port:
            return port
    except Exception:
        pass
    return None


def main():
    # 串口
    ser = None
    if len(sys.argv) > 1:
        port = sys.argv[1]
    else:
        print("Searching for HC-05 COM port...")
        port = find_com_port()
        if not port:
            print("HC-05 not found. Running in demo-only mode (no TX).")
            print("Specify port: python gesture_control.py COM5")

    if port:
        try:
            ser = serial.Serial(port, 9600, timeout=0.5)
            print(f"Connected: {port} @ 9600")
        except Exception as e:
            print(f"Serial error: {e}")
            print("Running in demo-only mode.")
            ser = None

    # 摄像头
    cap = cv2.VideoCapture(CAMERA_IDX)
    if not cap.isOpened():
        print("ERROR: Camera not found!")
        sys.exit(1)

    hands = mp_hands.Hands(
        static_image_mode=False,
        max_num_hands=1,
        min_detection_confidence=0.7,
        min_tracking_confidence=0.5,
    )

    last_gesture = "NONE"
    gesture_count = 0
    last_send = 0
    last_status = ""

    print("\n=== MoonCat Gesture Control ===")
    print("  Open Palm → TOUCH  |  Thumb Up → FEED")
    print("  Fist      → SLEEP  |  Point Up → STATUS")
    print("  V Sign    → HELP")
    print("  Press 'q' to quit\n")

    while True:
        ok, frame = cap.read()
        if not ok:
            break

        if FLIP_HORIZONTAL:
            frame = cv2.flip(frame, 1)

        h, w = frame.shape[:2]
        rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        result = hands.process(rgb)

        gesture = "NONE"

        if result.multi_hand_landmarks:
            for hand_lm in result.multi_hand_landmarks:
                mp_draw.draw_landmarks(
                    frame, hand_lm, mp_hands.HAND_CONNECTIONS,
                    mp_styles.get_default_hand_landmarks_style(),
                    mp_styles.get_default_hand_connections_style(),
                )
                gesture = classify_gesture(hand_lm)

        # 手势确认 (连续 CONFIRM_FRAMES 帧一致)
        if gesture == last_gesture and gesture != "NONE":
            gesture_count += 1
        else:
            gesture_count = 1
            last_gesture = gesture

        now = time.time()
        should_send = (
            gesture_count >= CONFIRM_FRAMES and
            gesture in GESTURE_CMD and
            now - last_send >= COOLDOWN_SEC
        )

        if should_send:
            cmd = GESTURE_CMD[gesture]
            if ser and ser.is_open:
                ser.write(cmd)
                last_status = f"Sent: {gesture} -> {cmd.decode()}"
                print(f"[GESTURE] {last_status}")
            else:
                last_status = f"DEMO: {gesture} -> {cmd.decode()}"
                print(f"[GESTURE] {last_status}")
            last_send = now

        # OSD 显示
        text = GESTURE_NAMES.get(gesture, "---")
        cooldown_left = max(0, COOLDOWN_SEC - (now - last_send))
        status_line = last_status if cooldown_left > 0 else ""

        cv2.putText(frame, text, (10, 30),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
        cv2.putText(frame, f"Frames: {gesture_count}/{CONFIRM_FRAMES}", (10, 60),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 0), 1)
        if status_line:
            cv2.putText(frame, status_line, (10, 85),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.45, (0, 255, 255), 1)
        if ser:
            cv2.putText(frame, f"[{port}]", (w - 70, 25),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 1)

        cv2.imshow("MoonCat Gesture Control", frame)

        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    cap.release()
    cv2.destroyAllWindows()
    if ser:
        ser.close()
    print("Bye!")


if __name__ == "__main__":
    main()
