from hspy import hspy
import time
# uart3
Uset = 10.0
s=hspy(serial_port='/dev/ttyAMA1', addr=0)
s.setU(Uset)
if s.getUset() == Uset:
    print("set U: ",Uset)
else:
    print("set U failed")
s.run()
while True:
    if s.getUout() > 0.99*Uset:
        s.stop()
        print("charge complete, Uout: ", s.getUout())
        break
    else:
        time.sleep(0.1)

