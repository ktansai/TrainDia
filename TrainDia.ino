#include <Arduino.h>
#include <ArduinoJson.h>
#include <M5Stack.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "time.h"

#include "WiFiConfig.h"

#define MODE_DEBUG 1

int mode = 0;

char httpResponceBuff[64];

const char* ntpServer =  "ntp.jst.mfeed.ad.jp"; //日本のNTPサーバー選択
const long  gmtOffset_sec = 9 * 3600;           //9時間の時差を入れる
const int   daylightOffset_sec = 0;             //夏時間はないのでゼロ

const char* url = "https://f568o9ukoc.execute-api.us-east-1.amazonaws.com/default/trainDiaLambda";

const int trainCount = 3;

class TrainDia{
    public: 
        struct tm timeinfo[trainCount];
        
        TrainDia(){
            timeinfo[0].tm_sec = 99;
            getNewDia();
        };

        void getNewDia(){
            HTTPClient http;
            http.begin(url);
            int httpCode = http.GET();
            if (httpCode == HTTP_CODE_OK) {
                String body = http.getString();
                body.toCharArray(httpResponceBuff,64);
                Serial.print("Response Body: ");
                Serial.println(body);
                
                StaticJsonBuffer<200> jsonBuffer;
                JsonObject& root = jsonBuffer.parseObject(body);

                for(int i = 0; i<trainCount; i++){
                    const char* time1 = root["train"][i];
                    int hours, minutes ;
                    sscanf(time1, "%2d:%2d", &hours, &minutes);
                    timeinfo[i].tm_hour = hours;
                    timeinfo[i].tm_min  = minutes;
                    timeinfo[i].tm_sec  = 0;
                }
            }
            return;
        };

        //不正かどうか確認
        bool shouldFetch(){
            if (timeinfo[0].tm_sec == 99){
                return true;
            }
            if (isPast()) {
                return true;
            }
            return false;

        };

        // 過ぎてるか判定
        bool isPast(){
            struct tm time_now;
            if (!getLocalTime(&time_now)) {
                Serial.println("Failed to obtain time");
                return false;
            }

            if (time_now.tm_hour * 60 + time_now.tm_min > timeinfo[0].tm_hour * 60 + timeinfo[0].tm_min) {
                Serial.println("past");
                return true;
            }
            else{
                return false;
            }
        };
    
    private:
};



TrainDia trainDia = TrainDia();


void printLocalTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%Y %m %d %a %H:%M:%S");    //日本人にわかりやすい表記へ変更
}






void setup()
{
    Serial.begin(115200);
    delay(10);

    // We start by connecting to a WiFi network

    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    M5.begin();
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

int value = 0;

void renderRemainingTime(){
    struct tm time_now;
    struct tm time_remaining;

    if (!getLocalTime(&time_now)) {
        M5.Lcd.println("Failed to obtain time");
        return;
    }

    int remain_seconds = 0;
    // remain_seconds += (trainDia.timeinfo[0].tm_hour - time_now.tm_hour) * (60 * 60) ; 
    // remain_seconds += (trainDia.timeinfo[0].tm_min  - time_now.tm_min)  * (60) ;
    // remain_seconds += (trainDia.timeinfo[0].tm_sec  - time_now.tm_sec);

    time_remaining = diffTime(trainDia.timeinfo[0], time_now);

    char str[20];
    sprintf(str,"%2d:%2d.%2d", time_remaining.tm_hour, time_remaining.tm_min, time_remaining.tm_sec);

    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextSize(2);
    M5.Lcd.drawCentreString(str, M5.Lcd.width()/2, M5.Lcd.height()/2 - 5*5, 4);
    // M5.Lcd.printf(text);
}

struct tm diffTime(struct tm time1 ,struct tm time2){
    struct tm result;
    int remain_seconds = 0;
    remain_seconds += (time1.tm_hour - time2.tm_hour) * (60*60);
    remain_seconds += (time1.tm_min - time2.tm_min)   * (60)   ;
    remain_seconds += (time1.tm_sec - time2.tm_sec)            ;

    result.tm_hour =  remain_seconds / (60 * 60); 
    remain_seconds -= result.tm_hour * (60 * 60);
    result.tm_min  =  remain_seconds / (60); 
    remain_seconds -= result.tm_min  * (60);
    result.tm_sec  = remain_seconds;

    return result;
}

void renderDebugConsole(char* text){
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        M5.Lcd.println("Failed to obtain time");
        return;
    }
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(0 ,0);

    M5.Lcd.println("Date: ");
    M5.Lcd.println(&timeinfo, "%Y %m %d %a %H:%M:%S");

    M5.Lcd.println("Response: ");
    M5.Lcd.println(text);
}

void loop()
{
    M5.update();

    if(trainDia.shouldFetch()){
        trainDia.getNewDia();
    }

    if(M5.BtnC.isPressed()){
        mode++;
        mode %= 2;
    }

    if(mode == MODE_DEBUG){
        renderDebugConsole(httpResponceBuff);
    }
    else{
        renderRemainingTime();
    }



    Serial.print("mode :");
    Serial.println(mode);
    delay(1000);
    printLocalTime();
}