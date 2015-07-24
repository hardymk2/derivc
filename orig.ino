
//=============================OVERVIEW============================
/*
Below is a code for a thermostat for use in incubators in the developing world. 
It is intended as an automated, self-regulating system and should require little user system interaction. 
It consists mainly of a temperature sensor along with a heat source and cooling fan to allow for bang-bang 
heating control for the environment. Additional features are an alarm system (with buzzer and LEDS), 
LED display (for easy temperature read-out), and data logging features (exporting the Serial data to a .csv 
file for easy plotting and analysis in excel). The user is able to control the desired temperature of the 
incubator, the critical threshold temperature considered too hot for the enclosure, and the alarm frequency 
using sliders controls. Built-in error cases include notifying the user of incorrect threshold settings, 
disconnected thermistor or wiring failures, and automatic timed heating shut-off to prevent dangerous overheating.
*/

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//These pin definitions are for v1.7 of the board
#define SLIDER1  A0 //Sets ambient temperature value for the incubator

#define HEAT1    7 //Bottom pad
//#define HEAT2    4 //Back wall pad  //unused
#define LED1     5 //alarm LEDs
#define BUZZER   6 //buzzer
#define BIT1     13
#define BIT2     12
#define BIT3     11


#include <Wire.h> 
#include "Adafruit_LEDBackpack.h" //library for LED display
#include "Adafruit_GFX.h" //library for LED display
//#include <PID_v1.h> //PID library
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

unsigned long timerled;  //controls led delay times
unsigned long start_time; 
unsigned long elapsed_time; //creates time stamp for data recording
int initcrit; //set initial condition for critical temperature
boolean led_run1=false; //controls for warning leds
boolean led_run2=true;
boolean heat_flag=false; //flag if the heater is on
int comment=0;  //flags for possible comments/warnings
int comment2=0;
int count=0; //counter for heating safety mechanism
float temperature[8] = {0,0,0,0,0,0,0,0};
float t[8] = {0,0,0,0,0,0,0,0};
float total=0;
float sample_time;
float old_temp;
float new_temp;
float baby_temp;
int i=-1;
int second_flag = 0;
int firsthump = 1; // this establishes the initial heating which is more likely to have a larger overshoot than future oscillations

const int num_dydx = 60;            //input parameter determining how the length of time over which to calculate the derivative. Input in seconds
const float b = -0.5;            //parameters for adjusting the set_temp
const float c = -0.1;
int r = 0;

float dydx = 0;
float dydx_array[num_dydx];
int d=0;
float set_temp;
float heatingpad_temp = 35;
float boxair_temp = 35;


Adafruit_7segment matrix = Adafruit_7segment(); //set up for LED display


