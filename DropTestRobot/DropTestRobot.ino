// LCD Settings
#include <LiquidCrystal_PCF8574.h>
#include <Wire.h>
#include <EEPROM.h>
LiquidCrystal_PCF8574 lcd(0x27); // set the LCD address to 0x27 for a 16 chars and 2 line display

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
#define DROP_COUNTER_MEMORY_SPREAD_START 10
#define DROP_COUNTER_MEMORY_SPREAD_STOP 4000
#define LOGIC_LOW 12
#define ERASE_EEPROM 13
#define OVERCURRENT_LIMIT 630
#define ANALOG_FILTER_ORDER 20
#define LIFT_TIMEOUT_ADDITION_MS 500
#define DROP_DELAY_MS 300
#define UNWIND_MS_PER_CM 125

// Define variables
int intState = 0;
int intA = 1;
int intB = 1;
int intX = 1;
int intY = 1;
int intUp = 1;
int intDown = 1;
int intLeft = 1;
int intRight = 1;
int intStart = 1;
int intSelect = 1;
//int intPause = 0;
int intLR = 1;
int intHeight = 0; // height in cm
int intDropCounter = 0;
int intPresetDrops = 0;  
int intLineNumber = 1; // row number
int intDisplayCounter = 0;
int intDropCounterMem = 10;
int intCurrentSense = 0;  // current sensor
int intLPF[ANALOG_FILTER_ORDER];
int intCurrentSenseAverage = 0;
int intUnwindDelay = 1000; 
int intHoldDelay = 300;  // 1 second delay
unsigned long delayStart = 0; // the time the delay started
bool delayRunning = false; // true if still waiting for delay to finish
bool bolMagnetOn = false;
bool bolBReleasedBefore = true;


// Create functions

void readIO() {  
// Read all IO pins/user inputs from controller 
  intA = digitalRead(A);
  intB = digitalRead(B);
  intX = digitalRead(X);
  intY = digitalRead(Y);
  intUp = digitalRead(UP);
  intDown = digitalRead(DOWN);
  intLeft = digitalRead(LEFT);
  intRight = digitalRead(RIGHT);
  intStart = digitalRead(START);
  intSelect = digitalRead(SELECT);
  intLR = digitalRead(LR);
  intCurrentSense = analogRead(A1); 
  }


void print_IO() {
  Serial.print("; X:");
  Serial.print(intX);
  Serial.print("; Y:");
  Serial.print(intY);
  Serial.print("; A:");
  Serial.print(intA);
  Serial.print("; B:");
  Serial.print(intB);
  Serial.print("; Up:");
  Serial.print(intUp);
  Serial.print("; Down:");
  Serial.print(intDown);
  Serial.print("; Left:");
  Serial.print(intLeft);
  Serial.print("; Right:");
  Serial.print(intRight);
  Serial.print("; LR: ");
  Serial.print(intLR);
  Serial.print("; Select: ");
  Serial.print(intSelect);

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
  
  Serial.print("; Current Sense: ");
  Serial.print(intCurrentSense);
  Serial.print("; Current Sense Average: ");
  Serial.print(intCurrentSenseAverage);
  Serial.print("; Limit Switch: ");
  Serial.print(digitalRead(LIMITSWITCH));
  Serial.println("");
}


void LCD_display_init() {
// Initialize the text on the LCD screen
  lcd.begin(20, 4); // Initialize the LCD screen dimensions
  lcd.setBacklight(255);  // LCD settings
  lcd.home();
  lcd.clear();
  lcd.print("Magnet: ");  
  lcd.setCursor(0,1);
  lcd.print("Preset Drops:");
  lcd.setCursor(0,2);
  lcd.print("Height (cm):");
  lcd.setCursor(0,3);
  lcd.print("Drop Counter:");
  delay(10); }
  
void LCD_display_refresh() {
// Refresh the LCD screen to set the location of the cursor, blank the text, and then print the label
  lcd.setCursor(8,0);
  if (digitalRead(MAGNET) == LOW){
    lcd.print("Off");
  }
  else{
   lcd.print("On ");
  }
  
  lcd.setCursor(14,1);  // Update line 1
  lcd.print("      ");  // Blank the text 
  lcd.setCursor(14,1);
  lcd.print(intPresetDrops);  // Replace the blank text with the accurate value
  lcd.setCursor(13,2);  // Update line 2
  lcd.print("       ");
  lcd.setCursor(13,2);
  lcd.print(intHeight);
  lcd.setCursor(14,3);  // Update line 3
  lcd.print("      ");
  lcd.setCursor(14,3);
  lcd.print(intDropCounter);   
  }


