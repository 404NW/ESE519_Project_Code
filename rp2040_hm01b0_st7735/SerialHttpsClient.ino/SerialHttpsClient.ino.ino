#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

#include <WiFiClientSecureBearSSL.h>

ESP8266WiFiMulti WiFiMulti;

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP("YOUR_WIFI_NAME","YOUR_WIFI_PASSWORD");
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  if ((WiFiMulti.run() == WL_CONNECTED)) {
    digitalWrite(LED_BUILTIN, LOW);


    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);

    client->setInsecure();   // client->setFingerprint(fingerprint);
    
    HTTPClient https;

    String url = Serial.readStringUntil('\n');

    if (url.length()>1)
    {
      if (https.begin(*client, url)) {

        int httpCode = https.GET();

        if ((httpCode > 0) &&
            (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)) {
          Serial.println(https.getString());
        }
        else {
          Serial.println();
        }

        https.end();
      } else {
        Serial.println();
      }
    }
  }
  digitalWrite(LED_BUILTIN, HIGH);
}
