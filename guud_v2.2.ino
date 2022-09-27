/*
  V2.2 LOG:
  
  Started on: Sep 17th, 2022, Juan Cereigido

  Notes: 2.1 works fine, but it doens't save the latest brightness value, 2.2 is intended to save those values
 
*/

// ---------------------- 8X8 MATRIX SETUP ----------------------

#include <FastLED.h>

#define NUM_LEDS 64
#define DATA_PIN 2 // D4 in ESP
#define SPEEDY 36 // Transition speed
#define MAX_BRIGHTNESS 200 // Maximum brightness. MAX_BRIGHTNESS is too much and warms too much the leds color.

// Define the array of leds
CRGB leds[NUM_LEDS];

uint8_t bright = 0;
uint8_t firstStart = 0;
int savedBrightness = MAX_BRIGHTNESS;

// ---------------------- 8X8 MATRIX SETUP ----------------------

// ---------------------- START VIBRATOR SETUP ----------------------

/**** General settings vars ***/
const int vibPin =  14;               // Number of the vibrator pin (14 == D5 in ESP)
int vibState = LOW;                   // Used to set the vibrator state (HIGH or LOW)
unsigned long currentMillis;          // Stores current time

/**** vibOnce effect vars ***/
int vibOnceStarted = false;           // Stores if vibrator has been already activated before
unsigned long prevMilVibOnce = 0;     // Stores last time vibrator has been updated

/**** vibError effect vars ***/
bool vibErrorStarted = false;
bool vibErrorFinished = false;
unsigned long prevMillisVibError;

bool switchOnCountStarted = false;
unsigned long switchOnCounter;
int switchSpeed = 5;

// ---------------------- END VIBRATOR SETUP ----------------------

// ---------------------- 6050 SETUP ----------------------

#include <Wire.h>
 
//Direccion I2C de la IMU
#define MPU 0x68
 
//Ratios de conversion
#define A_R 16384.0 // 32768/2
#define G_R 131.0 // 32768/250
 
//Conversion de radianes a grados 180/PI
#define RAD_A_DEG = 57.295779
 
//MPU-6050 da los valores en enteros de 16 bits
//Valores RAW
int16_t AcX, AcY, AcZ, GyX, GyY, GyZ;
 
//Angulos
float Acc[2];
float Gy[3];
float Angle[3];

String valores;
String brillo;

long tiempo_prev;
float dt;

// ---------------------- 6050 SETUP ----------------------

void setup() { 
 
// ---------------------- 8X8 MATRIX SETUP ----------------------

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);  // GRB ordering is assumed
  FastLED.setBrightness(MAX_BRIGHTNESS);
  //FastLED.setCorrection(TypicalPixelString);
  //FastLED.setTemperature(Tungsten100W);
  FastLED.show(); // Initialize all pixels to 'off'
    
// ---------------------- 8X8 MATRIX SETUP ----------------------

// ---------------------- START VIBRATOR SETUP ----------------------
  pinMode(vibPin, OUTPUT);
// ---------------------- END VIBRATOR SETUP ----------------------

// ---------------------- 6050 SETUP ----------------------

  Wire.begin(0,4); // D3=SDA / D2=SCL
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
  Serial.begin(115200);

// ---------------------- 6050 SETUP ----------------------

}

void loop() {

  if (firstStart == 0) {
    startAnimation();
    firstStart = 1;
  }

// ---------------------- START 6050 READ ----------------------

  read6050 ();
  printValues ();

// ---------------------- END 6050 READ ----------------------

// ---------------------- START POSITION READ --------------

  currentMillis = millis();                   // Updates the currentMillis variable
  
  if(Angle[0] < -6) {
    
    if (bright == savedBrightness && switchOnCountStarted == false) {
      switchOnCountStarted = true;  
      switchOnCounter = currentMillis;
    }
    
    if (switchOnCountStarted == true) {
      if (currentMillis - switchOnCounter >= 3000) {
        savedBrightness = MAX_BRIGHTNESS;  
      }
    }
    switchOn();
  }
  
  if(Angle[0] > 4) {
    if (bright == 0) {
      vibError();
    } else {
      switchOff(); 
    }
  }
    
  if(Angle[1] > 3) {
    fadeIn(15);
  }
  
  if(Angle[1] < -8) {
    fadeOut(15);
  }
  
  if(Angle[1] > -8 && Angle[1] < 3 && Angle[0] > -6 && Angle[0] < 6) {        // If lamp is at position zero
    vibOnceStarted = false;                                                   // Resets vibOnce var so it can vibrate again next time
    vibErrorStarted = false;                                                  // Resets vibError var so it can vibrate again next time
    switchOnCountStarted = false;
    digitalWrite(vibPin, LOW);                                                // Turns off vibrator to avoid bugs
  }

  if (savedBrightness < 30) {
    switchSpeed = 50;
  } else {
    switchSpeed = 5;
  }

// ---------------------- END POSITION READ --------------

}


