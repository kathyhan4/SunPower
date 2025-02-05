// LCD Settings
#include <LiquidCrystal_PCF8574.h>
#include <Wire.h>
#include <EEPROM.h>
LiquidCrystal_PCF8574 lcd(0x27); // set the LCD address to 0x27 for a 16 chars and 2 line display
#include <NintendoExtensionCtrl.h>


// Define Arduino pinout
#define ENABLE 7
#define FORWARD 5
#define REVERSE 6
#define LIMITSWITCH 4
#define MAGNET 3
#define A 45
#define B 47
#define X 41
#define Y 43
#define UP 39
#define DOWN 37 
#define LEFT 35
#define RIGHT 33
#define START 49
#define SELECT 51
#define LR 53  // LR button is not working --> need to trace the circuit
#define PRESET_DROP_MEMORY_LOC 0
#define HEIGHT_MEMORY_LOC 2
#define CURRENT_SENSE_LIMIT_LOC 4
#define DELAY_LOC 8
#define DELAY_LOC_DOWN 4002
#define DROP_COUNTER_MEMORY_SPREAD_START 10
#define DROP_COUNTER_MEMORY_SPREAD_STOP 4000
#define LOGIC_LOW 12
#define ERASE_EEPROM 13
#define ANALOG_FILTER_ORDER 40
#define LIFT_TIMEOUT_ADDITION_MS 500
#define DROP_DELAY_MS 500
#define UNWIND_MS_PER_CM 125
#define CURRENT_SENSE_AMP_VOLTAGE_GAIN 2
#define CURRENT_SENSE_PORT A0
#define ADC_MAX_BITS 1023
#define ADC_VCC 5
#define CURRENT_SENSE_NULL_VOLTAGE 2.5
#define CURRENT_SENSE_MAX_CURRENT 5

// Define variables
int intState = 0;
boolean bolA = false;
boolean bolB = false;
boolean bolX = false;
boolean bolY = false;
boolean bolUp = false;
boolean bolDown = false;
boolean bolLeft = false;
boolean bolRight = false;
boolean bolStart = false;
boolean bolSelect = false;
boolean bolTriggerL = false;
boolean bolTriggerR = false;
int intHeight = 0; // height in cm
int intDropCounter = 0;
int intPresetDrops = 0;  
int intLineNumber = 0; // row number
int intDisplayCounter = 0;
int intDropCounterMem = 10;
int intCurrentSense = 0;  // current sensor
int intLPF[ANALOG_FILTER_ORDER];
float floLPF[ANALOG_FILTER_ORDER];
int intCurrentSenseAverage = 0;
float floCurrentSenseAverage = 0;
int intUnwindDelay = 1000; 
int intHoldDelay = 300;  // 1 second delay
unsigned long delayStart = 0; // the time the delay started
bool delayRunning = false; // true if still waiting for delay to finish
bool bolMagnetOn = false;
bool bolBReleasedBefore = true;
SNESMiniController snes;
bool bolOvercurrentError = false;
bool bolLiftLimitError = false;
bool bolPauseError = false;
bool bolModuleStuckError = false;
bool bolUnwindError = false;
float floCurrentLimit = 2.2;
int intDelay = 100;
int intDelayDown = 100;


//Returns the current from the motor
float readMotorCurrent(){
  float floADCVoltage = 0;
  int intADC = 0;

  //Reads the ADC
  intADC = analogRead(CURRENT_SENSE_PORT);

  //Calculates the ADC voltage
  floADCVoltage = (float)(((float)intADC/(float)ADC_MAX_BITS)*(float)ADC_VCC);
  

  //Returns the current
  return (float)(floADCVoltage - CURRENT_SENSE_NULL_VOLTAGE)*CURRENT_SENSE_AMP_VOLTAGE_GAIN;
}

