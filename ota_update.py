import serial
import RPi.GPIO as GPIO
import time

spin = 11

def reset(ser):
    GPIO.output(spin,GPIO.LOW)
    time.sleep(0.01)
    GPIO.output(spin,GPIO.HIGH)
    time.sleep(0.01)
    for i in range(15): #gap 15 lines ROM boot log
        s = ser.readline()
    print(s)

def monitor(ser,seconds):
    tic = time.time()
    while time.time()-tic < seconds:
        if ser.in_waiting:
            s = ser.readline()
            print(s)

def main():
    ser = serial.Serial("/dev/ttyS0",baudrate = 115200, parity = serial.PARITY_NONE,stopbits=serial.STOPBITS_ONE,bytesize=serial.EIGHTBITS,timeout=1)
    GPIO.setmode(GPIO.BOARD)
    GPIO.setup(spin,GPIO.OUT)
    GPIO.output(spin,GPIO.HIGH)
    reset(ser)
    
    cmd = "u"
    ser.write(cmd)
    print("cmd writed: "+cmd)
    monitor(ser,180)

if __name__ == "__main__":
    main()  