'''
@author: Chang

for a given time to discharge and a given height of the probe, to control the probe server with serial port

cmd: 
d -- discharge
u -- update
o -- program finished, deep sleep
cmd and probe height is given to probe server when turned on
'''

import serial
import RPi.GPIO as GPIO
import time

spin = 11
stime = 500 #time to discharge in ms
cmd = ""

def reset(ser):
    GPIO.output(spin,GPIO.LOW)
    time.sleep(0.01)
    GPIO.output(spin,GPIO.HIGH)
    time.sleep(0.01)
    for i in range(15): #gap 15 lines ROM boot log
        s = ser.readline()
    print(s)

def writedata(ser,filename,seconds):
    with open(filename,'w') as f:
        tic = time.time()
        while time.time()-tic < seconds:
            s = ser.readline()
            f.write(s)
    

def monitor(ser,seconds):
    tic = time.time()
    while time.time()-tic < seconds:
        if ser.in_waiting:
            s = ser.readline()
            print(s)
            if s[0] == 'x':
                writedata(ser,"data.txt",5)
                print("data recieved")
    
def main():
    ser = serial.Serial("/dev/ttyS0",baudrate = 115200, parity = serial.PARITY_NONE,stopbits=serial.STOPBITS_ONE,bytesize=serial.EIGHTBITS,timeout=1)
    GPIO.setmode(GPIO.BOARD)
    GPIO.setup(spin,GPIO.OUT)
    GPIO.output(spin,GPIO.HIGH)
    reset(ser)
    
    cmd = "d"
    ser.write(cmd)
    print("cmd writed: "+cmd)
    monitor(ser,1)
    ser.write(str(stime))
    monitor(ser,20)

    reset(ser)
    cmd = "o"
    ser.write(cmd)
    print("cmd writed: "+cmd)
    monitor(ser,1)
    
if __name__ == "__main__":
    main()        