//Read all IO pins/user inputs from controller 
void readIO() {  
  //Reads the SNES controller
  boolean success = snes.update();  // Get new data from the controller
  if (success == true) {  // We've got data!
    //snes.printDebug();  // Print all of the values!
  }
  else {  // Data is bad :(
    Serial.print("SNES Disconnected!; ");
    snes.reconnect();
  }

  //Parses out the SNES controller
  bolA = snes.buttonA();
  bolB = snes.buttonB();
  bolX = snes.buttonX();
  bolY = snes.buttonY();
  bolUp = snes.dpadUp();
  bolDown = snes.dpadDown();
  bolLeft = snes.dpadLeft();
  bolRight = snes.dpadRight();
  bolStart = snes.buttonStart();
  bolSelect = snes.buttonSelect();
  bolTriggerL = snes.triggerL();
  bolTriggerR = snes.triggerR();

  //Reads the current sense
  intCurrentSense = analogRead(A0); 
  }


void print_IO() {
  Serial.print("; X:");
  Serial.print(bolX);
  Serial.print("; Y:");
  Serial.print(bolY);
  Serial.print("; A:");
  Serial.print(bolA);
  Serial.print("; B:");
  Serial.print(bolB);
  Serial.print("; Up:");
  Serial.print(bolUp);
  Serial.print("; Down:");
  Serial.print(bolDown);
  Serial.print("; Left:");
  Serial.print(bolLeft);
  Serial.print("; Right:");
  Serial.print(bolRight);
  Serial.print("; LR: ");
  Serial.print(bolTriggerL);
  Serial.print("; Select: ");
  Serial.print(bolSelect);

  if (intState == 0){
    Serial.print("; State: Idle");
  }
  else if (intState == 1){
    Serial.print("; State: Lift");
  }
  else if (intState == 2){
    Serial.print("; State: Hold");
  }
  else if (intState == 3){
    Serial.print("; State: Unwind");
  }
  else if (intState == 4){
    Serial.print("; State: Drop");
  }
  else if (intState == 5){
    Serial.print("; State: Drop Delay");
  }
  

  Serial.print("; Limit Switch: ");
  Serial.print(digitalRead(LIMITSWITCH));
  Serial.print("; Current Sense: ");
  Serial.print(readMotorCurrent());
  Serial.print("; Current Sense Average: ");
  Serial.print(floCurrentSenseAverage);
  Serial.println("");
}


void LCD_display_init() {
// Initialize the text on the LCD screen
  lcd.begin(20, 4); // Initialize the LCD screen dimensions
  lcd.setBacklight(255);  // LCD settings
  lcd.home();
  lcd.clear();
  lcd.print("OC Trip Amps:");  
  lcd.setCursor(0,1);
  lcd.print("Time Lift (ms):");
  lcd.setCursor(0,2);
  lcd.print("Time Drop (ms):");
  //lcd.setCursor(0,3);
  //lcd.print("Drop Counter:");; 
  }
  
void LCD_display_refresh() {
// Refresh the LCD screen to set the location of the cursor, blank the text, and then print the label
  if ((bolOvercurrentError == false) && (bolLiftLimitError == false) && (bolPauseError == false) && (bolModuleStuckError == false) && (bolUnwindError == false)){
    //Refreshes drop counter
    lcd.setCursor(0,3);
    lcd.print("                    ");  
   // lcd.setCursor(14,3);
   // lcd.print(intDropCounter); 
  }
  else{
    //Displays error
    if (bolOvercurrentError == true){
      lcd.setCursor(0,3);
      lcd.print ("Overcurrent Trip    ");
    }
    else if (bolLiftLimitError == true){
      lcd.setCursor(0,3);
      lcd.print ("Lift limit exceeded ");
    }
    else if (bolPauseError == true){
      lcd.setCursor(0,3);
      lcd.print ("User Pressed Pause ");
    }
    else if (bolModuleStuckError == true){
      lcd.setCursor(0,3);
      lcd.print ("Module Stuck        ") ;
    }
    else if (bolUnwindError == true){
      lcd.setCursor(0,3);
      lcd.print ("Module Unwind Error ") ;
    }
  }
  

  //Refreshes current limit
  lcd.setCursor(14,0);  // Update line 1
  lcd.print("     ");  // Blank the text 
  lcd.setCursor(14,0);
  lcd.print(floCurrentLimit);

  //Refreshes delay time up
  lcd.setCursor(16,1); 
  lcd.print("    ");  
  lcd.setCursor(16,1);
  lcd.print(intDelay);  

  //Refreshes delay time down
  lcd.setCursor(16,2); 
  lcd.print("    ");  
  lcd.setCursor(16,2);
  lcd.print(intDelayDown);  

  }


