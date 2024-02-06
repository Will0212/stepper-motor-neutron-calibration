// Pin assignments
const int dirPin = 2;           // Stepper motor direction, connected to DIR pin
const int clockPin = 3;         // Stepper motor rotation, connected to CLOCK pin
const int onoffPin = 5;         // Turn Stepper ON or OFF, connected to ON/OFF pin
const int inductionSensorPin1 = A0; // Inductive Sensor1 Input, switches ON/OFF depending on position of metal bead
const int inductionSensorPin2 = A1; // Inductive Sensor2 Input, switches ON/OFF depending on position of metal bead
const int pushbuttonPin = 12;    // Changes value of ON/OFF pin

// Boolean flags to control the motor
bool overrideOn = false; // To control which motor function is enabled, regualar or overriden
bool motorControlEnabled = false; // To control the whether the motor can move or not
bool direction = true; // To control the direction, true counter clockwise, false for clockwise
//bool gotDetected = false; // To check if a metal is detected, true for yes, false for not


// initial values
int timeDelay = 1000; // Delay between clock inputs in ms
int newDelay; // Delay in ms to replace timeDelay when inputted via Webapp
int stepCount = 0; // Current steps done by motor
float sourcePosition = 0; // Sistance of source from home


// Parameters of the motor
const float perimeter = 6*0.0254; // Perimeter of motor in meters
const int stepsPerRevolution = 1600; // Steps in one revolution of the motor
const float distancePerStep = perimeter/stepsPerRevolution; //Distance travelled in one step in meters


// Parameters to deal with data transfer
unsigned long lastSendTime = 0; // Last time data was sent 
const long sendInterval = 100; // Send data every 100 ms


