#include "arduino_secrets.h"
/* 
  Sketch generated by the Arduino IoT Cloud Thing "IoT Based Server Room Monitoring System"
  https://create.arduino.cc/cloud/things/116a7e43-bc11-4230-a16c-e7cf2942eaec 

  Arduino IoT Cloud Variables description

  The following variables are automatically generated and updated when changes are made to the Thing

  String server_room_data;
  float ac_mains_voltage;
  float barometric_pressure;
  float humidity;
  float temperature;
  bool automatic_mode;
  bool cooler_control;
  bool fire_alert_status;
  bool heater_control;
  bool power_failure_status;
  bool smoke_alert_status;

  Variables which are marked as READ/WRITE in the Cloud Thing will also have functions
  which are called when their values are changed from the Dashboard.
  These functions are generated with the Thing and added at the end of this sketch.
*/
#include <Arduino.h>
#include "thingProperties.h"
#include <Arduino_MKRIoTCarrier.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

MKRIoTCarrier carrier;

#define BOTtoken "xxxxxxxxxxx:xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" // your Bot Token (Get from Botfather)
String Mychat_id = "xxxxxxxxxx";
String bot_name = "IoTserverRoom_bot"; // you can change this to whichever is your bot name

WiFiSSLClient client;

UniversalTelegramBot bot(BOTtoken, client);

#define VOLT_CAL 1871

EnergyMonitor emon1;             // Create an instance

File dataFile;

int smoke_sensor_pin = A5;
int voltage_sensor_pin = A6;
int fire_sensor_pin = 6;

unsigned long startAlert, countAlert;
bool FirstAlert = true;
int AlertLimit = 2;
const long alertDuration = 300 * 1000;

 
uint32_t greenColor = carrier.leds.Color( 255, 0, 0);
uint32_t redColor = carrier.leds.Color( 0, 255, 0);
uint32_t blueColor = carrier.leds.Color( 0, 0, 255);
uint32_t noColor = carrier.leds.Color( 0, 0, 0);
 

void setup() {
  // Initialize serial and wait for port to open:
  Serial.begin(9600);
  // This delay gives the chance to wait for a Serial Monitor without blocking if none is found
  pinMode(fire_sensor_pin, INPUT);
  delay(1500); 

  // Defined in thingProperties.h
  initProperties();

  // Connect to Arduino IoT Cloud
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  //Get Cloud Info/errors , 0 (only errors) up to 4
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();
 
  //Wait to get cloud connection to init the carrier
  while (ArduinoCloud.connected() != 1) {
    ArduinoCloud.update();
    delay(500);
  }

  // Open up the file we're going to log to!
  dataFile = SD.open("datalog.txt", FILE_WRITE);
  if (! dataFile) {
    Serial.println("error opening datalog.txt");
    // Wait forever since we cant write data
    while (1) ;
  }
  dataFile.println("Temperature,Humidity,Pressure,acPower,Fire,Smoke");

  delay(500);
  CARRIER_CASE = false;
  carrier.begin();
  emon1.voltage(A6, VOLT_CAL, 1.7);  // Voltage: input pin, calibration, phase_shift
  carrier.display.setRotation(0);
  delay(1500);
  displayInitMsg();
  defaultPage();
}

void loop() {
  emon1.calcVI(25,1000);         // Calculate all. No.of half wavelengths (crossings), time-out
  ac_mains_voltage = emon1.Vrms;             //extract Vrms into Variable
  // Your code here 
  readSensors();
  defaultPage();
  displaySensorData();
  CheckForGesture();
  thermostatControl();
  ArduinoCloud.update();
}


void readSensors(){
  //Read status and values of all sensors

  temperature = carrier.Env.readTemperature();
  humidity = carrier.Env.readHumidity();
  barometric_pressure = carrier.Pressure.readPressure();

  if(ac_mains_voltage < 200){
    
    power_failure_status = 1;
    if (power_failure_status == HIGH) { 
      if (FirstAlert) {
        startAlert = millis(); 
        countAlert = 0;
        FirstAlert = false;
      }
      countAlert += 1;
      if (millis() - startAlert > alertDuration) {
        FirstAlert = true;
        if (countAlert >= AlertLimit) { 
          bot.sendMessage(Mychat_id, "Power Failure", "");
        }
     } 
    }
    }
  else{
    power_failure_status = 0;
  }

  bool smoke = digitalRead(smoke_sensor_pin);
  if(smoke == 0){
    smoke_alert_status = 1;
  }
  else{
    smoke_alert_status = 0;
  }

  bool fire = digitalRead(fire_sensor_pin);
  if(fire == 0){
    fire_alert_status = 1;
  }
  else{
    fire_alert_status = 0;
  }

  server_room_data = String(temperature) + "," + String(humidity) + "," + String(barometric_pressure) + "," + String(ac_mains_voltage); 
  dataFile.println(server_room_data);
  dataFile.flush();
  delay(1000);
}

