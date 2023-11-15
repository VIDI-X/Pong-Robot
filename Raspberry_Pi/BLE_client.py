from bluetooth import *
from bluepy import btle
import RPi.GPIO as GPIO
import time
import struct

ESP32_MAC_ADDR = "A4:E5:7C:FB:F8:D6"

SERVICE_UUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
MOTOR_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8"
BALL_UUID = "88ae53aa-72ad-44e4-ac95-8e5f7a4a0bd2"

setup_data = b'\x01\x00'

stopped = True

# enkoder motor
mota = 24
motb = 25
signal = 27
el_pow = 20

# servomotori za mehanizam
servoA = 12
#servo_L2 = 13
servoB = 22
#servo_R2 = 23

GPIO.setwarnings(False)
GPIO.setmode(GPIO.BCM)

GPIO.setup(mota, GPIO.OUT)
GPIO.setup(motb, GPIO.OUT)
GPIO.setup(signal, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setup(el_pow, GPIO.OUT)
GPIO.output(el_pow, GPIO.HIGH) #3.3V

left = GPIO.PWM(mota, 100)
right = GPIO.PWM(motb, 100)

GPIO.setup(servoA, GPIO.OUT)
#GPIO.setup(servo_L2, GPIO.OUT)
GPIO.setup(servoB, GPIO.OUT)
#GPIO.setup(servo_R2, GPIO.OUT)

servo1 = GPIO.PWM(servoA, 100)
servo2 = GPIO.PWM(servoB, 100)

servo1.start(0)
servo2.start(0)

active = right
active.start(0)


def Forward(motor, val):
    motor.ChangeDutyCycle(val)

def MotorControl(drive):
    global active, stopped
    global left, right
    global mota, motb
    if drive == b'\x01\x00':
        print("Going left")
        
        if (active == right):
            active = left
            
            right.stop()
            GPIO.output(motb, GPIO.LOW)
            #GPIO.cleanup()
            
            #lijevi = GPIO.PWM(motb1, 50)
            #desni = GPIO.PWM(motb2, 100)
            left.start(0)
        
        if (stopped == True):
            stopped = False
            Forward(left, 100)
        
    elif drive == b'\x02\x00':
        print("Going right")
        
        if (active == left):
            active = right
            
            left.stop()
            GPIO.output(mota, GPIO.LOW)
            #GPIO.cleanup()
            
            #lijevi = GPIO.PWM(mota1, 50)
            #desni = GPIO.PWM(mota2, 100)
            
            right.start(0)
        
        if (stopped == True):
            stopped = False
            Forward(right, 100)
        
    elif drive == b'\x00\x00':
        print("Stopping")
        if (stopped == False):
            stopped = True
            Forward(active, 0) #Forward(lijevi, desni, 0)

def BallControl(value):
    if value == b'\x00\x00':
        Forward(servo1, 100)
        Forward(servo2, 100)
    elif value == b'\x01\x00':
        Forward(servo1, 0)
        Forward(servo2, 0)


def receive_notify(handle, data):
    global ESP32
    
    #val = ESP32.readCharacteristic(handle)
    print(handle, data)
    if handle == 42:
        MotorControl(data)
    elif handle == 45:
        BallControl(data)


class PeripheralDelegate(btle.DefaultDelegate):
    def __init__(self, callback):
        btle.DefaultDelegate.__init__(self)
        self.callback = callback

    def handleNotification(self, handle, data):
        print("Handling notification...")
        self.callback(handle, data)


try:
    ESP32 = btle.Peripheral(ESP32_MAC_ADDR)
    
    motor_delegate = PeripheralDelegate(receive_notify)
    ESP32.setDelegate(motor_delegate)
    
    motor_char = ESP32.getCharacteristics(uuid = MOTOR_UUID)[0]
    motor_handle = motor_char.getHandle() + 1
    #print(motor_handle)
    
    ESP32.writeCharacteristic(motor_handle, setup_data, True)
    
    ball_delegate = PeripheralDelegate(receive_notify)
    ESP32.setDelegate(ball_delegate)
    
    ball_char = ESP32.getCharacteristics(uuid = BALL_UUID)[0]
    ball_handle = ball_char.getHandle() + 1
    #print(reket_handle)
    
    ESP32.writeCharacteristic(ball_handle, setup_data, True)
    
    print ("Waiting for notifications...")
    while True:
        
        if (ESP32.waitForNotifications(60)):
            #print("Finished waiting...")
            pass
        else:
            print("Disconnecting from ESP32 device...")
            ESP32.disconnect()
            
            left.stop()
            right.stop()
            servo1.stop()
            servo2.stop()
            GPIO.cleanup()
    
except KeyboardInterrupt:
    print("Canceling bluetooth ...")
    #client_sock.close()
    ESP32.disconnect()
    
    left.stop()
    right.stop()
    servo1.stop()
    servo2.stop()
    GPIO.cleanup()
