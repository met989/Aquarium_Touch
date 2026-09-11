import serial
import time

try:
    s = serial.Serial('COM7', 115200)
    s.setDTR(False)
    s.setRTS(False)
    print("In ascolto su COM7... (premi il tasto RESET sulla scheda)")
    
    t = time.time()
    while time.time() - t < 20:
        if s.in_waiting > 0:
            print(s.read(s.in_waiting).decode('utf-8', 'ignore'), end='')
        time.sleep(0.1)
except Exception as e:
    print(e)
