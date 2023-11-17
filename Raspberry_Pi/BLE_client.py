from bluepy import btle
import RPi.GPIO as GPIO

ESP32_MAC_ADDR = "A4:E5:7C:FB:F8:D6"

SERVICE_UUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
MOTOR_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8"
MECHANISM_UUID = "88ae53aa-72ad-44e4-ac95-8e5f7a4a0bd2"

setup_data = b'\x01\x00'

# enkoder motor
mota = 24
motb = 25
signal = 27
el_pow = 20

# servomotori za pokretanje mehanizma
servoA = 12
servoB = 22

# setup
GPIO.setwarnings(False)
GPIO.setmode(GPIO.BCM) # definira raspored GPIO pinova

GPIO.setup(mota, GPIO.OUT)
GPIO.setup(motb, GPIO.OUT)
GPIO.setup(signal, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setup(el_pow, GPIO.OUT)
GPIO.output(el_pow, GPIO.HIGH)
left = GPIO.PWM(mota, 100)
right = GPIO.PWM(motb, 100)

GPIO.setup(servoA, GPIO.OUT)
GPIO.setup(servoB, GPIO.OUT)

servo1 = GPIO.PWM(servoA, 100)
servo2 = GPIO.PWM(servoB, 100)

servo1.start(0)
servo2.start(0)

active = right
stop = True

def MotorControl(drive):
    global active, left, right, stop
    # čitanje podatka (binarni zapis) sa VIDI X
    if drive == b'\x01\x00':    # LIJEVO
        if (active == right):
            active = left
            if (stop == False):
                right.stop()
        left.start(100)
        stop = False
    elif drive == b'\x02\x00':  # DESNO
        if (active == left):
            active = right
            if (stop == False):
                left.stop()
        right.start(100)
        stop = False
    elif drive == b'\x00\x00':  # STOP
        active.stop()
        stop = True

def MechanismControl(value):
    # čitanje podatka (binarni zapis) sa VIDI X
    if value == b'\x00\x00':    # ON
        servo1.start(100)
        servo2.start(100)
    elif value == b'\x01\x00':  # OFF
        servo1.stop()
        servo2.stop()

def receive_notify(handle, data):
    global motor_handle, mechanism_handle
    if handle == motor_handle:
        MotorControl(data)
    elif handle == mechanism_handle:
        MechanismControl(data)

class PeripheralDelegate(btle.DefaultDelegate):
    def __init__(self, callback):
        btle.DefaultDelegate.__init__(self)
        self.callback = callback

    def handleNotification(self, handle, data):
        self.callback(handle, data)

try:
    ESP32 = btle.Peripheral(ESP32_MAC_ADDR)
    
    delegate = PeripheralDelegate(receive_notify)
    ESP32.setDelegate(delegate)

    # Saznaj vrijednost handle-a "Motor" karakteristike i omogući slanje
    # obavijesti putem te karakteristike (zapis "setup" koda)
    motor_char = ESP32.getCharacteristics(uuid = MOTOR_UUID)[0]
    motor_handle = motor_char.getHandle()
    ESP32.writeCharacteristic(motor_handle + 1, setup_data, True)

    # Saznaj vrijednost handle-a "Mechanism" karakteristike i omogući slanje
    # obavijesti putem te karakteristike (zapis "setup" koda)
    mechanism_char = ESP32.getCharacteristics(uuid = MECHANISM_UUID)[0]
    mechanism_handle = mechanism_char.getHandle()
    ESP32.writeCharacteristic(mechanism_handle + 1, setup_data, True)

    print("Waiting...")
    while True:
        if (ESP32.waitForNotifications(60)):
            pass
        else:
            print("Disconnecting from ESP32 device...")
            ESP32.disconnect()
            GPIO.cleanup()
    
except KeyboardInterrupt:
    ESP32.disconnect()
    GPIO.cleanup()
    
