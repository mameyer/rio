#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include "BluetoothSerial.h"

MAX30105 particleSensor;
BluetoothSerial ESP_BT;

int max30102_status;
int incoming;

const byte RATE_SIZE = 8;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;

long irValue;
float beatsPerMinute;
int beatAvg;

void max30102_initialize()
{
  
  max30102_status = particleSensor.begin(Wire, I2C_SPEED_FAST); // use default I2C port, 400kHz speed
  
  if (!max30102_status)
  {
    Serial.println("MAX30102 was not found. Please check wiring/power. ");
    return;
  }
  
  Serial.println("place your index finger on the sensor with steady pressure.");
  delay(3);

  particleSensor.setup(); // configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); // turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0); // turn off green LED
}

void setup()
{
  Serial.begin(115200);

  ESP_BT.begin("rip-checker"); //Name of your Bluetooth Signal
  pinMode (LED_BUILTIN, OUTPUT); //Specify that LED pin is output

  max30102_initialize();
}

void print_max30102_serial()
{
  if (irValue < 50000) {
    Serial.print("IR=");
    Serial.print(irValue);
    Serial.print("RIP in peaces");
    Serial.println();
    return;
  }
  
  Serial.print(", BPM=");
  Serial.print(beatsPerMinute);
  Serial.print(", Avg BPM=");
  Serial.print(beatAvg);
  Serial.println();
}

void print_max30102_blt()
{
  if (irValue < 50000) {
    ESP_BT.print("IR=");
    ESP_BT.print(irValue);
    ESP_BT.print(" => no finger?");
    ESP_BT.println();
    return;
  }
  
  ESP_BT.print(", BPM=");
  ESP_BT.print(beatsPerMinute);
  ESP_BT.print(", Avg BPM=");
  ESP_BT.print(beatAvg);
  ESP_BT.println();
}

void loop()
{
  irValue = particleSensor.getIR();
 
  if (checkForBeat(irValue) == true)
  {
    // we sensed a beat!
    long delta = millis() - lastBeat;
    lastBeat = millis();
     
    beatsPerMinute = 60 / (delta / 1000.0);
     
    if (beatsPerMinute < 255 && beatsPerMinute > 20)
    {
      rates[rateSpot++] = (byte)beatsPerMinute; // store this reading in the array
      rateSpot %= RATE_SIZE; // wrap variable
       
      // take average of readings
      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++)
      beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
  }

  print_max30102_serial();
  
  if (ESP_BT.available()) // check if we receive anything from Bluetooth
  {
    incoming = ESP_BT.read(); // read what we recevive
    Serial.print("BT received:");
    Serial.println(incoming);

    if (incoming == 49)
    {
      digitalWrite(LED_BUILTIN, HIGH);

      if (!max30102_status)
      {
        ESP_BT.println("max30102 not connected. initialize..");
        max30102_initialize();
      }
      else
      {
        print_max30102_blt();
      }
    }
       
    if (incoming == 48)
    {
      digitalWrite(LED_BUILTIN, LOW);

      ESP_BT.println("max30102 initialize and calibrate..");
      max30102_initialize();
    }
  }
}
