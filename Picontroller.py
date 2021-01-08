'''
@author: Chang

for a given time to discharge and a given height of the probe, to control the probe server with serial port

cmd: 
d -- discharge
t -- transfer data
u -- update
cmd and probe height is given to probe server when turned on
'''

import serial
import RPi.GPIO as GPIO
import time

spin = 18
stime = 0
cmd = ""

def main():
    ser = serial.Serial("/dev/ttyS0",baudrate = 115200, parity = serial.PARITY_NONE,stopbits=serial.STOPBITS_ONE,bytesize=serial.EIGHTBITS,timeout=1)
    GPIO.setup(spin,GPIO.OUT)
    GPIO.output(spin,GPIO.HIGH)
    cmd = "d"+ str(stime)
    ser.write(cmd)
    time.sleep(1.5)
    GPIO.output(spin,GPIO.LOW)
    time.sleep(0.01)
    GPIO.output(spin,GPIO.HIGH)
    cmd = 't'
    ser.write(cmd)
    with open("data.txt",'w') as f:
        s = ""
        while 't' not in s:
            s = ser.readline()
            f.write(s)
            f.write('\n')
    GPIO.output(spin,GPIO.LOW)
    
        
