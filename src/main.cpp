#include <ArduinoJson.h>
#include <rpcWiFi.h>
#include <TFT_eSPI.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <config.h>

/*
src/config.h should look like:

#pragma once

const char *SSID = <WIFI_SSID>;
const char *PASSWORD = <WIFI_PASSWORD>;
const char* CLIENT_ID = <CLIENT_ID_OF_REGISTERED_APP>;
const char* TENANT_ID = <TENANT_ID_OF_REGISTERED_APP>;
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
const int DELAY_FOR_REFRESH = 5000;

const char* root_ca_auth = \
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

const char* root_ca_graph = \
                    "-----BEGIN CERTIFICATE-----\n"
                    "MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh\n"
                    "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
                    "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\n"
                    "MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT\n"
                    "MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n"
                    "b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG\n"
                    "9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI\n"
                    "2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx\n"
                    "1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ\n"
                    "q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz\n"
                    "tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ\n"
                    "vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP\n"
                    "BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV\n"
                    "5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY\n"
                    "1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4\n"
                    "NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG\n"
                    "Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91\n"
                    "8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe\n"
                    "pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl\n"
                    "MrY=\n"
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
  client.setCACert(root_ca_auth);
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
  } else {
      connectWifi();
    }
}

void getAuthDeviceCode() {
  client.setCACert(root_ca_auth);
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
    } else {
      connectWifi();
    }
}

void fetchPresence() {
  client.setCACert(root_ca_graph);
  HTTPClient http;
  if(WiFi.status() == WL_CONNECTED){
      http.begin(client, "https://graph.microsoft.com/v1.0/me/presence");
      http.addHeader("Authorization", "Bearer " + String(accessToken));
      int httpResponseCode = http.GET();

      if (httpResponseCode > 0) {
        if (httpResponseCode == HTTP_CODE_OK) {
            String httpPayload = http.getString();

            DynamicJsonDocument payload(4096);
            deserializeJson(payload, httpPayload);

            String availability = payload["availability"].as<String>();
            String activity = payload["activity"].as<String>();

            if (activity == "OffWork") {
              tft.fillScreen(TFT_WHITE);
            } else if (activity == "DoNotDisturb" ||  activity == "InACall" || activity == "InAConferenceCall" || activity == "InAMeeting" || activity == "Presenting") {
              tft.fillScreen(TFT_RED);
            } else if (activity == "Available") {
              tft.fillScreen(TFT_GREEN);
            } else {
              tft.fillScreen(TFT_YELLOW);
            }

            tft.drawString(activity, 20, 40);
          } else {
            tft.drawString("Trying again....", 20, 20);
            delay(DELAY_FOR_RETRY);
            fetchPresence();
          }
      } else {
        tft.drawString("Trying again....", 20, 20);
        delay(DELAY_FOR_RETRY);
        fetchPresence();
      }
      http.end();  
    } else {
      connectWifi();
    }
}
 
void setup() {
    Serial.begin(9600);
    while(!Serial);

    connectWifi();

    tft.begin();
    tft.setRotation(3);

    tft.fillScreen(TFT_WHITE);
    tft.setTextColor(TFT_BLACK);
    tft.setTextSize(2);

    getAuthDeviceCode();
}
 
void loop() {
  if (authenticationCompleted) {
    fetchPresence();
    delay(DELAY_FOR_REFRESH);
  }
}