void LCD_user_interface() {
// Change user inputs based on arrow pad and display on LCD screen
  if (intState == 0) {
    //Set cursor position for each line
    if (intLineNumber == 1) {
      lcd.setCursor(12,1);
      lcd.blink(); 
      }  // Set blinking cursor
     else if (intLineNumber == 0) {
      lcd.setCursor(14,0);
      lcd.blink(); 
      }
    //Change rows using the up and down buttons
    if (bolUp == true) {
      if (intLineNumber > 0) {
        intLineNumber--; }}
    else if (bolDown == true) {
      if (intLineNumber < 2) {
        intLineNumber++; }}
    //Change the values by using the left and right buttons
    else if (bolRight == true) { //  Use right button to increase value
      if (intLineNumber == 1) {
        intDelay += 5; }  // increment PresetDrops by 100
      else if (intLineNumber == 2) {
        intDelayDown += 5; }  
      else if ((intLineNumber == 0) && (floCurrentLimit < CURRENT_SENSE_MAX_CURRENT))  {
        floCurrentLimit = floCurrentLimit + 0.1; 
        }  // increment current limit by 0.1
      }
    else if (bolLeft == true) {  //  Use left button to decrease value
      if (intLineNumber == 1) {
        intDelay -= 5; 
        if (intPresetDrops < 1) { // PresetDrops value of 100 drops is the lowest the user can choose
          intDelay = 5; }}
      else if (intLineNumber == 2) {
        intDelayDown -= 5; }  
      else if (intLineNumber == 0) {
        floCurrentLimit = floCurrentLimit - 0.1;   
        if (floCurrentLimit < 0.1) {  // Height value of 1 cm is the lowest the user can choose
          floCurrentLimit = 0.1; 
        }
      }
    }
    else if (bolA){
      //Clears all the errors
      bolOvercurrentError = false;
      bolLiftLimitError = false;
      bolPauseError = false;
      bolModuleStuckError = false;
      bolUnwindError = false;
      LCD_display_init();
    }
  }
}

//Clears the EEPROM of all drop counter data
void EEPROM_clear() {
  int Z = 0;
  for (int i = DROP_COUNTER_MEMORY_SPREAD_START; i <= DROP_COUNTER_MEMORY_SPREAD_STOP; i+=2) {
    EEPROM.put(i, Z); 
  }
}

//Returns the last drop counter value and sets the drop counter index
int EEPROM_read_last_count() {
// Find and save the last drop count before the program reset occurred
  int intMaxNum = 0;
  int intCurrentNum = 0;
  for (int i = DROP_COUNTER_MEMORY_SPREAD_START; i <= DROP_COUNTER_MEMORY_SPREAD_STOP; i+=2) {
    EEPROM.get(i, intCurrentNum);
    if (intCurrentNum > intMaxNum) {
      intMaxNum = intCurrentNum;
      intDropCounterMem = i; 
    }
  }
  return intMaxNum; 
}

//Manuel control of all machine aspects
void manual_control() {
  // Hold X to raise the module
  if (bolX == true) {
    digitalWrite(FORWARD, HIGH);  // Motor in forward motion
    digitalWrite(REVERSE, LOW);
    digitalWrite(ENABLE, HIGH);
  }
  
  // Hold Y to lower the module
  if (bolY == true) {
    digitalWrite(FORWARD, LOW); 
    digitalWrite(REVERSE, HIGH);
    digitalWrite(ENABLE, HIGH);  
  }  

  //Disables the motor  if nothing is pressed
  if (bolX == false && bolY == false){
    digitalWrite(FORWARD, LOW);
    digitalWrite(REVERSE, LOW);
    digitalWrite(ENABLE, LOW);  
  }

  // Press B to turn on and off the magnet
  if (bolB == true) {
    //Checks to see if the B button was release in a previous time
    if(bolBReleasedBefore == true){
      //Toggles the magnet on/off
      if (bolMagnetOn == false){
        digitalWrite(MAGNET, HIGH);
        bolMagnetOn = true;
        } 
      else if(bolMagnetOn == true){
        digitalWrite(MAGNET, LOW); 
        bolMagnetOn = false;
      }

      //Resets the released button flag
      bolBReleasedBefore = false;
    }
  }

  //Checks for B button release
  if (bolB == false){
    bolBReleasedBefore = true;
  }
  
  //Resets the drop counter
  if ((bolTriggerL == true) && (bolTriggerR == true)){
    EEPROM_clear();
    intDropCounter = 0;
  }
  
}

