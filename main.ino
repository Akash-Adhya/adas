#include <Servo.h>
#include <L298N.h>
#include <NewPing.h>
#include <SoftwareSerial.h> 

#define MAX_DISTANCE 200 // Increased max distance
#define SAFE_DISTANCE 30 // Distance threshold for obstacle avoidance
#define MIN_DISTANCE 10  // Minimum distance before emergency stop
#define TURN_DELAY 300   // Standard turning duration

// Pin definitions
const int TRIG_PIN = A4, ECHO_PIN = A5;
const int LEFT_IR = 8, RIGHT_IR = 9;    // IR sensors
const int SERVO_PIN = 7;    // Servo motor pin
const int BUZZER_PIN = 3;   // Buzzer for alerts
const int LED_PIN = 2;      // Status LED

// Motor control pins
const int FL = 4, VL = 6, BL = 5;   // Left motor
const int FR = 12, VR = 11, BR = 13; // Right motor

// System variables
int currentSpeed = 60; // Default speed (0-100)
int autoSpeed = 60;    // Autonomous mode speed
int manualSpeed = 80;  // Manual mode speed
int systemMode = 0;    // 0=Auto, 1=Manual, 2=Parking
unsigned long lastObstacleTime = 0;

Servo steeringServo;
L298N leftMotor(VL, FL, BL);
L298N rightMotor(VR, FR, BR);
NewPing sonar(TRIG_PIN, ECHO_PIN, MAX_DISTANCE);

void setup() {
  Serial.begin(9600);
  
  // Initialize pins
  pinMode(LEFT_IR, INPUT);
  pinMode(RIGHT_IR, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  
  // Initialize servo
  steeringServo.attach(SERVO_PIN);
  centerServo();
  
  // Initial beep to indicate system ready
  beep(1);
  digitalWrite(LED_PIN, HIGH);
  
  Serial.println("ADAS System Initialized");
}

void loop() {
  int distance = getDistance();
  
  // Mode selection (could be controlled via Bluetooth)
  if (Serial.available()) {
    char cmd = Serial.read();
    handleCommand(cmd);
  }
  
  if (systemMode == 0) { // Autonomous mode
    autonomousDrive(distance);
  } 
  else if (systemMode == 1) { // Manual mode
    // Would be controlled via Bluetooth/remote
  }
  else if (systemMode == 2) { // Parking mode
    autoParking();
  }
  
  delay(50); // Main loop delay
}

// Autonomous driving logic
void autonomousDrive(int distance) {
  int leftIR = digitalRead(LEFT_IR);
  int rightIR = digitalRead(RIGHT_IR);

  
  if (distance < MIN_DISTANCE) {
    emergencyStop();
    return;
  }
  
  // Line following when no obstacles
  if (distance > SAFE_DISTANCE * 2) {
    if (!leftIR && !rightIR) {
      moveForward(autoSpeed);
    } 
    else if (leftIR && !rightIR) {
      turnRight(autoSpeed);
    } 
    else if (!leftIR && rightIR) {
      turnLeft(autoSpeed);
    } 
    else {
      stopMotors();
    }
  } 
  // Obstacle avoidance
  else {
    avoidObstacle(distance);
  }
}

// Obstacle avoidance routine
void avoidObstacle(int centerDistance) {
  stopMotors();
  delay(200);
  
  int leftDist = scanLeft();
  int rightDist = scanRight();
  
  Serial.print("Left: "); Serial.print(leftDist);
  Serial.print(" Right: "); Serial.println(rightDist);
  
  if (centerDistance < MIN_DISTANCE) {
    emergencyStop();
    moveBackward(autoSpeed);
    delay(500);
    stopMotors();
    return;
  }
  
  if (leftDist > rightDist && leftDist > SAFE_DISTANCE) {
    // More space on left
    turnLeft(autoSpeed);
    delay(TURN_DELAY);
    moveForward(autoSpeed);
    delay(300);
  } 
  else if (rightDist > SAFE_DISTANCE) {
    // More space on right
    turnRight(autoSpeed);
    delay(TURN_DELAY);
    moveForward(autoSpeed);
    delay(300);
  } 
  else {
    // No clear path - back up and try again
    moveBackward(autoSpeed);
    delay(800);
    stopMotors();
  }
  
  centerServo();
}

// Automatic parking routine
void autoParking() {
  int distance = getDistance();
  
  if (distance > 30) {
    moveForward(40);
  } 
  else if (distance > 15) {
    moveForward(30);
  } 
  else {
    stopMotors();
    beep(3);
    systemMode = 0; // Return to autonomous mode
  }
}

// Sensor functions
int getDistance() {
  delay(30);
  int dist = sonar.ping_cm();
  return (dist == 0) ? MAX_DISTANCE : dist;
}

int scanLeft() {
  steeringServo.write(135);
  delay(500);
  int dist = getDistance();
  centerServo();
  return dist;
}

int scanRight() {
  steeringServo.write(45);
  delay(500);
  int dist = getDistance();
  centerServo();
  return dist;
}

// Motor control functions
void moveForward(int speed) {
  leftMotor.setSpeed(speed);
  rightMotor.setSpeed(speed);
  leftMotor.forward();
  rightMotor.forward();
}

void moveBackward(int speed) {
  leftMotor.setSpeed(speed);
  rightMotor.setSpeed(speed);
  leftMotor.backward();
  rightMotor.backward();
}

void turnLeft(int speed) {
  leftMotor.setSpeed(speed);
  rightMotor.setSpeed(speed);
  leftMotor.backward();
  rightMotor.forward();
}

void turnRight(int speed) {
  leftMotor.setSpeed(speed);
  rightMotor.setSpeed(speed);
  leftMotor.forward();
  rightMotor.backward();
}

void stopMotors() {
  leftMotor.stop();
  rightMotor.stop();
}

void emergencyStop() {
  stopMotors();
  beep(2);
}

// Utility functions
void centerServo() {
  steeringServo.write(90);
  delay(100);
}

void beep(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
  }
}

void handleCommand(char cmd) {
  switch (cmd) {
    case 'A': systemMode = 0; break; // Auto mode
    case 'M': systemMode = 1; break; // Manual mode
    case 'P': systemMode = 2; break; // Parking mode
    case '+': autoSpeed = min(autoSpeed + 5, 100); break;
    case '-': autoSpeed = max(autoSpeed - 5, 30); break;
  }
  Serial.print("Mode: "); Serial.print(systemMode);
  Serial.print(" Speed: "); Serial.println(autoSpeed);
}