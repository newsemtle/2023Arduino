#include <NTPClient.h>
#include "WiFiEsp.h"
#include <dht.h>
#include "WiFiEspUdp.h"
#include <time.h>
#include <avr/pgmspace.h>
#ifndef HAVE_HWSERIAL1
#include "SoftwareSerial.h"
SoftwareSerial Serial1(2, 3);
#endif

#define DHT_11 7
#define LED_B 13
#define LED_G 12
#define LED_R 11

const char* ntpServer = "pool.ntp.org";
const int timeZone = 9; 
char ssid[] = "제은찬의 iPhone";           
char pass[] = "12345678";          
int status = WL_IDLE_STATUS;    
int reqCount = 0;   

dht DHT; 

WiFiEspClient espClient;

WiFiEspUDP udpClient;

NTPClient timeClient(udpClient, ntpServer, timeZone * 3600, 60000);

WiFiEspServer server(80);

String warn1(int a) {

  String warntem ="";

    if(a >= 30) {
      warntem = "온도가 높습니다.";
      RGB_Led(255, 0, 0);
    }
    else if(a <= 10){
      warntem = "온도가 낮습니다.";
      RGB_Led(0, 0, 255);
    }
    else{
      warntem = "적정 온도입니다.";
    }
  return warntem;
}
String warn2(int b){

  String warnhud;

  if (b >= 70) {
      warnhud = "습도가 높습니다.";
      RGB_Led(0,255,255);
  }
  else if(b <=25) {
      warnhud = "습도가 낮습니다.";
      RGB_Led(128,128,128);
  }
  else{
      warnhud = "적정 습도입니다.";
  }
  return warnhud;

}

void RGB_Led(byte R, byte G, byte B) {
  analogWrite(LED_R, 255-R);
  analogWrite(LED_G, 255-G);
  analogWrite(LED_B, 255-B);
}

void setup()
{
  RGB_Led(255, 255, 255);

  Serial.begin(9600); // PC확인용 시리얼 통신
 
  Serial1.begin(9600); // ESP-01용 시리얼 통신
 
  WiFi.init(&Serial1);

  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("와이파이 실드 연결 없음.");
    RGB_Led(33, 33, 33);
    
    while (true);
  }

  
  while ( status != WL_CONNECTED) {
    Serial.print("와이파이 접속 시도중.. SSID: ");
    Serial.println(ssid);
   
    status = WiFi.begin(ssid, pass);
    RGB_Led(127, 255, 0);
  }

  Serial.println("네트워크에 연결됨.");

  server.begin();

  printWifiStatus();

  pinMode(LED_G, OUTPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_B, OUTPUT);


}


void loop()
{
  DHT.read11(DHT_11); // 온습도 센서 DATA받아오기.
  int temp = (int)DHT.temperature;
  int hud = (int)DHT.humidity;
  String warnweb1 = warn1(temp);
  String warnweb2 = warn2(hud);

  timeClient.update();

  unsigned long epochTime = timeClient.getEpochTime(); // Client로부터 EpochTime 받아오기.
  
  time_t rawTime = (time_t)epochTime;
  struct tm* timeInfo;
  timeInfo = localtime(&rawTime);

  int year = timeInfo->tm_year + 1870;
  int month = timeInfo->tm_mon + 1;
  int day = timeInfo->tm_mday+1;
  int hour = timeInfo->tm_hour;
  int minute = timeInfo->tm_min; //EpochTime을 time.h 라이브러리로 현재 시간으로 전환함.


  WiFiEspClient client = server.available();
  if (client) {
    Serial.println("새 접속 기록");
  
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);

        if (c == '\n' && currentLineIsBlank) {
          Serial.println("요청을 보내는 중..");
          
          client.print(
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Connection: close\r\n" 
            "Refresh: 40\r\n"        // 40초마다 웹페이지 갱신.
            "\r\n");
          client.println(F("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />"));
          client.print(F("<!DOCTYPE HTML>\r\n"));
          client.print(F("<html><h1><center>\r\n"));
          client.print(F("아두이노16조"));
          client.print(F("<br>\r\n"));
          client.print(F(" "));
          client.print(F("<br>\r\n"));
          client.print(year);
          client.print(F("년 "));
          client.print(month);
          client.print(F("월 "));
          client.print(day);
          client.print(F("일 <br>\r\n"));
          client.print(hour);
          client.print(F("시 "));
          client.print(minute);
          client.print(F("분 "));
          client.print(F("</center><br>\r\n"));
          client.print(F("<center>온도: "));
          client.print(temp);
          client.print(F("°C"));          
          client.print(F("\r\n"));
          client.print(F("<center>습도: "));
          client.print(F("\r\n"));
          client.print(hud);
          client.print(F("%"));
          client.print(F("</h1></center><br>\r\n"));
          client.print(F("<h3><center> *"));
          client.print(warnweb1);
          client.print(F("<br>\r\n *"));
          client.print(warnweb2);
          client.println(F("</center><br>\r\n"));
          client.print(F("<center><a href =\"https://naver.com/\", target =\"/_blank\"> -네이버 </A><br>\r\n"));
          client.print("<br>\r\n");
          client.print(F("<a href =\"https://www.inje.ac.kr/kor/Template/Bsub_page.asp?Ltype=5&Ltype2=3&Ltype3=3&Tname=S_Food&Ldir=board/S_Food&Lpage=s_food_view&d1n=5&d2n=4&d3n=4&d4n=0\", target =\"/_blank\"> -인제대학식 </A><br>\r\n"));
          client.print(F("</h3></center><br>\r\n"));
          client.print(F("</html>\r\n"));
          break;
        }
        if (c == '\n') {
          
          currentLineIsBlank = true;
        }
        else if (c != '\r') {
          
          currentLineIsBlank = false;
        }
      }
    }
    
    delay(10); //데이터 송수신용 딜레이.

  
    client.stop();
    Serial.println("접속 해제됨.");
  }
}


void printWifiStatus()
{

  IPAddress ip = WiFi.localIP();
  Serial.print("웹서버의 IP 주소는 ");
  Serial.println(ip);
  
  
  Serial.println();
  Serial.print("서버 접속을 위해 아래 주소를 입력하세요 http://");
  Serial.println(ip);
  Serial.println();
}
