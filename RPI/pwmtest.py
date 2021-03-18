import RPi.GPIO as GPIO
from time import sleep

pwm0Pin = 12

GPIO.setwarnings(False)
GPIO.setmode(GPIO.BOARD)
GPIO.setup(pwm0Pin.GPIO.OUT)
pwm = GPIO.pwm(pwm0Pin,125000)

while True:
    pwm.start(50)
    sleep(5)
    pwm.stop()
    sleep(5)