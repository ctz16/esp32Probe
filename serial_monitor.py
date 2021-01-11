import argparse
import sys
import serial
import RPi.GPIO as GPIO
import time

spin = 11

parser = argparse.ArgumentParser(description='pass a cmd to serial device and listening')
parser.add_argument('cmd',type=str)
args = parser.parse_args()
cmd = args.cmd
print("cmd input:"+cmd)

GPIO.setmode(GPIO.BOARD)
GPIO.setup(spin,GPIO.OUT)
GPIO.output(spin,GPIO.LOW)
time.sleep(0.01)
GPIO.output(spin,GPIO.HIGH)
time.sleep(0.01)

ser = serial.Serial("/dev/ttyS0",baudrate = 115200, parity = serial.PARITY_NONE,stopbits=serial.STOPBITS_ONE,bytesize=serial.EIGHTBITS,timeout=1)
ser.write(cmd)
while True:
    print(ser.readline())