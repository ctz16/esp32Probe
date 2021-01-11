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

spin = 11
stime = 1 #time to discharge in ms
cmd = ""
discharge_loop_cnt = 10 #total sample points

def reset(ser):
    GPIO.output(spin,GPIO.LOW)
    time.sleep(0.01)
    GPIO.output(spin,GPIO.HIGH)
    time.sleep(0.01)
    for i in range(15): #gap 15 lines ROM boot log
        s = ser.readline()
    print(s)
    
def main():
    ser = serial.Serial("/dev/ttyS0",baudrate = 115200, parity = serial.PARITY_NONE,stopbits=serial.STOPBITS_ONE,bytesize=serial.EIGHTBITS,timeout=1)
    GPIO.setmode(GPIO.BOARD)
    GPIO.setup(spin,GPIO.OUT)
    GPIO.output(spin,GPIO.HIGH)
    reset(ser)
    
    cmd = "d"
    ser.write(cmd)
    print("cmd writed: "+cmd)
    print(ser.readline())
    ser.write(str(stime))
    time.sleep(10)
    reset(ser)
    
    cmd = "t"
    ser.write(cmd)
    print("cmd writed: "+cmd)
    print(ser.readline())
    # with open("data.txt",'w') as f:
    #     s = ""
    #     for i in range(discharge_loop_cnt):
    #         s = ser.readline()
    #         f.write(s)
    for i in range(discharge_loop_cnt):
        s = ser.readline()
        print(s)

    time.sleep(5)
    reset(ser)
    cmd = "o"
    ser.write(cmd)
    print("cmd writed: "+cmd)
    print(ser.readline())
    
if __name__ == "__main__":
    main()        

