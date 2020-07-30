/*
Copyright 2016 Attila Szarvas

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/


#include <EEPROM.h>

const int kFan0PwmPin = 3;
const int kFan1PwmPin = 9;
const int kFan2PwmPin = 10;
const int kFan3PwmPin = 11;

const int kFan0RpmPin = 4;
const int kFan1RpmPin = 6;
const int kFan2RpmPin = 7;
const int kFan3RpmPin = 8;

void setup() {
  Serial.begin(19200);

  pinMode(kFan0RpmPin, INPUT_PULLUP); 
  pinMode(kFan1RpmPin, INPUT_PULLUP);
  pinMode(kFan2RpmPin, INPUT_PULLUP); 
  pinMode(kFan3RpmPin, INPUT_PULLUP);
}

void emptySerialBuffer() {
  while (Serial.available() > 0) {
    Serial.read();
  }
}

// Returns 1 if command is intercepted
//         0 otherwise
int ReadSerial(int* buffer, const unsigned int length) {
  while (Serial.available() > 0) {
    if (Serial.read() == 'c') {
      Serial.setTimeout(100);
      int i;
      for (i = 0; i < length; ++i) {
        buffer[i] = Serial.parseInt();
      }

      // We want to respond quickly to duty cycle commands
      // So if there is a long queue of such commands, we 
      // discard the old ones
      emptySerialBuffer();
      return 1;
    }
  }
  
  return 0;
}

// This affects the minimum measurable RPM
// Non-rotating fans will cause twice this much delay per reading
const unsigned long kPwmReadTimeout = 300000;

unsigned long ReadRpm(const int pin) {
  digitalWrite(pin, HIGH);
  delay(5);
  
  // Intel compliant PWM fans generate 2 pulses per cycle
  const unsigned long period_width = 60000000/2;
  unsigned long cycle_width = pulseIn(pin, LOW, kPwmReadTimeout) + pulseIn(pin, HIGH, kPwmReadTimeout);
  
  if (cycle_width == 0) {
    return 0;
  }
  
  return period_width / cycle_width;
}

int dutycycle_command_age = 0;
int fan_0_dutycycle;
int fan_1_dutycycle;
int fan_2_dutycycle;
int fan_3_dutycycle;

void loop() {
  dutycycle_command_age += 1;
  
  const int kCommandLength = 5;
  int command[kCommandLength];
  
  const int kCommandSetDutycycle = 1;
  const int kCommandSetDefaultDutycycle = 2;
  
  unsigned int rpm0, rpm1, rpm2, rpm3;

  rpm0 = ReadRpm(kFan0RpmPin);
  rpm1 = ReadRpm(kFan1RpmPin);
  rpm2 = ReadRpm(kFan2RpmPin);
  rpm3 = ReadRpm(kFan3RpmPin);
      
  Serial.print(rpm0);
  Serial.print(" ");
  Serial.print(rpm1);
  Serial.print(" ");
  Serial.print(rpm2);
  Serial.print(" ");
  Serial.print(rpm3);
  Serial.print(" ");
  Serial.print(fan_0_dutycycle);
  Serial.print(" ");
  Serial.print(fan_1_dutycycle);
  Serial.print(" ");
  Serial.print(fan_2_dutycycle);
  Serial.print(" ");
  Serial.println(fan_3_dutycycle);

  if (ReadSerial(command, kCommandLength)) {
    switch (command[0]) {
      case kCommandSetDutycycle:
        dutycycle_command_age = 0;
        fan_0_dutycycle = command[1];
        fan_1_dutycycle = command[2];
        fan_2_dutycycle = command[3];
        fan_3_dutycycle = command[4];
        break;
        
      case kCommandSetDefaultDutycycle:
        EEPROM.write(0, command[1]);
        EEPROM.write(1, command[2]);
        EEPROM.write(2, command[3]);
        EEPROM.write(3, command[4]);
        break;
    }
  }
  
  if (dutycycle_command_age > 10) {
    fan_0_dutycycle = EEPROM.read(0);
    fan_1_dutycycle = EEPROM.read(1);
    fan_2_dutycycle = EEPROM.read(2);
    fan_3_dutycycle = EEPROM.read(3);
  }
  
  analogWrite(kFan0PwmPin, fan_0_dutycycle);
  analogWrite(kFan1PwmPin, fan_1_dutycycle);
  analogWrite(kFan2PwmPin, fan_2_dutycycle);
  analogWrite(kFan3PwmPin, fan_3_dutycycle);

  delay(200);
}