// ---------------------- START OF STARTING ANIMATION ----------------------

void startAnimation(){
  ledsOff(0,63);
}

// ---------------------- END OF STARTING ANIMATION ----------------------

void ledsOn (uint8_t starting, uint8_t ending){
  for(int i=starting; i<=ending; i++) {
    leds[i] = CRGB(MAX_BRIGHTNESS,MAX_BRIGHTNESS,MAX_BRIGHTNESS);
  }

  FastLED.show();
}

void ledsOff (uint8_t starting, uint8_t ending){
  for(int i=starting; i<=ending; i++) {
    leds[i] = CRGB(0,0,0);
  }

  FastLED.show();
}

// ---------------------- START OF FADE EFFECTS ----------------------

void fadeIn (uint8_t fadeSpeed) {
  
  if (bright < MAX_BRIGHTNESS) {
    vibOnce ();
    EVERY_N_MILLISECONDS(fadeSpeed) {
      bright++;
      for(int i=0; i<=63; i++) {            // Turns ALL leds on, adding one step of brightness every N millisecons
        leds[i] = CRGB(bright,bright,bright);
      }
      FastLED.show();
      if (bright < 15) {
        savedBrightness = 15;                  // Saves the latest brightness value as 5 to avoid setting saved value to 0
      } else {
        savedBrightness = bright;             // Saves the latest brightness value so next time it turns on, it goes to the last value and not 100%
      }
    }
  } else {
    vibError();
  }
 
}

void fadeOut (uint8_t fadeSpeed) {
  
  if (bright > 0) {
    vibOnce ();
    
    if (bright <= MAX_BRIGHTNESS && bright > 80) { // If the brightness is between MAX_BRIGHTNESS and 80, the brightness will decrease twice as fast (the LEDs doenst visually change too much between these values)
      EVERY_N_MILLISECONDS(fadeSpeed) {
        bright = bright - 2;
        for(int i=0; i<=63; i++) {            // Turns ALL leds on, adding one step of brightness every N millisecons
          leds[i] = CRGB(bright,bright,bright);
        }
        FastLED.show();
        savedBrightness = bright;             // Saves the latest brightness value so next time it turns on, it goes to the last value and not 100%
      }
    }
        
    if (bright <= 80 && bright > 0) {
      EVERY_N_MILLISECONDS(fadeSpeed) {
        bright--;
        for(int i=0; i<=63; i++) {            // Turns ALL leds on, adding one step of brightness every N millisecons
          leds[i] = CRGB(bright,bright,bright);
        }
        FastLED.show();
        if (bright < 10) {
          savedBrightness = 10;                  // Saves the latest brightness value as 5 to avoid setting saved value to 0
        } else {
          savedBrightness = bright;             // Saves the latest brightness value so next time it turns on, it goes to the last value and not 100%
        }
      }
    }
    } else {
        vibError ();
    }
}

// ---------------------- END OF FADE EFFECTS ----------------------

// ---------------------- START OF SWITCH EFFECTS ----------------------

void switchOn () {
  
  if (bright == MAX_BRIGHTNESS) {
    vibError();
  }

  if (bright < MAX_BRIGHTNESS) {
    while(bright < savedBrightness) {
      vibOnce ();
      EVERY_N_MILLISECONDS(switchSpeed) {
        read6050();
        printValues();
        bright++;
        for(int i=0; i<=63; i++) {            // Turns ALL leds on, adding one step of brightness every N millisecons
          leds[i] = CRGB(bright,bright,bright);
        }
        FastLED.show();
      }
    }
  } 
}

void switchOff () {
  while(bright > 0) {
    vibOnce ();
    EVERY_N_MILLISECONDS(switchSpeed) {
      read6050();
      printValues();
      bright--;
      for(int i=0; i<=63; i++) {            // Turns ALL leds on, adding one step of brightness every N millisecons
        leds[i] = CRGB(bright,bright,bright);
      }
      FastLED.show();
    }
  }
}

