from hspy.hspy import hspy
import time
import RPi.GPIO as GPIO

Uset = 100.0
Iset = 10.0
trigPin = 40

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

print("Uout:", s.getUout())
GPIO.setwarnings(False)
GPIO.setmode(GPIO.BOARD)
GPIO.setup(trigPin, GPIO.OUT)
GPIO.output(trigPin,GPIO.HIGH)
time.sleep(0.001)
GPIO.output(trigPin,GPIO.LOW)
GPIO.cleanup()
print("triggered")