void displayInitMsg(){
  carrier.display.fillScreen(0x0000);
  //Basic configuration for the text
  carrier.display.setRotation(0);
  carrier.display.setTextWrap(true);
  carrier.display.drawBitmap(50, 40, arduino_logo, 150, 106, 0x253); //0x00979D);
  carrier.display.setTextColor(0xFFFF);
  carrier.display.setTextSize(3);
  carrier.display.setCursor(35, 160);
  carrier.display.print("Loading");
  //Printing a three dots animation
  for (int i = 0; i < 3; i++)
  {
    carrier.display.print(".");
    delay(1000);
  }

  carrier.display.fillScreen(0x0000);
  carrier.display.setTextColor(0xFFFF);
  carrier.display.setTextSize(2);
  carrier.display.setCursor(55, 30);
  carrier.display.print("Arduino IoT");
  carrier.display.setTextSize(2);
  carrier.display.setCursor(15, 60);
  carrier.display.print("CloudGames Contest");

  carrier.display.setTextSize(2);
  carrier.display.setTextColor(0x07FF);
  carrier.display.setCursor(24, 120);
  carrier.display.println("IoT Based Server");
  carrier.display.setCursor(25, 140);
  carrier.display.println("Room Monitoring");
  carrier.display.setCursor(75, 160);
  carrier.display.println("System");
  delay(4500);
}


void displaySensorData(){
  carrier.Buttons.update();
 
  if (carrier.Buttons.onTouchDown(TOUCH0)) {
    showTemperature();
  }
 
  if (carrier.Buttons.onTouchDown(TOUCH1)) {
    showHumidity();
  }

  if (carrier.Buttons.onTouchDown(TOUCH2)) {
    showPressure();
  }

  if (carrier.Buttons.onTouchDown(TOUCH3)) {
    showVoltage();
  }

}


void defaultPage(){
  // Default Page shown on Display of Opla Kit
  carrier.display.fillScreen(0x0000);
  carrier.display.setTextColor(0xFFFF);
  carrier.display.setTextSize(2);
  carrier.display.setCursor(44, 40);
  carrier.display.print("Press Buttons");
  carrier.display.setCursor(24, 70);
  carrier.display.print("to Get the Sensor");
  carrier.display.setCursor(70, 100);
  carrier.display.setTextSize(2);
  carrier.display.print("Readings");
  carrier.display.drawBitmap(70, 145, gesture_logo, 100, 100, 0x0D14); //0x0CA1A6);
  delay(100);
}

void displayReadings(){

    carrier.display.fillScreen(ST77XX_MAGENTA);
    carrier.display.setTextColor(ST77XX_WHITE);
    carrier.display.setTextSize(3);

  
    carrier.display.setCursor(30, 45);
    carrier.display.print("T: ");
    carrier.display.print(temperature);
    carrier.display.print(" C");
    
  
    carrier.display.setCursor(30, 85);
    carrier.display.print("H: ");
    carrier.display.print(humidity);
    carrier.display.print(" %");


    carrier.display.setCursor(30, 125);
    carrier.display.print("P: ");
    carrier.display.print(barometric_pressure);
    carrier.display.print(" kP");
    

    carrier.display.setCursor(30, 165);
    carrier.display.print("V: ");
    carrier.display.print(ac_mains_voltage);
    carrier.display.print(" V");

    delay(2500);
    defaultPage();
}

void CheckForGesture(){

    if (carrier.Light.gestureAvailable())
    {
      uint8_t gesture = carrier.Light.readGesture(); // a variable to store the type of gesture read by the light sensor
      // when gesture is UP
      if ((gesture == UP) || (gesture == DOWN) || (gesture == RIGHT) || (gesture == UP))
      {
        displayReadings();
      }

    }
}

