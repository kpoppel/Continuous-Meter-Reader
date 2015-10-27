//-*-c++-*-
/* Meter reader */

#include <SPI.h>
#include "Histogram.h"
#include "Meter.h"
#include "Comparator.h"
#include "MeterReader_pb.h"
#include "OpticalSensor.h"
#include "settings.h"
#include "CommunicateSerial.h"
#include "CommunicateWireless.h"
#include "RadioMessageSender.h"
#include "crc32.h"

Settings<MeterReader_Settings> settings;

#define ADCPIN A7

// Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 9 & 10
RF24 radio(9,10);
RadioMessageSender radioSender(radio);

const char* addresses[] = {"meterS","meterR"};

//uint64_t baseAddress = 286093099LL; 
uint64_t myAddress; 

// analog sensor
Comparator comparator;
Meter meter;
Histogram histogram;

// digital sensors
OpticalSensor digitalSensors[] = {
    OpticalSensor(8),
    OpticalSensor(7), 
    OpticalSensor(6), 
    OpticalSensor(5), 
    OpticalSensor(4), 
    OpticalSensor(3)
};
OpticalSensorCalibrator calibrator(digitalSensors);

const int SAMPLE_FREQUENCY = 200; // Hz
const int CALIBRATION_SECONDS = 5;
const int SAMPLE_TIME = 1000 / SAMPLE_FREQUENCY; // ms
const int SAMPLES_TO_CALIBRATE = CALIBRATION_SECONDS*1000/SAMPLE_TIME;
const int TICKS_BETWEEN_SEND = 200; // 1 second

int calibrationSamples=SAMPLES_TO_CALIBRATE;
bool calibrationMode, calibrationDone;
bool sendValueFlag;
unsigned int ticks; // must be big enough to hold TICKS_BETWEEN_SEND
Receiver<MeterReader_Message, MeterReader_Message_fields> serialinput(Serial);

uint64_t lastSentValue = UINT64_MAX;

void setup() {
  sendValueFlag = false;
  ticks = 0;
  calibrationMode = false;
  calibrationDone = false;

 
  // pick the next series
  settings.load();
  settings.s.seriesId += 1;
  settings.save();

  radio.begin();
  radio.setPALevel(RF24_PA_MAX);

  //myAddress = baseAddress + settings.s.meterId;
  //radio.openWritingPipe(address);
  radio.openWritingPipe((const uint8_t*)addresses[1]);
  radio.openReadingPipe(1,(const uint8_t*)addresses[0]);
  radio.enableDynamicPayloads();
  radio.stopListening();

  radioSender.begin(settings.s.meterId, settings.s.seriesId);
  

  // sanitize the number of digital sensors
  if(settings.s.fallingEdgeAmounts_count > 6) {
    settings.s.risingEdgeAmounts_count = 0;   
  }

  settings.s.fallingEdgeAmounts_count = settings.s.risingEdgeAmounts_count;

  for(int i=0 ; i < settings.s.risingEdgeAmounts_count ; i++) {
    digitalSensors[i].setRisingEdgeAmount(settings.s.risingEdgeAmounts[i]);
    digitalSensors[i].setFallingEdgeAmount(settings.s.fallingEdgeAmounts[i]);
  }

  Serial.begin(57600);

  // the serial port does not work reliably right after initializing 
  // the port, this delay makes sure it's ready before we use it
  delay(500);
  sendLog(Serial, MeterReader_LogMessage_Type_NOTE, "Rebooted");
  sendSettings(Serial, settings.s);
  
  comparator.setThreshold(settings.s.threshold);
  comparator.setHysteresis(settings.s.hysteresis);
  comparator.addUnitIncrementListener(&meter);  

 // initialize timer1 
  noInterrupts();           // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  OCR1A = 16000000/64/SAMPLE_FREQUENCY;  // compare match register 16MHz/64/200Hz
  TCCR1B |= (1 << WGM12);   // CTC mode (Clear timer on compare match)
  TCCR1B |= (1 << CS11) | (1 << CS10);  // 64 prescaler 
  TIMSK1 |= (1 << OCIE1A);  // enable timer compare interrupt
  interrupts();             // enable all interrupts
}