//Initial setup when motor Webapp started
void setup()
{
  // Output pin setup
  pinMode(dirPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(onoffPin, OUTPUT);

  // Input pin setup
  pinMode(inductionSensorPin1, INPUT_PULLUP);
  pinMode(inductionSensorPin2, INPUT_PULLUP);
  pinMode(pushbuttonPin, INPUT);

  // Values of output pins
  digitalWrite(dirPin, HIGH);
  digitalWrite(onoffPin, LOW);
  digitalWrite(pushbuttonPin, LOW);

  // Start serial communication
  Serial.begin(9600);
}

// Continous loop through these functions
void loop()
{
  overrideOn ? motorFunctionOverriden() : motorFunction(); //Use regular motor function when override is off, Use overriden motor function when override is on

  readCommand(); // Read the commands from the Webapp and change values accordingly

  updatePosition();  // Update the current postion of the source on the Webapp
}

// Rotate the motor one step
void rotateMotor() 
{
  digitalWrite(clockPin, HIGH); // Set the clock pin to high
  delayMicroseconds(timeDelay); // Delay by timeDelay ms
  digitalWrite(clockPin, LOW); // Set the clock pin to low
  delayMicroseconds(timeDelay); // Delay by timeDelay ms
  stepCount++; // Increment the step count by 1
  setPosition(); // Set the position of the source
}


// Set the position of the motor
void setPosition() 
{
  sourcePosition = direction ? sourcePosition + distancePerStep : sourcePosition - distancePerStep; // Increase the position if direction is true, increase if false
}


// Change the direction of the motor
void ChangeDirection()
{
  direction = !direction; // Change the value of the direction flag
  digitalWrite(dirPin, direction ? HIGH : LOW); // If direction is true, set dirPIn to high, if it's false, set dirPin to true
  Serial.print("direction:");
  Serial.println(direction ? "CounterClockWise" : "ClockWise");
}


// Set the direction
void setDirection(bool val)
{
    if(val != direction){ //Only change direction if val is not the same as the current direction
      ChangeDirection();
    }
}


// Send the source to the housing
void goToHousing()
{

  bool targetDirection = getPosition() < 0;// Determine the direction based on the current position
  setDirection(targetDirection);

  motorControlEnabled = true;// Start moving towards the target

  bool targetReached = false;// Use a flag to keep track of whether the target has been reached

  while(!targetReached) {
    if(motorControlEnabled) {// Update the motor state without blocking
      rotateMotor(); // Rotate the motor
    }
    if(getPosition() >= -0.001 && getPosition() <= 0.001) {// Check if the target position is reached
      digitalWrite(onoffPin, LOW); // Stop the motor
      motorControlEnabled = false; // Disable further motor control
      targetReached = true; // Set the flag to exit the loop
    }

    // Insert a minimal delay to allow for system responsiveness
    delay(1); // Adjust this delay as needed to balance responsiveness with motor control accuracy

    // Optionally, include logic to periodically break out of this loop to allow the main loop to process
  }
}


// Get the current position of the source
float getPosition()
{
  return sourcePosition; // Return the current position of the source
}


// Perform specific actions depending on a specific command
void readCommand()
{
  if (Serial.available() > 0)// Check if serial data is available 
  {
    String command = Serial.readStringUntil('\n');// Set command to the inputted command from the Webapp
    if (command == "stop") 
    {
      motorControlEnabled = false;
    } else if (command == "start") 
    {
      motorControlEnabled = true;
      stepCount=0;
    }
    else if (command == "changedirection")
    {
      ChangeDirection();
    }
    else if (command.startsWith("setDelay"))
    {
      int index = command.indexOf(' ');
      if (index != -1) 
      {
        String delayString = command.substring(index + 1);
        newDelay = delayString.toInt();
        timeDelay = newDelay;
      }
    }
    else if(command == "overrideOn" && motorControlEnabled == false)
    {
      if(overrideOn == true){
        overrideOn = false;
        Serial.print("overrideOn:");
        Serial.println(overrideOn ? "ON" : "OFF");
      }
      else{
        overrideOn = true;
        Serial.print("overrideOn:");
        Serial.println(overrideOn ? "ON" : "OFF");
      }
    }

    else if (command == "goToHousing"){
      goToHousing();
    }
    else if (command == "goToDetector"){
      goToDetector();
    }
  }
}


// Send the current position to the Webapp
void updatePosition()
{
  if (millis() - lastSendTime > sendInterval)// Check if the current time minus the last recorded time is greater than the predefined interval 
    {
      Serial.print("sourcePosition:");// If the condition is true, update the postion on the Webapp
      Serial.println(sourcePosition);

      lastSendTime = millis();// Update lastSendTime to the current time, resetting the timer for the next interval
    }
}

// General function to control the motor
void motorFunction()
{
  int sensorValue1 =  analogRead(inductionSensorPin1);// Get the value of the sensor pin 1
  float voltage1 = sensorValue1 * (12.0/1023.0);// Convert the value to a voltage

  int sensorValue2 =  analogRead(inductionSensorPin2);// Get the value of the sensor pin 2
  float voltage2 = sensorValue2 * (12.0/1023.0);// Convert the value to a voltage

  if(digitalRead(inductionSensorPin1) <= 10 && digitalRead(inductionSensorPin2) <= 10)//Check if the voltage in the induction sensor is less than 10
  {
    if (motorControlEnabled && digitalRead(pushbuttonPin) == HIGH)// Check if motor control is allowed and the push button is not pressed  
    {  
      if (direction)// Execute if direction is true
      {
        digitalWrite(onoffPin, HIGH);// Set motor on
        digitalWrite(dirPin, HIGH);// Set direction high
        rotateMotor();// Rotate the motor
      } 
      else // Check if motor control is not allowed or if the push button is pressed
      {
        digitalWrite(onoffPin, HIGH); // Set motor on
        digitalWrite(dirPin, LOW); //Set direction low
        rotateMotor();// Rotate the motor
      }
    }
    else  // Check if the volage in the induction sensor is greater than 10
    {
      digitalWrite(onoffPin, LOW); // Stop the motor
    }
  }
  if(voltage1 > 10 && direction) // CHeck if metal entered the induction sensor  1 while the motor is counterclockwise
  {
    ChangeDirection(); // Change the direction
    for(int i=0;i<1600;i++) //Rotate one revolution
      {
        rotateMotor();
      }
    digitalWrite(onoffPin, LOW); // Stop the motor
    motorControlEnabled = false; // Disallow motor control
  }

  if(voltage2 > 10 && !direction) // CHeck if metal entered the induction sensor 2 while the motor is clockwise
  {
    ChangeDirection(); // Change the direction
    for(int i=0;i<1600;i++) //Rotate one revolution
      {
        rotateMotor();
      }
    digitalWrite(onoffPin, LOW); // Stop the motor
    motorControlEnabled = false; // Disallow motor control
  }
  
  if(digitalRead(pushbuttonPin) == LOW ){ //Check if pushbutton is pressed
    ChangeDirection(); // Change the direction
    for(int i=0 ; i<4800; i++) //Rotate three revolutions
      {
        rotateMotor();
      }
    digitalWrite(onoffPin, LOW); // Stop the motor
    motorControlEnabled = false; // Disallow motor control
  }
}


// Overriden fucntion to control the motor
void motorFunctionOverriden()
{
  if (motorControlEnabled) // Check if motor contros is allowed
  {  
    if (direction) //Check if the direction is true
    {
      digitalWrite(onoffPin, HIGH); // Set motor on
      digitalWrite(dirPin, HIGH); // Set dirPin high
      rotateMotor(); // Rotate the motor
    } 
    else // Check if the direction is false
    {
      digitalWrite(onoffPin, HIGH); // Set motor on
      digitalWrite(dirPin, LOW); // Set dirPin low
      rotateMotor(); // Rotate the motor
    }
  }
}


// Send the source to the housing
void goToDetector()
{
  bool targetDirection = getPosition() < 2;// Determine the direction based on the current position
  setDirection(targetDirection);

  motorControlEnabled = true;// Start moving towards the target

  bool targetReached = false;// Use a flag to keep track of whether the target has been reached

  while(!targetReached) {// Update the motor state without blocking
    if(motorControlEnabled) { // Check if motor control is allowed
      rotateMotor(); // Rotate the motor
    }
    
    if(getPosition() >= 1.999 && getPosition() <= 2.001) {// Check if the target position is reached
      digitalWrite(onoffPin, LOW); // Stop the motor
      motorControlEnabled = false; // Disable further motor control
      targetReached = true; // Set the flag to exit the loop
    }

    // Insert a minimal delay to allow for system responsiveness
    delay(1); // Adjust this delay as needed to balance responsiveness with motor control accuracy

    // Optionally, include logic to periodically break out of this loop to allow the main loop to process
  }
}