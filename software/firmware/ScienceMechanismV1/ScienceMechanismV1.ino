#define RS485 Serial1 //Select RS-485 Serial Port.                //This needs to be changed based on the design
//#define POTHOS_DEBUG  //Comment this out for extra efficiency. Leave as is for verbose debug statements over USB.
#include <pothos.h> //Include pothos library

enum PIN // Enum of pinouts
{
  EN485 = 32,    // RS485 Enable Pin (O)
  BLUE_LED = 13, // Blue LED source (O)

  // First motor controller to LARY (VNH7100AS)
  INA_1 = 23,  // Clockwise input (O)
  INB_1 = 22,  // Counter clockwise input (O)
  PWM_1 = 9,   // Speed control (O)
  CS_1 = 15,   // Current sense (I)
  SEL_1 = 25, // Addresses current sense (I)
  TEMP_1 = 14, // 1.27M -- TEMP -- 100K Thermistor (I)

  // Second motor controller to LARY (VNH7100AS)
  INA_2 = 4,  // Clockwise input (O)
  INB_2 = 16, // Counter clockwise input (O)
  PWM_2 = 3,  // Speed control (O)
  CS_2 = 17,  // Current sense (I)
  SEL_2 = 1, // Addresses current sense (I)
  TEMP_2 = 1, // 1.27M -- TEMP -- 100K Thermistor (I)

  // Limit switches (directly to connector, externally pulled low)
  LIM_1 = 29, // Limit switch 1 (I)
  LIM_2 = 27, // Limit switch 2 (I)
  LIM_3 = 28, // Limit switch 3 (I)
  LIM_4 = 12, // Limit switch 4 (I)

  // Video mux (CD74HC4052M96, externally pulled low)
  VID_SELECT_1 = 26, // Select bit 0 (O)
  VID_SELECT_2 = 31, // Select bit 1 (O)

  // Stepper motor driver (A4988)
  DIR = 24,  // Direction of rotation (O)
  STEP = 33, // Increment one step (LOW -> HIGH) (O)

  //Lazer MOS (lowside NMOS gate)
  LAZ_EN = 7, // Lazer enable (O)

  // Servos (directly to connector)
  SERVO_1 = 6,  // Servo 1 (O)
  SERVO_2 = 10, // Servo 2 (O)

  // LED Enable (lowside NMOS gate)
  LED_EN = 11, // Enables lowside gate (O)

  // Pump control (SN74AHCT138 Decoder fed into 74HC9114D Inverter)
  PUMP_EN = 8,        // Enable (O)
  PUMP_SELECT_1 = 5,  // Select bit A (LSB) (O)
  PUMP_SELECT_2 = 21, // Select bit B (O)
  PUMP_SELECT_3 = 20  // Select bit C (O)
};

enum REGISTER // Enum of register addresses
{
  // First motor controller registers
  SPEED_1 = 0,
  DIR_1 = 1,
  TMP_1 = 2,
  CURRENT_1 = 3,

  // Second motor controller registers
  SPEED_2 = 4,
  DIR_2 = 5,
  TMP_2 = 6,
  CURRENT_2 = 7,

  // Video selection register (0 - 3)
  VID_SELECT = 8
};

uint32_t updateTimer = 0;
int updateTime = 50;

int pothosTimeout = 50; // The recommended pothos timeout is 50 ms
uint8_t slaveID = 11;   // The slave ID for the node (1-255)

pothos comms(slaveID, PIN::EN485, pothosTimeout); // Init the pothos library w/o RGB LED

void setup()
{
  comms.setup(500000); //Pothos will communicate at a baudrate of 500Kbps      //This is high enough to ensure that data speeds will never be the bottleneck.

#ifdef POTHOS_DEBUG
  Serial.begin(115200);
#endif

  setPinModes();  // Sets all pinmodes
  setDataTypes(); // Sets all pothos data types
}

void loop()
{
  comms.update(); // Maintains communication over pothos
   if(millis()-updateTimer >= updateTime){       //update timer to trigger at 20Hz, This saves a vast majority of processing power for watching communicatrions
    updateTimer = millis();
    driveVertical();
    driveDrill();
    setVideoSelect();
    comms.data.set_data(REGISTER::TMP_1, readTempOne());
    comms.data.set_data(REGISTER::TMP_2, readTempTwo());
    comms.data.set_data(REGISTER::CURRENT_1, readCurrentOne());
    comms.data.set_data(REGISTER::CURRENT_2, readCurrentTwo());
  }
}