ISR(TIMER1_COMPA_vect)          // timer compare interrupt service routine
{

  int sensorValue = analogRead(ADCPIN);

  if(calibrationMode) {
    if(settings.s.samplingMode == MeterReader_Settings_SamplingMode_ANALOG) {
      if(calibrationSamples > 0) {
        histogram.sample(sensorValue);
        calibrationSamples--;
      } else if(calibrationSamples == 0) {
        int low = histogram.getPercentile(25);
        int high = histogram.getPercentile(75);
        int hysteresis = (high-low)/2;
        int threshold=(high+low)/2;    
        comparator.setHysteresis(hysteresis);
        comparator.setThreshold(threshold);    
        calibrationMode = false;
        calibrationDone = true;
      }
    } else {
      if(calibrator.tick()) {
        calibrationMode = false;
        calibrationDone = true;
      }
    }
  } else {
    if(settings.s.samplingMode == MeterReader_Settings_SamplingMode_ANALOG) {
      comparator.sample(sensorValue);  
    } else {
      int change = 0;
      for(int sensor = 0; sensor < settings.s.risingEdgeAmounts_count ; sensor++) {
        change += digitalSensors[sensor].getAmount();
      }
      meter.add(change);
    }

    ticks++;
    if(ticks > TICKS_BETWEEN_SEND) {
      sendValueFlag = true;
      ticks = 0;
    }
  }



}

void loop() {
    if(sendValueFlag) {
      uint64_t currentValue;
      // protected access to variables shared with ISR BEGIN
      noInterrupts();
      sendValueFlag = 0;
      currentValue = meter.getCurrentValue();
      interrupts();
      // protected access to variables shared with ISR END

      if(true || currentValue != lastSentValue) {
        //if(settings.s.communicationChannel == MeterReader_Settings_CommunicationChannel_WIRELESS) {
        //unsigned long time = micros();                             // Take the time, and send it.  This will block until complete
        //if (!radio.write( &time, sizeof(unsigned long) )){
        //  Serial.println(F("failed"));
        //}
        sendCounterUpdateByRadio(radioSender, settings.s.meterId, settings.s.seriesId, currentValue);
        //} else {
        //sendCounterUpdate(Serial, settings.s.meterId, settings.s.seriesId, currentValue);
        //}
        lastSentValue = currentValue;
      }

      if(calibrationDone) {
        calibrationDone = false;
        noInterrupts();
        if(settings.s.samplingMode == MeterReader_Settings_SamplingMode_ANALOG) {
          settings.s.threshold = comparator.getThreshold();
          settings.s.hysteresis = comparator.getHysteresis();
        } else {
          for(int i=0 ; i < settings.s.fallingEdgeAmounts_count; i++) {
            settings.s.fallingEdgeAmounts[i] = digitalSensors[i].getFallingEdgeAmount();
            settings.s.risingEdgeAmounts[i] = digitalSensors[i].getRisingEdgeAmount();
          }
        }
        settings.save();
        interrupts();
        sendSettings(Serial, settings.s);
      }
    }

    if(serialinput.process()) {
      switch(serialinput.message.which_message) {
        case MeterReader_Message_calibrate_tag:
          noInterrupts();
          calibrationMode = true;
          if(settings.s.samplingMode == MeterReader_Settings_SamplingMode_ANALOG) {
            calibrationSamples=SAMPLES_TO_CALIBRATE;
            histogram.clear();
          } else {
            calibrator.start(settings.s.fallingEdgeAmounts_count, 1000, 1);
          }
          interrupts();
          break;
        case MeterReader_Message_settings_tag:
          noInterrupts();
          settings.s = serialinput.message.message.settings;
          interrupts();
          settings.save();
          sendSettings(Serial, settings.s);
          break;
        default:
          // ignore message
          break;
      }
    }
    
}

