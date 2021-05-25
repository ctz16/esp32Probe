import serial
from matplotlib import pyplot as plt
import numpy as npy

datafile = "data.txt"

def writedata(ser,filename):
    with open(filename,'w') as f:
        while True:
            s = ser.readline().decode()
            if s[0] == 'x':
                break
            f.write(s)

def main():
    ser = serial.Serial("COM5",baudrate=115200,timeout=1)
    while True:
        if ser.in_waiting:
            s = ser.readline().decode()
            print(s)
            if s[0] == 'x':
                s = ser.readline().decode()
                total_time = int(s)
                print("total time", total_time)
                writedata(ser,datafile)
                print("data recieved")
                break
    plotVal(datafile,total_time)

def plotVal(filename,time):
    ADvalue = []
    cnt=0
    with open(filename,'r') as f:
        while True:
            s = f.readline()
            if s == '':
                break
            elif s[0] != '\n':
                ADvalue.append(int(s))
                cnt+=1
    ADvalue = npy.array(ADvalue)
    plt.plot(npy.linspace(0,time,cnt),ADvalue)
    plt.xlabel("time[ms]")
    plt.ylabel("AD_val")
    plt.grid(linewidth=0.5)
    plt.show()

if __name__ == "__main__":
    main()