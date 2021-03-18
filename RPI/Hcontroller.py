import RPi.GPIO as GPIO
from time import sleep

pwm0Pin = 12

def sendpwm():
    GPIO.setwarnings(False)
    GPIO.setmode(GPIO.BOARD)
    GPIO.setup(pwm0Pin.GPIO.OUT)
    pwm = GPIO.pwm(pwm0Pin,125000)
    pwm.start(50)
    sleep(0.005)
    pwm.stop()