void LCD_user_interface() {
// Change user inputs based on arrow pad and display on LCD screen
  if (intState == 0) {
    //Set cursor position for each line
    if (intLineNumber == 1) {
      lcd.setCursor(14,1);
      lcd.blink(); 
      }  // Set blinking cursor
    else if (intLineNumber == 2) {
      lcd.setCursor(13,2);
      lcd.blink(); 
      }
    //Change rows using the up and down buttons
    if (intUp == 0) {
      if (intLineNumber > 1) {
        intLineNumber--; }}
    else if (intDown == 0) {
      if (intLineNumber < 2) {
        intLineNumber++; }}
    //Change the values by using the left and right buttons
    else if (intRight == 0) { //  Use right button to increase value
      if (intLineNumber == 1) {
        intPresetDrops += 100; }  // increment PresetDrops by 100
      else if (intLineNumber == 2) {
        intHeight++; }}  // increment Height by 1
    else if (intLeft == 0) {  //  Use left button to decrease value
      if (intLineNumber == 1) {
        intPresetDrops -= 100;  
        if (intPresetDrops < 100) { // PresetDrops value of 100 drops is the lowest the user can choose
          intPresetDrops = 100; }}
      else if (intLineNumber == 2) {
        intHeight--;  
        if (intHeight < 1) {  // Height value of 1 cm is the lowest the user can choose
          intHeight = 1; 
        }
      }
    }
  }
}


void EEPROM_clear() { 
// Short Arduino pins 12 and 13 every time you run on a new Arduino to reset the EEPROM
  int Z = 0;
  for (int i = DROP_COUNTER_MEMORY_SPREAD_START; i <= DROP_COUNTER_MEMORY_SPREAD_STOP; i+=2) {
    EEPROM.put(i, Z); 
  }
}


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


