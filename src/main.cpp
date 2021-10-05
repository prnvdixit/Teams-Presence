#include <ArduinoJson.h>
#include <rpcWiFi.h>
#include <TFT_eSPI.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <config.h>

/*
src/config.h should look like:

#pragma once

const char *SSID = WIFI_SSID;
const char *PASSWORD = WIFI_PASSWORD;
const char* CLIENT_ID = CLIENT_ID_OF_REGISTERED_APP;
const char* TENANT_ID = TENANT_ID_OF_REGISTERED_APP;
*/

TFT_eSPI tft;
WiFiClientSecure client;

String userCode;
String deviceCode;
String URL;
String accessToken;
String refreshToken;
int expirationTimer;

bool deviceCodeFetched = false;
bool authenticationCompleted = false;
bool waitingForDeviceCodeEnter = true;

const int DELAY_FOR_RETRY = 5000;
const char* root_ca = \
                    "-----BEGIN CERTIFICATE-----\n"
                    "MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh\n"
                    "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
                    "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\n"
                    "QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT\n"
                    "MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n"
                    "b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG\n"
                    "9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB\n"
                    "CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97\n"
                    "nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt\n"
                    "43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P\n"
                    "T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4\n"
                    "gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO\n"
                    "BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR\n"
                    "TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw\n"
                    "DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr\n"
                    "hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg\n"
                    "06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF\n"
                    "PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls\n"
                    "YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk\n"
                    "CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=\n"
                    "-----END CERTIFICATE-----\n";

void connectWifi() {

  WiFi.mode(WIFI_STA);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting to WiFi...");
    WiFi.begin(SSID, PASSWORD);
    delay(DELAY_FOR_RETRY);
  }

  Serial.println("Connected!");
}

void fetchAuthToken() {
  HTTPClient http;
  if(WiFi.status() == WL_CONNECTED){
      http.begin(client, "https://login.microsoftonline.com/" + String(TENANT_ID) + "/oauth2/v2.0/token");
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");

      String httpRequestData = "device_code=" + String(deviceCode) + "&grant_type=urn%3Aietf%3Aparams%3Aoauth%3Agrant-type%3Adevice_code&client_id=" + String(CLIENT_ID);

      int httpResponseCode = http.POST(httpRequestData);

      if (httpResponseCode > 0) {
        if (httpResponseCode == HTTP_CODE_OK) {
            String httpPayload = http.getString();

            DynamicJsonDocument payload(4096);
            deserializeJson(payload, httpPayload);

            accessToken = payload["access_token"].as<String>();
            refreshToken = payload["refresh_token"].as<String>();
            expirationTimer = payload["expires_in"].as<int>();;

            authenticationCompleted = true;
            tft.fillScreen(TFT_BLACK);
          } else {
            tft.drawString("Please enter the code....", 20, 60);
            delay(DELAY_FOR_RETRY);
            fetchAuthToken();
          }
      } else {
        tft.drawString("Please enter the code....", 20, 60);
        delay(DELAY_FOR_RETRY);
        fetchAuthToken();
      }
      http.end();  
  }
}

void getAuthDeviceCode() {
  HTTPClient http;
  if(WiFi.status() == WL_CONNECTED){
      http.begin(client, "https://login.microsoftonline.com/" + String(TENANT_ID) + "/oauth2/v2.0/devicecode");
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");

      String httpRequestData = "client_id=" + String(CLIENT_ID) + "&scope=user.read%20presence.read%20offline_access";

      int httpResponseCode = http.POST(httpRequestData);

      if (httpResponseCode > 0) {
        if (httpResponseCode == HTTP_CODE_OK) {
            String httpPayload = http.getString();

            DynamicJsonDocument payload(4096);
            deserializeJson(payload, httpPayload);

            userCode = payload["user_code"].as<String>();
            deviceCode = payload["device_code"].as<String>();
            URL = "aka.ms/devicelogin";

            Serial.println(userCode);
            deviceCodeFetched = true;

            tft.drawString(userCode, 20, 20);
            tft.drawString(URL, 20, 40);

            fetchAuthToken();
          } else {
            tft.drawString("Trying again....", 20, 20);
            delay(DELAY_FOR_RETRY);
            getAuthDeviceCode();
          }
      } else {
        tft.drawString("Trying again....", 20, 20);
        delay(DELAY_FOR_RETRY);
        getAuthDeviceCode();
      }
      http.end();  
    }
}
 
void setup() {
    Serial.begin(9600);
    while(!Serial);

    connectWifi();
    client.setCACert(root_ca);

    tft.begin();
    tft.setRotation(3);

    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);

    getAuthDeviceCode();
}
 
void loop() {
  if (authenticationCompleted) {

    waitingForDeviceCodeEnter = false;
    tft.drawString(String(expirationTimer), 20, 20);

  }
}