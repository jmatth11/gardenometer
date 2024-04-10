# gardenometer
arduino project to monitor garden metrics

Hardware for this part of the project:
- (Arduino Nano)[https://store-usa.arduino.cc/products/arduino-nano] The main board
- (DS18B20)[https://www.amazon.com/Gikfun-DS18B20-Temperature-Waterproof-EK1083x3/dp/B012C597T0] The temperature sensor
- (TEMT6000)[https://www.sparkfun.com/products/8688] The light sensor
- (Capacitive Soil Moisture Sensor v1.2)[https://gikfun.com/products/gikfun-capacitive-soil-moisture-sensor-corrosion-resistant-for-arduino-moisture-detection-garden-watering-diy-pack-of-2pcs] The moisture sensor for the soil.
- (ESP8266)[https://www.sparkfun.com/products/17146] The wifi module to talk to the home server. This part has a separate programmed project [here](https://github.com/jmatth11/gardenometer-http)

We setup and listen all of our sensors on their appropriate pins and send that data to the ESP8266 module to send out to our home server.

Functionality:
- Displaying when it is in an error state.
- Calibrating the capacitive soil moisture sensor when told via server.
- collect and calculate metrics from the sensors and write out to it's serial ports.

