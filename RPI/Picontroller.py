# -*- coding: UTF-8 -*-

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
import numpy as np
from matplotlib import pyplot as plt

spin = 11 # reset pin / EN
stime = 20000 # time to discharge in ms
cmd = ""
num_samples = 20000
num_lines = 200
datafile = "data.txt"

def plotVal(filename):
    ADvalue = []
    with open(filename,'r') as f:
        for i in range(num_lines):
            for x in f.readline().split(','):
                if x != '\n':
                    ADvalue.append(int(x))
        time = int(f.readline())
    ADvalue = np.array(ADvalue)
    zeroCross = np.where(np.diff(np.sign(ADvalue-2048)))[0].shape[-1]
    freq = zeroCross / 2 / time * 1000
    print('freq: ', freq)
    plt.plot(np.linspace(0,time,num_samples),ADvalue)
    plt.xlabel("time[ms]")
    plt.ylabel("AD_val")
    plt.grid(linewidth=0.5)
    plt.show()

def reset(ser):
    GPIO.output(spin,GPIO.LOW)
    time.sleep(0.01)
    GPIO.output(spin,GPIO.HIGH)
    time.sleep(0.01)
    for i in range(15): # gap 15 lines ROM boot log
        s = ser.readline().decode()
    print(s)

def writedata(ser,filename,seconds):
    with open(filename,'w') as f:
        tic = time.time()
        while time.time()-tic < seconds:
            s = ser.readline().decode()
            f.write(s)
    
def monitor(ser,seconds):
    tic = time.time()
    while time.time()-tic < seconds:
        if ser.in_waiting:
            s = ser.readline().decode()
            print(s)
            if s[0] == 'X':
                ser.write(str(stime).encode())
            elif s[0] == 'x':
                writedata(ser,datafile,10)
                print("data recieved")
                break
    
def main():
    ser = serial.Serial("/dev/ttyS0",baudrate = 115200, parity = serial.PARITY_NONE,stopbits=serial.STOPBITS_ONE,bytesize=serial.EIGHTBITS,timeout=1)
    GPIO.setwarnings(False)
    GPIO.setmode(GPIO.BOARD)
    GPIO.setup(spin,GPIO.OUT)
    GPIO.output(spin,GPIO.HIGH)
    
    reset(ser)
    cmd = "d"
    ser.write(cmd.encode())
    print("cmd writed: "+cmd)
    monitor(ser,70)

    reset(ser)
    cmd = "o"
    ser.write(cmd.encode())
    print("cmd writed: "+cmd)
    monitor(ser,1)

    plotVal(datafile)


if __name__ == "__main__":
    main()        