//Low pass filter for the current sense
void filterCurrentSense() {
  float floCurrentSenseReading = 0;
  
  //Clears the average
  floCurrentSenseAverage = 0;
   
  //Propagates the delay and feeds in the last value of the current sense
  for (int i =  0; i < (ANALOG_FILTER_ORDER-1); i++){
    //Moves the average from the N+1 to N
    floLPF[i] = floLPF[i+1];

    //Sums the average
    floCurrentSenseAverage = floCurrentSenseAverage +floLPF[i];
  }

  //Feeds the instantaneous current sense in
  floCurrentSenseReading = readMotorCurrent();
  floLPF[ANALOG_FILTER_ORDER-1] = floCurrentSenseReading;
  floCurrentSenseAverage = floCurrentSenseAverage + floCurrentSenseReading;

  //Performs the average
  floCurrentSenseAverage = (float)(floCurrentSenseAverage / ANALOG_FILTER_ORDER);
}

//State machine
void state_machine() {
  switch (intState) {
    //=================
    //Idle  
    //=================  
    case 0:
      // Allow for manual adjustment before starting cycle    
      manual_control();
      
      // Transistion to next state
      if (bolStart == true) {  // Press Start button to begin the state machine
        //EEPROM.put(PRESET_DROP_MEMORY_LOC, intPresetDrops);
        //EEPROM.put(HEIGHT_MEMORY_LOC, intHeight);
        EEPROM.put(CURRENT_SENSE_LIMIT_LOC, floCurrentLimit);
        EEPROM.put(DELAY_LOC, intDelay);
        EEPROM.put(DELAY_LOC_DOWN, intDelayDown);
        intState = 1;  // Reset State variable to move onto the next case
        delayStart = millis(); // start delay
        delayRunning = true; // not finished delay 
        
        //Resets the error flags
        bolOvercurrentError = false;
        bolLiftLimitError = false;
        bolPauseError = false;
        bolModuleStuckError = false;
        bolUnwindError = false;
        
        //Redraws the LCD
        LCD_display_init();
      }
      break;
      
    //================= 
    //Lift
    //=================      
    case 1:
      //Checks for time out on lift
      if (delayRunning && ((millis() - delayStart) >= (intDelay))){
        //Goes to the unwind state
        delayStart = millis(); // start delay
        intState = 3;
      }
      //Checks for overcurrent
      else if (abs(floCurrentSenseAverage) > floCurrentLimit){
        //Turns everything off and goes to idle state
        digitalWrite(FORWARD, LOW);  
        digitalWrite(REVERSE, LOW);
        digitalWrite(ENABLE, LOW);
        digitalWrite(MAGNET, LOW);  
        bolOvercurrentError = true;
        intState = 0;
      }
      //Runs current state
      else {
        // Execute current state
        // Wind cable up to correct height until switch is pressed
        digitalWrite(FORWARD, HIGH);  // Motor in forward motion
        digitalWrite(REVERSE, LOW);
        digitalWrite(ENABLE, HIGH);
        digitalWrite(MAGNET, LOW);  
      }
      break;     
      
    //=================
    //Unwind 
    //=================     
    case 3:
      delayRunning = true; // not finished delay yet
      // Unwind some cable with extra slack    
      digitalWrite(FORWARD, LOW); 
      digitalWrite(REVERSE, HIGH);
      digitalWrite(ENABLE, HIGH);
      digitalWrite(MAGNET, HIGH); 

      
      //Checks to see if we're done unwinding
      if (delayRunning && ((millis() - delayStart) >= intDelayDown)) {
        delayRunning = true;
        delayStart = millis(); // start delay
        intState = 1;
      }
      break;
      
    //=================
    //Default
    //=================
    default:
      intState = 0;  // Return to Idle state
      break; 
  }

  //Checks for abort
  if (bolSelect == true){
    //Turns everything off and goes to idle state
    digitalWrite(FORWARD, LOW);  
    digitalWrite(REVERSE, LOW);
    digitalWrite(ENABLE, LOW);
    digitalWrite(MAGNET, LOW);  
    delayRunning = false;
    bolPauseError = true;
    intState = 0;
  }
}