void manual_control() {
  // Hold X to raise the module
  if (intX == 0) {
    digitalWrite(FORWARD, HIGH);  // Motor in forward motion
    digitalWrite(REVERSE, LOW);
    digitalWrite(ENABLE, HIGH);
  }
  
  // Hold Y to lower the module
  if (intY == 0) {
    digitalWrite(FORWARD, LOW);  // Motor in reverse
    digitalWrite(REVERSE, HIGH);
    digitalWrite(ENABLE, HIGH);  
  }  

  //Disables the motor  if nothing is pressed
  if (intX == 1 && intY == 1){
    digitalWrite(FORWARD, LOW);
    digitalWrite(REVERSE, LOW);
    digitalWrite(ENABLE, LOW);  
  }

  // Press B to turn on and off the magnet
  if (intB == 0) {
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
  if (intB == 1){
    bolBReleasedBefore = true;
  }
  
  //Resets the drop counter
  if (intA == 0 && intSelect == 0){
    
    EEPROM_clear();
    intDropCounter = 0;
  }
  
  
  
}

void filterCurrentSense() {
  //Clears the average
  intCurrentSenseAverage = 0;
   
  //Propagates the delay and feeds in the last value of the current sense
  for (int i =  0; i < (ANALOG_FILTER_ORDER-1); i++){
    //Moves the average from the N+1 to N
    intLPF[i] = intLPF[i+1];

    //Sums the average
    intCurrentSenseAverage = intCurrentSenseAverage + intLPF[i];
  }

  //Feeds the instantaneous current sense in
  intLPF[ANALOG_FILTER_ORDER-1] = intCurrentSense;
  intCurrentSenseAverage = intCurrentSenseAverage + intCurrentSense;

  //Performs the average
  intCurrentSenseAverage = int(intCurrentSenseAverage / ANALOG_FILTER_ORDER);
}


void state_machine() {
  //Serial.println(intState);
  switch (intState) {
// Idle    
    case 0:
      // Allow for manual adjustment before starting cycle    
      manual_control();
      
      // Transistion to next state
      if (intStart == 0) {  // Press Start button to begin the state machine
        EEPROM.put(PRESET_DROP_MEMORY_LOC, intPresetDrops);
        EEPROM.put(HEIGHT_MEMORY_LOC, intHeight);
        intState = 1;  // Reset State variable to move onto the next case
        delayStart = millis(); // start delay
        delayRunning = true; // not finished delay yet

        //Calculates the unwind time
        intUnwindDelay = int(UNWIND_MS_PER_CM*intHeight);
      }
      break; 
// Lift      
    case 1:
      // Transistion to next state
      if (digitalRead(LIMITSWITCH) == HIGH) { // If switch is pressed, move to next state      
        intState = 2; 
        // Start millisDelay
        delayStart = millis(); // start delay
      }
      else if (delayRunning && ((millis() - delayStart) >= (LIFT_TIMEOUT_ADDITION_MS+intUnwindDelay))){
        //Turns everything off and goes to idle state
        digitalWrite(FORWARD, LOW);  
        digitalWrite(REVERSE, LOW);
        digitalWrite(ENABLE, LOW);
        digitalWrite(MAGNET, LOW);
        delayRunning = false; 
        intState = 0;
      }
      //Checks for overcurrent
      else if (intCurrentSenseAverage > OVERCURRENT_LIMIT){
        //Turns everything off and goes to idle state
        digitalWrite(FORWARD, LOW);  
        digitalWrite(REVERSE, LOW);
        digitalWrite(ENABLE, LOW);
        digitalWrite(MAGNET, LOW);  
        intState = 0;
      }
      else {
        // Execute current state
        // Wind cable up to correct height until switch is pressed
        digitalWrite(FORWARD, HIGH);  // Motor in forward motion
        digitalWrite(REVERSE, LOW);
        digitalWrite(ENABLE, HIGH);
        digitalWrite(MAGNET, LOW);  // Magnet off
      }
      
      
      break;       
// Hold
    case 2:
      
      delayRunning = true; // not finished delay yet
      // Turn magnet on and hold the module in the air
      digitalWrite(FORWARD, LOW);  // Motor off
      digitalWrite(REVERSE, LOW);
      digitalWrite(ENABLE, LOW);
      digitalWrite(MAGNET, HIGH);  // Magnet on
      // check if delay has timed out after 1 sec
      if (delayRunning && ((millis() - delayStart) >= intHoldDelay)) {
        delayRunning = false; // // prevent this code being run more then once
        intState = 3;
        // Start millisDelay
        delayStart = millis(); // start delay
      }
      
      break;
// Unwind      
    case 3:
      delayRunning = true; // not finished delay yet
      // Unwind some cable with extra slack    
      digitalWrite(FORWARD, LOW);  // Motor in reverse
      digitalWrite(REVERSE, HIGH);
      digitalWrite(ENABLE, HIGH);
      digitalWrite(MAGNET, HIGH);  // Magnet on
      // check if delay has timed out after 2 sec
      if (delayRunning && ((millis() - delayStart) >= intUnwindDelay)) {
        delayRunning = false; // // prevent this code being run more then once
        intState = 4;
      }
      
      break;
// Drop      
    case 4:
    // Turn magnet off and drop module    
      digitalWrite(FORWARD, LOW);  // Motor off
      digitalWrite(REVERSE, LOW);
      digitalWrite(ENABLE, LOW);
      digitalWrite(MAGNET, LOW);  // Magnet off
      
    // Loop until preferred number of drops is reached
      if (intDropCounter < intPresetDrops) {  
        intState = 5; 
        intDropCounter++;
        EEPROM.put(intDropCounterMem, intDropCounter);
        intDropCounterMem += 2;
        if (intDropCounterMem > DROP_COUNTER_MEMORY_SPREAD_STOP) {  // If exceed memory capacity, reset and overwrite data from the beginning
          intDropCounterMem = DROP_COUNTER_MEMORY_SPREAD_START; }

        // Start millisDelay
        delayStart = millis(); // start delay
        delayRunning = true; 
      }
      else {
        intState = 0; }

      break;

// Drop delay
    case 5:
      if (delayRunning && ((millis() - delayStart) >= DROP_DELAY_MS)) {
        delayRunning = false; //
        intState = 1;
      } 
      
      break; 
// Default
    default:
      intState = 0;  // Return to Idle state
      break; 
  }



  //Checks for emergency abort
  if (intSelect == 0){
    //Turns everything off and goes to idle state
    digitalWrite(FORWARD, LOW);  
    digitalWrite(REVERSE, LOW);
    digitalWrite(ENABLE, LOW);
    digitalWrite(MAGNET, LOW);  
    delayRunning = false;
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
// Clear EEPROM values
  if (digitalRead(ERASE_EEPROM)== LOW) {
    EEPROM_clear(); }
// Initialize LCD display
  LCD_display_init();
  Serial.begin(115200);   // Begin Serial connection
// Set up EEPROM value storage
  EEPROM.get(HEIGHT_MEMORY_LOC, intHeight);
  EEPROM.get(PRESET_DROP_MEMORY_LOC, intPresetDrops);
  intDropCounter = EEPROM_read_last_count(); 
}


void loop() {       
  readIO();  // Read all user inputs
  filterCurrentSense();
  print_IO(); 
  
  //Run display delay every 200 micro-sec while running alongside a fast 10 micro-sec loop
  if (intDisplayCounter < 20) {
    intDisplayCounter++; }
  else {
    LCD_display_refresh();
    LCD_user_interface();
    intDisplayCounter = 0; 
   }
   
  //Run state machine and set delay  
  state_machine();
  delay(10);
}
  

// Things to finish: Pause, Magnet on/off, E-stop, Reset drop counter
