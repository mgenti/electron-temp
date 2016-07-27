#include "application.h"
#include <math.h>
#include <string>
//#include "ThingSpeak.h"


// which analog pin to connect
#define THERMISTOR1_PIN A0
#define THERMISTOR2_PIN A1
// resistance at 25 degrees C
#define THERMISTORNOMINAL 10000
// temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25
// how many samples to take and average, more takes longer
// but is more 'smooth'
#define NUMSAMPLES 5
// The beta coefficient of the thermistor (usually 3000-4000)
#define BCOEFFICIENT 3988
// the value of the 'other' resistor
#define SERIESRESISTOR 9800

int blueLed = D7;
int t1Samples[NUMSAMPLES];
int t2Samples[NUMSAMPLES];
unsigned long myChannelNumber = 134641;
const char * myWriteAPIKey = "AET6K22V4K68D4TU";
//TCPClient client;
bool checkTemp = false;

float steinhart(float resistance)
{
  float steinhart;
  steinhart = resistance / THERMISTORNOMINAL;     // (R/Ro)
  steinhart = log(steinhart);                  // ln(R/Ro)
  steinhart /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart;                 // Invert
  steinhart -= 273.15;                         // convert to C
  return steinhart;
}

class LEDTimerCallback
{
  uint8_t blueLedState = LOW;
public:
     void onTimeout();
} LEDCallback;

void LEDTimerCallback::onTimeout()
{
  if (blueLedState == LOW)
  {
    blueLedState = HIGH;
  }
  else
  {
    blueLedState = LOW;
  }
  digitalWrite(blueLed, blueLedState);
}

class TempTimerCallback
{
public:
     void onTimeout();
} callback;

void TempTimerCallback::onTimeout()
{
  checkTemp = true;
  Serial.println("checkTemp = true");
}

Timer t(600000, &TempTimerCallback::onTimeout, callback);
Timer tLed(1000, &LEDTimerCallback::onTimeout, LEDCallback);

void updateTemp()
{
  // take N samples in a row, with a slight delay
  for (uint8_t i=0; i < NUMSAMPLES; i++) {
   t1Samples[i] = analogRead(THERMISTOR1_PIN);
   t2Samples[i] = analogRead(THERMISTOR2_PIN);
   delay(10);
  }

  // average all the samples out
  float t1Average = 0;
  float t2Average = 0;
  for (uint8_t i=0; i < NUMSAMPLES; i++) {
     t1Average += t1Samples[i];
     t2Average += t2Samples[i];
  }
  t1Average /= NUMSAMPLES;
  t2Average /= NUMSAMPLES;


  Serial.print("Average A0 analog reading ");
  Serial.print(t1Average);

  // convert the value to resistance
  t1Average = 4095 / t1Average - 1;
  t1Average = SERIESRESISTOR / t1Average;
  Serial.print(" Thermistor resistance ");
  Serial.println(t1Average);

  Serial.print("Temperature ");
  char faren[5];
  sprintf(faren, "%2.1f", 9.0f*steinhart(t1Average)/5.0f+32.0f);
  Serial.print(faren);
  Serial.println(" *F");

  bool publishResult = Particle.publish("A0", faren, PRIVATE, NO_ACK);
  Serial.print("publishResult=");
  Serial.println(publishResult);
  //ThingSpeak.setField(1, faren);


  Serial.print("Average A1 analog reading ");
  Serial.print(t2Average);

  // convert the value to resistance
  t2Average = 4095 / t2Average - 1;
  t2Average = SERIESRESISTOR / t2Average;
  Serial.print(" Thermistor resistance ");
  Serial.println(t2Average);

  Serial.print("Temperature ");
  sprintf(faren, "%2.1f", 9.0f*steinhart(t2Average)/5.0f+32.0f);
  Serial.print(faren);
  Serial.println(" *F");

  publishResult = Particle.publish("A1", faren, PRIVATE, NO_ACK);
  Serial.print("publishResult=");
  Serial.println(publishResult);

  //ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
}

//SYSTEM_MODE(SEMI_AUTOMATIC);
void setup() {
  pinMode(blueLed, OUTPUT);
  pinMode(THERMISTOR1_PIN, INPUT);
  pinMode(THERMISTOR2_PIN, INPUT);

  Serial.begin(9600);
  Serial.println("Hello World!");

  t.start();
  tLed.start();

  //Cellular.off();
  //Cellular.on();
  //Cellular.connect();
  //Particle.connect();
  //ThingSpeak.begin(client);
}

void loop() {
  if (checkTemp)
  {
    checkTemp = false;
    updateTemp();
  }
}