// This function will set the pinmode of all non-pothos pins (exclude 485-enable, Rx, Tx, and RGBLED pins)
void setPinModes()
{
  pinMode(PIN::BLUE_LED, OUTPUT); // Sets the blue LED on pin 13 to an output

  // First motor controller to LARY (VNH7100AS)
  pinMode(PIN::INA_1, OUTPUT); // Clockwise input (O)
  pinMode(PIN::INB_1, OUTPUT); // Counter clockwise input (O)
  pinMode(PIN::PWM_1, OUTPUT); // Speed control (O)
  pinMode(PIN::CS_1, INPUT);   // Current sense (I)
  pinMode(PIN::SEL_1, INPUT); // Addresses current sense (I)
  pinMode(PIN::TEMP_1, INPUT); // 1.27M -- TEMP -- 100K Thermistor (I)

  // Second motor controller to LARY (VNH7100AS)
  pinMode(PIN::INA_2, OUTPUT); // Clockwise input (O)
  pinMode(PIN::INB_2, OUTPUT); // Counter clockwise input (O)
  pinMode(PIN::PWM_2, OUTPUT); // Speed control (O)
  pinMode(PIN::CS_2, INPUT);   // Current sense (I)
  pinMode(PIN::SEL_2, INPUT); // Addresses current sense (I)
  pinMode(PIN::TEMP_2, INPUT); // 1.27M -- TEMP -- 100K Thermistor (I)

  // Limit switches (directly to connector, externally pulled low)
  pinMode(PIN::LIM_1, INPUT); // Limit switch 1 (I)
  pinMode(PIN::LIM_2, INPUT); // Limit switch 2 (I)
  pinMode(PIN::LIM_3, INPUT); // Limit switch 3 (I)
  pinMode(PIN::LIM_4, INPUT); // Limit switch 4 (I)

  // Video mux (CD74HC4052M96, externally pulled low)
  pinMode(PIN::VID_SELECT_1, OUTPUT); // Select bit 0 (O)
  pinMode(PIN::VID_SELECT_2, OUTPUT); // Select bit 1 (O)

  // Stepper motor driver (A4988)
  pinMode(PIN::DIR, OUTPUT);  // Direction of rotation (O)
  pinMode(PIN::STEP, OUTPUT); // Increment one step (LOW -> HIGH) (O)

  //Lazer MOS (lowside NMOS gate) NOT USED
  pinMode(PIN::LAZ_EN, OUTPUT); // Lazer enable (O)

  // Servos (directly to connector)
  pinMode(PIN::SERVO_1, OUTPUT); // Servo 1 (O)
  pinMode(PIN::SERVO_2, OUTPUT); // Servo 2 (O)

  // LED Enable (lowside NMOS gate)
  pinMode(PIN::LED_EN, OUTPUT); // Enables lowside gate (O)

  // Pump control (SN74AHCT138 Decoder fed into 74HC9114D Inverter) NOT USED
  pinMode(PIN::PUMP_EN, OUTPUT);       // Enable (O)
  pinMode(PIN::PUMP_SELECT_1, OUTPUT); // Select bit A (LSB) (O)
  pinMode(PIN::PUMP_SELECT_2, OUTPUT); // Select bit B (O)
  pinMode(PIN::PUMP_SELECT_3, OUTPUT); // Select bit C (O)
}

void setDataTypes()
{ // This function is for setting the data type of each register
  // comms.data.set_type(REGISTER::LED, "char");           // Chars are actually unsigned 8bit integers in disquise and are the closest thing to a bool that's supported by pothos
  // comms.data.set_type(REGISTER::TMP, "int");            // The temperature data is an int
  // comms.data.set_type(REGISTER::TIME_DATA, "long");     // Longs are also supported. time is often a long.
  // comms.data.set_type(REGISTER::TMP_DATA, "float");     // Floats are also supported
}

void driveVertical(){
    bool direct = (comms.data.get_char_data(REGISTER::DIR_1) != '\0');
    uint8_t motorSpeed = int(comms.data.get_char_data(REGISTER::SPEED_1));

    // Check limit switches
    if(digitalRead(PIN::LIM_1) && direct){
      motorSpeed = 0;
      
    }else if(digitalRead(PIN::LIM_2) && !direct){
      motorSpeed = 0;
      
    }
    
    digitalWrite(PIN::INA_1, direct);
    digitalWrite(PIN::SEL_1, direct);
    digitalWrite(PIN::INB_1, !direct);
    analogWrite(PIN::PWM_1, motorSpeed);
}

void driveDrill(){
    bool direct = (comms.data.get_char_data(REGISTER::DIR_2) != '\0');
    uint8_t motorSpeed = int(comms.data.get_char_data(REGISTER::SPEED_2));
    digitalWrite(PIN::INA_2, direct);
    digitalWrite(PIN::SEL_2, direct);
    digitalWrite(PIN::INB_2, !direct);
    analogWrite(PIN::PWM_2, motorSpeed);
}

void setVideoSelect(){
  uint8_t selected = int(comms.data.get_char_data(REGISTER::VID_SELECT));
  digitalWrite(PIN::VID_SELECT_1, (selected & 0x1)); // Get first bit of selected
  digitalWrite(PIN::VID_SELECT_2, (selected & 0x2)); // Get second bit of selected
}

float readTempOne(){
  int raw = analogRead(PIN::TEMP_1);
  float tempC = (((raw/1023.0)*3.3)-0.4)/.0195;
  return(tempC);
}

float readTempTwo(){
  int raw = analogRead(PIN::TEMP_2);
  float tempC = (((raw/1023.0)*3.3)-0.4)/.0195;
  return(tempC);
}

float readCurrentOne(){                                    //current reading is only accurate at > 200mA
  int raw = analogRead(PIN::CS_1);
  float current = ((raw/1023.0)*3.3)*(112.0/43.0);
  return(current);
}

float readCurrentTwo(){                                    //current reading is only accurate at > 200mA
  int raw = analogRead(PIN::CS_2);
  float current = ((raw/1023.0)*3.3)*(112.0/43.0);
  return(current);
}