void setup() {  
// Set up all pinModes
  pinMode(ENABLE, OUTPUT);
  pinMode(FORWARD, OUTPUT);
  pinMode(REVERSE, OUTPUT);
  pinMode(MAGNET, OUTPUT);
  pinMode(LIMITSWITCH, INPUT);
  digitalWrite(LIMITSWITCH, HIGH);
  pinMode(A,INPUT);
  digitalWrite(A,HIGH);
  pinMode(B,INPUT);
  digitalWrite(B,HIGH);
  pinMode(X,INPUT);
  digitalWrite(X,HIGH);
  pinMode(Y,INPUT);
  digitalWrite(Y,HIGH);
  pinMode(START,INPUT);
  digitalWrite(START,HIGH);
  pinMode(SELECT,INPUT);
  digitalWrite(SELECT,HIGH);
  pinMode(LEFT,INPUT);
  digitalWrite(LEFT,HIGH);
  pinMode(RIGHT,INPUT);
  digitalWrite(RIGHT,HIGH);
  pinMode(UP,INPUT);
  digitalWrite(UP,HIGH);
  pinMode(DOWN,INPUT);
  digitalWrite(DOWN,HIGH);
  pinMode(START,INPUT);
  digitalWrite(START,HIGH);
  pinMode(SELECT,INPUT);
  digitalWrite(SELECT,HIGH);
  pinMode(LR,INPUT);
  digitalWrite(LR,HIGH);
  pinMode(LOGIC_LOW, OUTPUT);
  digitalWrite(LOGIC_LOW,LOW);
  pinMode(ERASE_EEPROM,INPUT);
  digitalWrite(ERASE_EEPROM,HIGH);

  //Sets up the SNES controller
  snes.begin();
  snes.connect();

  
  //Clear EEPROM values
  if (digitalRead(ERASE_EEPROM)== LOW) {
    EEPROM_clear(); 
    }
    
  //Initialize LCD display
  LCD_display_init();

  //Begin Serial connection
  Serial.begin(115200);   
  
  //Set up EEPROM value storage
  //EEPROM.get(HEIGHT_MEMORY_LOC, intHeight);
  //EEPROM.get(PRESET_DROP_MEMORY_LOC, intPresetDrops);
  EEPROM.get(CURRENT_SENSE_LIMIT_LOC, floCurrentLimit);
  EEPROM.get(DELAY_LOC, intDelay);
  EEPROM.get(DELAY_LOC_DOWN, intDelayDown);

  //Checks for weird numbers
  if ((intDelay > 0) && (intDelay < 2000)){
    //Do nothing
  }
  else{
    //Sets delay to 100
    intDelay = 100;
  }

  //Checks for weird numbers
  if ((intDelayDown > 0) && (intDelayDown < 2000)){
    //Do nothing
  }
  else{
    //Sets delay to 100
    intDelayDown = 100;
  }

}


void loop() {    
  //Read all user inputs   
  readIO();  

  //Reads current sense and filters
  filterCurrentSense();

  //Prints debug data
  print_IO(); 
  
  //Run display delay every 200 micro-sec while running alongside a fast 10 micro-sec loop
  if (intDisplayCounter < 20) {
    intDisplayCounter++; 
    }
  else {
    LCD_display_refresh();
    LCD_user_interface();
    intDisplayCounter = 0; 
   }
   
  //Run state machine and set delay  
  state_machine();

  //Small delay 
  //delay(10);
}