void setup()
{ 
  //Initialize inputs and outputs
  pinMode(SLIDER1, INPUT);
  pinMode(A3, INPUT); //thermistor 

  pinMode(2, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(HEAT1, OUTPUT);
  pinMode(BIT1, OUTPUT);
  pinMode(BIT2, OUTPUT);
  pinMode(BIT3, OUTPUT);
    
  //Set up for LED display
  #ifndef __AVR_ATtiny85__ 
  Serial.begin(9600);//can adjust temp display time by adjusting serial.begin
  #endif
  matrix.begin(0x70);

  initcrit = 0; //The critical condition is not initialized
  old_temp=22;  //arbitrary non-zero starting temp
  sample_time=millis();
  start_time = millis(); //initializes the timer
  
  digitalWrite(2, HIGH);
  digitalWrite(BUZZER, LOW);

  for (int j = 0; j < num_dydx; j++)
    dydx_array[j] = 0;
    
 
}

void loop()
{
  delay(100);
  //=============================READ INPUTS============================
  int sensorValue = analogRead(A3); //thermistor
  int val1 = analogRead(SLIDER1);
  long slider = map(val1, 0, 1020, 38.49, 26.5);     //mapping lower temperature threshold to slider 1 location
  long hightemp = slider+2;  //absolute hottest temperature, alarm sounds
    
  
    for (int r=4; r<=7; r++){
      digitalWrite(BIT1, bitRead(r, 0));
      digitalWrite(BIT2, bitRead(r, 1));   
      digitalWrite(BIT3, bitRead(r, 2));
     
      int sensorValue = analogRead(A3); //thermistor
      float tempvolt = sensorValue*(5.0/1023.0);  //convert analog reading to voltage...and then to temperature
      t[r] = 100*(3.044625*pow(10,-5)*pow(tempvolt,6)-(.005209376*pow(tempvolt,5))+0.065699269*pow(tempvolt,4)-0.340695972*pow(tempvolt,3)+0.897136183*pow(tempvolt,2)-1.419855102*tempvolt+1.451672296); //voltage to temp conversion eqn
      temperature[r] += t[r]; 
    }
  i++;
  
  if (i == 10){
    for(int k=4; k<=7; k++){
      temperature[k] /= 10;
      }
      baby_temp = temperature[4];
 
     new_temp = baby_temp;     //globally used variable for temperature

    dydx -= dydx_array[d]; 
    dydx_array[d] = new_temp - old_temp;  //dx = 1
    dydx += dydx_array[d];
     
    old_temp = new_temp;        //making the current values "old" values
    total = 0;
    i = 0;
    second_flag = 1;
    d++;
    if (d >= num_dydx)            //num_dydx (set above) sets the time length over which the derivative is taken. num_dydx = 5 (sec)
      d = 0;
  }  

  
  
  //======================ADDRESSING USER ERROR CASES=====================
  if (baby_temp < 10 || baby_temp > 45){ //addresses temperature probe being out of range. Most likely the sensor is unpluged
    comment = 2;
    digitalWrite(BUZZER, HIGH);   //heater turns on

      }

   //=============================HEATER CONTROL=============================
  else{
    digitalWrite(BUZZER, LOW);
 
    set_temp = slider+ b*dydx;             //the last term being a function of how long the heating pad can release heat after it is shut off
                                                                                      // b,c <0 and small
    
    //this is used to mitigate the initial overshoot of the first hump.  The initial set is lowered so that the heating pads cool some and dont push waay above the slider temp.
    if (firsthump==1){
      set_temp -= .6;
      if (dydx<0 && baby_temp > set_temp )
        firsthump = 0;}
    
    
    
   
        
    comment = 0; //no comment  
    if (baby_temp < set_temp){    //Baby's temperature is too low
      digitalWrite(HEAT1, HIGH);   //heater turns on
      heat_flag = true;
    }
    else{                  //Baby's temperature is just right
      digitalWrite(HEAT1, LOW);
      heat_flag = false;
      count = 0;
    }
      
  }


   //======================CRITICAL TEMPERATURE REACHED======================
    if(baby_temp > hightemp){ //if reaches too hot threshold temp
      comment2 = 1;
      if (initcrit==0){    //initial case to set up alternating led and timer (timerled)
        timerled=millis(); //built-in overall timer for led delays
        initcrit = 1;      //escapes the initial case from now on
        led_run1 = false;
        led_run2 = true;}  //changes led states for alternating lights
      if (millis()-timerled >= 75UL){  //changes state of leds
        led_run1 = !led_run1;
        digitalWrite(LED1, led_run1);
        led_run2 = !led_run2;
        digitalWrite(BUZZER, led_run2);
        timerled=millis();}  //resets the timer
    }
    else{
      digitalWrite(LED1, LOW); //turns off the leds
      digitalWrite(BUZZER, LOW);
      initcrit = 0;     //resets the initial critical condition
      comment2 = 0;} 


  elapsed_time = (millis() - start_time)/1000;

   //=============================PRINT OUTS============================
  //Print to 7-segment Display
  if (second_flag == 1) {

    int threedig = round(baby_temp*10);
    matrix.writeDigitNum(4, threedig % 10);  //Tenths digit (rounded)
    int twodig = floor(threedig / 10);
    matrix.writeDigitNum(3, twodig % 10,true); //Ones digit, decimal is true
    int wondig = floor(twodig / 10);
    matrix.writeDigitNum(1,wondig); //Tens digit

    matrix.writeDisplay();

    r++;
    //if (r == 60) {  //use to control how often data is printed to the serial monitor
    Serial.print(elapsed_time); Serial.print(", ");  
    for(int k=4; k<=7; k++){
      Serial.print(temperature[k]); Serial.print(", ");}
    Serial.print(slider); Serial.print(", ");
    Serial.print(set_temp); Serial.print(", ");
    Serial.print(dydx); Serial.print(", ");
    Serial.print(heat_flag); Serial.print(", ");

    if(comment == 1)
      Serial.println("WARNING: Incorrect temperature settings! Check temperature threshold values!");
    else if(comment == 2)
      Serial.println("WARNING: Sensor out of range - check wiring!");
    else if(comment2 == 1)
      Serial.println("It's too hot!");
    else
      Serial.println();
      
      r = 0;
    //}
    
    second_flag = 0;
    memset(temperature, 0, sizeof(temperature));  //clears the temperature array

  }
}

   //==========================ALARM FUNCTION=========================

void alarm() {                //LEDs flicker off and on
  digitalWrite(LED1, HIGH);
  digitalWrite(BUZZER, LOW);   
  delay(120);               // wait for 0.12 seconds
  digitalWrite(LED1, LOW);  
  digitalWrite(BUZZER, HIGH);   
  delay(120);               // wait for 0.12 seconds
}
