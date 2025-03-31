#include <PromLokiTransport.h>
#include <PrometheusArduino.h>

#include "config.h"
#include "certificates.h"

// Define sensor values
int sensorPin = 11;
int sensorValue = 0;

// Prometheus client and transport
PromLokiTransport transport;
PromClient client(transport);

// Create a write request
WriteRequest req(1, 3074);

// Define TimeSeries
TimeSeries ts(5, "soil_moisture",  "{monitoring_type=\"gabby_plant\",board_type=\"esp32\",room=\"living_room\"}");

int loopCounter = 0;

void setup() {
  // Start the serial output at 9,600 baud 
  Serial.begin(9600);

  uint8_t serialTimeout;
  while (!Serial && serialTimeout < 50) {
      delay(100);
      serialTimeout++;
  }
  
  Serial.println("Starting");
  Serial.print("Free Mem Before Setup: ");
  Serial.println(freeMemory());

  // Configure and start the transport layer
  transport.setUseTls(true);
  transport.setCerts(grafanaCert, strlen(grafanaCert));
  transport.setWifiSsid(WIFI_SSID);
  transport.setWifiPass(WIFI_PASSWORD);
  transport.setDebug(Serial);  // Remove this line to disable debug logging of the client.
  if (!transport.begin()) {
    Serial.println(transport.errmsg);
    while (true) {};
  }

  // Configure the client
  client.setUrl(GC_PROM_URL);
  client.setPath((char*)GC_PROM_PATH);
  client.setPort(GC_PORT);
  client.setUser(GC_PROM_USER);
  client.setPass(GC_PROM_PASS);
  client.setDebug(Serial);  // Remove this line to disable debug logging of the client.
  if (!client.begin()) {
    Serial.println(client.errmsg);
    while (true) {};
  }

  // Add our TimeSeries to the WriteRequest
  req.addTimeSeries(ts);
  req.setDebug(Serial);  // Remove this line to disable debug logging of the write request serialization and compression.

  Serial.print("Free Mem After Setup: ");
  Serial.println(freeMemory());
}

void loop() {
  // Get current timestamp
  int64_t time;
  time = transport.getTimeMillis();

  // Read the analog value from the sensor
  sensorValue = analogRead(sensorPin);
  
  // Check if the soil is dry
  if (sensorValue > 500) {
    Serial.print(sensorValue);
    Serial.println(" - Status: Soil is too dry - time to water!");
  } else {
    Serial.print(sensorValue);
    Serial.println(" - Status: Soil is perfect!");
  }

  if (!ts.addSample(time, sensorValue)) {
      Serial.println(ts.errmsg);
  }

  loopCounter++;

  if (loopCounter >= 4) {
    // Send data
    loopCounter = 0;
    PromClient::SendResult res = client.send(req);
    if (!res == PromClient::SendResult::SUCCESS) {
        Serial.println(client.errmsg);
    }

    // Reset batches after a succesful send.
    ts.resetSamples();
}
  // Wait before taking another reading
  delay(5000);
}