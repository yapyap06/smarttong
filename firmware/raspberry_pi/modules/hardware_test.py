import time
import RPi.GPIO as GPIO
from gpiozero import Servo
from gpiozero.pins.pigpio import PiGPIOFactory

# ---------------------------------------------------------
# SMARTTONG - RASPBERRY PI 5 SENSOR & ACTUATOR TEST SCRIPT
# ---------------------------------------------------------

# Use pigpio for hardware PWM on Raspberry Pi 5 (reduces servo jitter)
factory = PiGPIOFactory()

# ==========================================
# 1. ULTRASONIC SENSOR (HC-SR04) CONFIG
# ==========================================
# Example for one compartment (e.g., Plastic)
TRIG_PIN = 23
ECHO_PIN = 24

GPIO.setmode(GPIO.BCM)
GPIO.setup(TRIG_PIN, GPIO.OUT)
GPIO.setup(ECHO_PIN, GPIO.IN)

def get_distance_cm():
    """Reads distance from the HC-SR04 sensor."""
    # Send 10us pulse to trigger
    GPIO.output(TRIG_PIN, True)
    time.sleep(0.00001)
    GPIO.output(TRIG_PIN, False)

    start_time = time.time()
    stop_time = time.time()

    # Wait for echo to go high
    while GPIO.input(ECHO_PIN) == 0:
        start_time = time.time()

    # Wait for echo to go low
    while GPIO.input(ECHO_PIN) == 1:
        stop_time = time.time()

    # Calculate distance (speed of sound = 34300 cm/s)
    time_elapsed = stop_time - start_time
    distance = (time_elapsed * 34300) / 2
    return round(distance, 1)


# ==========================================
# 2. SERVO MOTOR (TowerPro MG996R) CONFIG
# ==========================================
# Connect servo signal wire to GPIO 18
SERVO_PIN = 18

# Initialize servo with custom pulse widths suitable for MG996R
# min_pulse_width usually around 0.5ms (0.0005) and max around 2.5ms (0.0025)
sorting_servo = Servo(SERVO_PIN, min_pulse_width=0.5/1000, max_pulse_width=2.5/1000, pin_factory=factory)

def sort_item(category):
    """Moves the servo to guide the item into the correct compartment."""
    print(f"Sorting item to: {category}")
    
    if category == "PLASTIC":
        sorting_servo.min()  # -90 degrees
    elif category == "PAPER":
        sorting_servo.value = -0.33  # -30 degrees
    elif category == "METAL":
        sorting_servo.value = 0.33   # +30 degrees
    elif category == "SISA_BAKI":
        sorting_servo.max()  # +90 degrees
    else:
        sorting_servo.mid()  # 0 degrees (neutral/closed)

    time.sleep(1) # Wait for servo to reach position
    sorting_servo.mid() # Return to neutral position


# ==========================================
# MAIN TEST LOOP
# ==========================================
try:
    print("Starting SmartTong Hardware Test...")
    print("Press CTRL+C to exit.")
    
    # Test Servo
    print("\n--- Testing Servo Motor ---")
    sort_item("PLASTIC")
    time.sleep(1)
    sort_item("METAL")
    
    # Test Ultrasonic
    print("\n--- Testing Ultrasonic Sensor ---")
    while True:
        dist = get_distance_cm()
        print(f"Distance measured: {dist} cm")
        
        # Simple fill percentage logic (assume bin is 50cm deep)
        # Empty = 50cm, Full = 5cm
        fill_pct = 100 - ((dist - 5) / 45 * 100)
        fill_pct = max(0, min(100, fill_pct)) # Clamp between 0-100
        
        print(f"Estimated Fill Level: {round(fill_pct)}%")
        
        time.sleep(2)

except KeyboardInterrupt:
    print("\nTest stopped by user.")
finally:
    GPIO.cleanup()
    sorting_servo.close()
    print("GPIO Cleaned up.")
