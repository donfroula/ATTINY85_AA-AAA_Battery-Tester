/*  BatteryTesterAttiny.ino  -
  2015 Copyright (c) Andreas Spiess  All right reserved.

  Author: Andreas Spiess

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

*/

#include <TinyOzOLED.h>
#include <TinyWireM.h>

// ATtiny Pin 5 = SDA                   ATtiny Pin 6 = (D1) to LED2
// ATtiny Pin 7 = SCK                    ATtiny Pin 8 = VCC (2.7-5.5V)

#define FET 1          // ATtiny Pin 6
#define ADCVOLT A2     // ATtiny Pin 3
#define ADCCURRENT A3  // ATtiny Pin 2

const float FACTOR = 1.9;    // Adjust for 2:1 Voltage divider and internal reference


int emptyRaw ;
int emptyOld ;
float empty ;
int loadRaw ;
int currRaw ;
float load ;
float curr ;
int emptySum;
int loadSum;


void setup() {
  analogReference(INTERNAL);      // use precise internal reference

  OzOled.init();  //initialze OLED display

  OzOled.clearDisplay();           //clear the screen and set start position to top left corner
  OzOled.setNormalDisplay();       //Set display to Normal mode

  OzOled.sendCommand(0xA1);        // set Orientation
  OzOled.sendCommand(0xC8);

  pinMode(FET, OUTPUT); //Initialize pin that controls the MOSFET and battery load
}

void loop() {

  digitalWrite(FET, LOW); //Turn off load on battery

  // wait till value stable - read ADC until difference between readings is less than 3
  do {
    emptyOld = emptyRaw;
    emptyRaw = analogRead(ADCVOLT);
    delay(1); //Allow settling time between ADC reads
  } while (abs(emptyRaw - emptyOld) > 3);

  // Average over 10 cycles
  emptySum = 0;
  for (int i = 0; i < 10; i++) {
    emptySum = emptySum + analogRead(ADCVOLT);
    delay(1); //Allow settling time between ADC reads
  }
  //Calculate actual voltage. Each step of the ADC is 1.1 volts / 1024. We multiply 1024 by 10 for averaging.
  //Both the 1.1 reference and divide by 2 resistor divider are accounted for in the FACTOR constant term, which
  //   can be adjusted for resistor inaccuracies.
  empty = emptySum  / 10240.0 * FACTOR;

  //No-load voltage is calculated, now turn on load and calculate new voltage
  digitalWrite(FET, HIGH);             // switch load resistors on (nominally 2-1 ohm resistors in series)


  // wait till value stable - read ADC until difference between readings is less than 3
  do {
    emptyOld = loadRaw;
    loadRaw = analogRead(ADCVOLT);
    delay(1); //Allow settling time between reads
  }  while (abs(loadRaw - emptyOld) > 3);
  
  // Average over 10 cycles
  loadSum = 0;
  for (int i = 0; i < 10; i++) {
    loadSum = loadSum + analogRead(ADCVOLT);
    delay(1); //Allow settling time between ADC reads
  }
  //Calculate actual voltage. Each step of the ADC is 1.1 volts / 1024. We muktiply 1024 by 10 for averaging.
  //Both the 1.1 reference and divide by 2 resistor divider are accounted for in the FACTOR constant term, which
  //   can be adjusted for resistor inaccuracies.
  load = loadSum / 10240.0 * FACTOR;


  //Now calculate the current (load) being drawn with load switched on. Voltage is now stable, so just read the value of the
  //   current sense voltage divider (2-1 ohm resistors in series to stay below 1.1 volt reference voltage).
  //ADC connection is between the 2-1 ohm resistors.
  //Current through the sense resistors is (Vbattery / 2ohms) or typically (1.05 / 2) = 525ma
  //Voltage at the 50% divider is (1.05 - (1.05 / 2)) = 0.525 volts, the same as the current through both resistors!
  //In the special case of 2-1ohm resistors the voltage at the middle of the divider is equal to the current.

  currRaw = analogRead(ADCCURRENT); //Read the 2nd ADC to get sense resistor voltage
  if (currRaw < 10) {currRaw = 0;} //Supress any reading below 10 as noise to avoid display flickering
  //Now calculate the actual current being drawn from the battery in milliamps. This is the voltage drop across one of the two series
  //   sense resistors times 1000 for milliamps. 
  curr = currRaw * (1.1 / 1024.0) * 1000;

  digitalWrite(FET, LOW);   // switch load off

  displayOLED(); //Display the results

  delay(500); //Wait 1/2 second between reads to avoid excessive load on the battery

}

//Routine to display results
void displayOLED() {
  OzOled.printString("   1.5 Volt", 0, 0, 11); //Print the first line of the header in yellow area of display
  OzOled.printString("Battery Tester", 0, 1, 14); //Print the second line of the header in yellow area of display
  
  char tmp[10]; //Temporary character array for numerical conversion to a string
  
  dtostrf(empty, 1, 2, tmp); //Convert the no-load voltage to a string with 2 decimal places of accuracy
  OzOled.printString("No Load: ", 0, 3, 9); //Print static label for no-load voltage reading
  OzOled.printString(tmp, 10, 3, 4); //Print the actual no-load voltage on the same line
  
  dtostrf(load, 1, 2, tmp); //Convert the loaded voltage to a string with 2 decimal places of accuracy
  OzOled.printString("Loaded:  ", 0, 5, 9); //Print static label for loaded voltage reading
  OzOled.printString(tmp, 10, 5, 4); //Print the actual loaded voltage on the same line
  
  dtostrf(curr, 3, 0, tmp); //Convert the load current to a 3-digit string with no decimal places (ma)
  OzOled.printString("Load(ma): ", 0, 7, 10); //Print static label for load current reading
  OzOled.printString(tmp, 11, 7, 3); //Print the actual current load in ma on the same line
}

