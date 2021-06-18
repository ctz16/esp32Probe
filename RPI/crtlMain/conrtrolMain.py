from hspy.hspy import hspy
import time
import RPi.GPIO as GPIO

Uset = 100.0
Iset = 10.0
bottomTrigPin = 40
topTrigPin = 39
defaultTimeout = 10

s=hspy(serial_port='/dev/ttyUSB0', addr=0)
s.setU(Uset)
s.setI(Iset)
if s.getUset() == Uset:
    print("set U: ",Uset)
else:
    print("set U failed")
if s.getIset() == Iset:
    print("set I: ",Iset)
else:
    print("set I failed")
s.run()
time.sleep(10)
# while True:
#     Uout = s.getUout()
#     print("Uout:", Uout)
#     print("Iout:", s.getIout())
#     if Uout >= Uset:
#         break
s.stop()
print("Uout:", s.getUout())

def trig_callback():
    global topTrigTime
    topTrigTime = time.time()
    global topTrigFlag
    topTrigFlag = True
    
topTrigFlag = False
GPIO.setwarnings(False)
GPIO.setmode(GPIO.BOARD)
GPIO.setup(bottomTrigPin, GPIO.OUT)

GPIO.output(bottomTrigPin,GPIO.HIGH)
time.sleep(0.001)
GPIO.output(bottomTrigPin,GPIO.LOW)
btTrigTime = time.time()
topTrigTime = 0
print("bottom triggered")

GPIO.setup(topTrigPin, GPIO.IN)
GPIO.add_event_detect(topTrigPin, GPIO.RISING, callback=trig_callback)

while True:
    if time.time()-btTrigTime > defaultTimeout:
        print("top trigger not detected")
        break
    if topTrigFlag:
        print("top triggered, time:")
        print(topTrigTime-btTrigTime)
        break

GPIO.cleanup()