/*void switchOff () {

  if (bright == 0) {
    vibError();
  }
  
  if (bright > 0) {
    while(bright > 0) {
      vibOnce ();
      EVERY_N_MILLISECONDS(switchSpeed) {
        read6050();
        printValues();
        bright--;
        for(int i=0; i<=63; i++) {            // Turns ALL leds on, adding one step of brightness every N millisecons
          leds[i] = CRGB(bright,bright,bright);
        }
        FastLED.show();
      }
    }
  }
}*/

// ---------------------- END OF SWITCH EFFECTS ----------------------

// ---------------------- START OF VIBRATOR EFFECTS ----------------------

void vibOnce () {
  currentMillis = millis();                   // Updates the currentMillis variable

  if (vibOnceStarted == false) {              // Has the vibrator already been activated?
      prevMilVibOnce = currentMillis;              // Saves the time when the vibrator turned on
      digitalWrite(vibPin, HIGH);             // Turns on the vibrator
      vibOnceStarted = true;
  } else {
    if (currentMillis - prevMilVibOnce >= 50) {    // Check if it has been on for more than 50 milliseconds
      digitalWrite(vibPin, LOW); 
    }
  }
}

void vibError () {

  currentMillis = millis();

  if (vibErrorStarted == false) {
    digitalWrite(vibPin, HIGH);
    prevMillisVibError = currentMillis;
    vibErrorStarted = !vibErrorStarted;
  } else {
    if (currentMillis - prevMillisVibError >= 40) {
      digitalWrite(vibPin, LOW);
    }
  
    if (currentMillis - prevMillisVibError >= 100) {
      digitalWrite(vibPin, HIGH);
    }
  
    if (currentMillis - prevMillisVibError >= 140) {
      digitalWrite(vibPin, LOW);
      return;
    }
  }
  
}

// ---------------------- END OF VIBRATOR EFFECTS ----------------------


void read6050 () {  
  //Leer los valores del Acelerometro de la IMU
  Wire.beginTransmission(MPU);
  Wire.write(0x3B); //Pedir el registro 0x3B - corresponde al AcX
  Wire.endTransmission(false);
  Wire.requestFrom(MPU,6,true);   //A partir del 0x3B, se piden 6 registros
  AcX=Wire.read()<<8|Wire.read(); //Cada valor ocupa 2 registros
  AcY=Wire.read()<<8|Wire.read();
  AcZ=Wire.read()<<8|Wire.read();
 
  //A partir de los valores del acelerometro, se calculan los angulos Y, X respectivamente, con la formula de la tangente.
  Acc[1] = atan(-1*(AcX/A_R)/sqrt(pow((AcY/A_R),2) + pow((AcZ/A_R),2)))*RAD_TO_DEG;
  Acc[0] = atan((AcY/A_R)/sqrt(pow((AcX/A_R),2) + pow((AcZ/A_R),2)))*RAD_TO_DEG;
 
  //Leer los valores del Giroscopio
  Wire.beginTransmission(MPU);
  Wire.write(0x43);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU,6,true);   //A partir del 0x43, se piden 6 registros
  GyX=Wire.read()<<8|Wire.read(); //Cada valor ocupa 2 registros
  GyY=Wire.read()<<8|Wire.read();
  GyZ=Wire.read()<<8|Wire.read();
 
  //Calculo del angulo del Giroscopio
  Gy[0] = GyX/G_R;
  Gy[1] = GyY/G_R;
  Gy[2] = GyZ/G_R;

  dt = (millis() - tiempo_prev) / 1000.0;
  tiempo_prev = millis();
 
  //Aplicar el Filtro Complementario
  Angle[0] = 0.98 *(Angle[0]+Gy[0]*dt) + 0.02*Acc[0];
  Angle[1] = 0.98 *(Angle[1]+Gy[1]*dt) + 0.02*Acc[1];

  //Integración respecto del tiempo paras calcular el YAW
  Angle[2] = Angle[2]+Gy[2]*dt;
}

void printValues () {
  //valores = "Y: " +String(Angle[0]) + " | X: " + String(Angle[1]) + "," + String(Angle[2]);
  valores = " Switch axis: " +String(Angle[0]) + " | Fade axis: " + String(Angle[1]);
  brillo = "Brightness: " + String(bright) + " Saved Brightness: " + String(savedBrightness) + " Switch speed:" + String(switchSpeed);
  Serial.println(brillo + valores);
}
