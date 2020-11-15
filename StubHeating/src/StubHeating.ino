/*
 * Project StubHeating
 * Description:
 * Author:
 * Date:
 */
#include "DHTStub.h"

DHTStub dht(D5);

// setup() runs once, when the device is first turned on.
void setup() {
  // Put initialization like pinMode and begin functions here.
  dht.begin();
  dht.updateData(-10, 100);
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {    
    dht.sendIfNeeded();
}