void showTemperature(){
  // displaying temperature
  carrier.display.fillScreen(0x0000);
  carrier.display.setCursor(25, 60);
  carrier.display.setTextSize(3);
  carrier.display.print("Temperature");
  float temperature = carrier.Env.readTemperature();   //storing temperature value in a variable
  carrier.display.drawBitmap(80, 80, temperature_logo, 100, 100, 0xDAC9); //0xDA5B4A); //draw a thermometer on the MKR IoT carrier's display
  carrier.display.setCursor(60, 180);
  carrier.display.print(temperature); // display the temperature on the screen
  carrier.display.print(" C");
  delay(2500);
  defaultPage();
}

void showHumidity(){
  // displaying humidity
  carrier.display.fillScreen(0x0000);
  carrier.display.setCursor(54, 40);
  carrier.display.setTextSize(3);
  carrier.display.print("Humidity");
  carrier.display.drawBitmap(70, 70, humidity_logo, 100, 100, 0x0D14); //0x0CA1A6); //draw a humidity figure on the MKR IoT carrier's display
  float humidity = carrier.Env.readHumidity(); //storing humidity value in a variable
  carrier.display.setCursor(60, 180);
  carrier.display.print(humidity); // display the humidity on the screen
  carrier.display.print(" %");
  delay(2500);
  defaultPage();
}

void showPressure(){
  // displaying pressure
  carrier.display.fillScreen(0x0000);
  carrier.display.setCursor(54, 40);
  carrier.display.setTextSize(3);
  carrier.display.print("Pressure");
  carrier.display.drawBitmap(70, 60, pressure_logo, 100, 100, 0xF621); //0xF1C40F); //draw a pressure figure on the MKR IoT carrier's display
  float press = carrier.Pressure.readPressure(); //storing pressure value in a variable
  carrier.display.setCursor(40, 160);
  carrier.display.println(press); // display the pressure on the screen
  carrier.display.setCursor(160, 160);
  carrier.display.print("KPa");
  delay(2500);
  defaultPage();
}

void showVoltage(){
  // displaying pressure
  carrier.display.fillScreen(0x0000);
  carrier.display.setCursor(54, 40);
  carrier.display.setTextSize(3);
  carrier.display.print("AC Mains");
  carrier.display.setCursor(40, 160);
  carrier.display.println(ac_mains_voltage); // display the pressure on the screen
  carrier.display.setCursor(160, 160);
  carrier.display.print("V");
  delay(2500);
  defaultPage();
}


void thermostatControl(){

  if (temperature < 20) {
    carrier.leds.fill(blueColor, 0, 5);
    carrier.leds.show();
 
    if (automatic_mode == true) {
      carrier.Relay1.open();
    }
 
  } else if (temperature >= 20 && temperature <= 33) {
    carrier.leds.fill(greenColor, 0, 5);
    carrier.leds.show();
 
    if (automatic_mode == true) {
      carrier.Relay1.close();
    }
 
  } else if (temperature > 34) {
    carrier.leds.fill(redColor, 0, 5);
    carrier.leds.show();
    if (automatic_mode == true) {
      carrier.Relay2.open();
    }
  }
  delay(1000);
}
 

/*
  Since AutomaticMode is READ_WRITE variable, onAutomaticModeChange() is
  executed every time a new value is received from IoT Cloud.
*/
void onAutomaticModeChange()  {
  // Add your code here to act upon AutomaticMode change
  if (automatic_mode == false) {
    carrier.Relay1.close();
    carrier.Relay2.close();
  }
}
/*
  Since CoolerControl is READ_WRITE variable, onCoolerControlChange() is
  executed every time a new value is received from IoT Cloud.
*/
void onCoolerControlChange()  {
  // Add your code here to act upon CoolerControl change
  if (cooler_control == true) {
    carrier.Relay2.open();
  } else {
    carrier.Relay2.close();
  }
}
/*
  Since HeaterControl is READ_WRITE variable, onHeaterControlChange() is
  executed every time a new value is received from IoT Cloud.
*/
void onHeaterControlChange()  {
  // Add your code here to act upon HeaterControl change
  if (heater_control == true) {
    carrier.Relay1.open();
  } else {
    carrier.Relay1.close();
  }
}