/*
 * Project StubHeating
 * Description:
 * Author:
 * Date:
 */

#include "DHTStub.h"
#include "RoomTempModel.h"

DHTStub dht(D5);
RoomTempModel model;

int setExtTemp(String extra){
  int value = extra.toInt();
  model.setExtTemp(value);
  return 0;
}

int setHeaterOn(String extra){
  int value = extra.toInt();
  model.setHeaterEnabled(value > 0);
  return 0;
}

void setup() {
  Particle.function("setExtTemp", setExtTemp);
  Particle.function("setHeaterOn", setHeaterOn);

  dht.begin();
  model.begin();  

  dht.updateHumid(10);
}

void loop() {    
  static unsigned long time_now = 0;
  
  //if(dht.waitingResponse()){
    //model.getTemp();
  //  delay(1);
    //dht.send();
  //}
  if(millis() - time_now > model.getPeriodMs()){
      time_now = millis();
      model.run();
      dht.updateTemp(model.getTemp());
  }
  
}


