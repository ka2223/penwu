#include <Arduino.h>
#include <ESP8266WiFi.h> // 引入ESP8266WiFi库
#include <ESP8266WebServer.h> // 引入ESP8266WebServer库
#include <NTPClient.h> // 引入NTPClient库
#include <WiFiUdp.h> // 引入WiFiUdp库
#include <WiFiManager.h>          // 引入WiFiManager库  配网用

#include <EEPROM.h>

//const char* ssid = "TP-LINK_E068"; // 你的WiFi名称
//const char* password = "12345678"; // 你的WiFi密码
ESP8266WebServer server(80); // 创建一个Web服务器实例，监听80端口


const int ledPin = 2; // LED连接到2号端口
bool autoMode = false; // 自动模式初始状态为关闭

unsigned long ledOnTime = 1000; // LED点亮时间，单位：毫秒
unsigned long ledOffTime = 1000; // LED熄灭时间，单位：毫秒
unsigned long lastChangeTime = 0; // 上一次改变LED状态的时间
unsigned long lastChangeTime1 = 0; 
unsigned long currentTime = 0; // 获取当前时间
unsigned long lastUpdateTime = 0 ;  //时间更新周期

unsigned long epochTime = 0 ; //获取Unix时间戳
WiFiUDP ntpUDP; // 创建一个UDP实例
NTPClient timeClient(ntpUDP, "cn.pool.ntp.org", 28800); // 创建一个NTPClient实例，设置时间服务器为cn.pool.ntp.org，时区为东八区

int startHour = 12; // 自动模式开始小时数
int startMinute = 0; // 自动模式开始分钟数
int endHour = 17; // 自动模式结束小时数
int endMinute = 30; // 自动模式结束分钟数
int on_remiant = ledOnTime/1000 ;
int off_remiant = ledOffTime/1000;
float ver = 2.31 ;//版本信息
int  saved_on = 0 ;   //保存的信息
int  saved_off = 0 ;   //保存的信息
int  saved_xk  = 0;    //保存的信息
void setup() {
  pinMode(ledPin, OUTPUT); // 设置ledPin为输出模式
  digitalWrite(ledPin, HIGH); // 初始状态为熄灭
  Serial.begin(115200); // 设置串口波特率为115200
  WiFiManager wifiManager;       // 创建WiFiManager对象 初始化WiFiManager

  // 检查是否已保存网络配置
  if (WiFi.SSID() == "") {
    Serial.println("未找到已保存的网络配置，进入配网模式");
    wifiManager.setAPCallback(configModeCallback); // 设置配置模式回调函数
    wifiManager.setConfigPortalTimeout(180);       // 设置配置门户超时时间（秒）
    wifiManager.startConfigPortal("ESP8266_AP");   // 启动配置门户
  } else {
    // 尝试连接到已保存的网络
    WiFi.begin(WiFi.SSID().c_str(), WiFi.psk().c_str());
    Serial.print("尝试连接到已保存的网络: ");
    Serial.println(WiFi.SSID());

    // 等待连接
    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      timeout++;

      // 如果连接超时，进入配网模式
      if (timeout > 60) {
        Serial.println("\n连接超时，进入配网模式");
        wifiManager.setAPCallback(configModeCallback);
        wifiManager.setConfigPortalTimeout(180);
        wifiManager.startConfigPortal("ESP8266_AP");
        break;
      }
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\n连接成功");
      Serial.print("SSID: ");
      Serial.println(WiFi.SSID());
      Serial.print("密码: ");
      Serial.println(WiFi.psk());
    }
  }


// 初始化WiFi连接
 // WiFi.mode(WIFI_STA);
 // WiFi.begin(ssid, password); // 连接WiFi

  //while (WiFi.status() != WL_CONNECTED) { // 等待连接成功
  //  delay(500);
 //   Serial.print(".");
 // }
//  Serial.println("");
 // Serial.print("Connected to ");        //wifi 用户名 密码 连接
 // Serial.println(ssid);
 // Serial.print("IP address: ");
 // Serial.println(WiFi.localIP());
 // Serial.println("连接成功！");
   EEPROM.begin(1000); // 初始化EEPROM，2为每个数据的大小，单位为字节  
   EEPROM.get(8, saved_on);
   Serial.println(saved_on);
   EEPROM.get(12, saved_off);
   Serial.println(saved_off);
   EEPROM.get(16, saved_xk) ;   
   if(saved_on >0 ){
     ledOnTime = saved_on*1000 ; 
     Serial.print("on:"); 
     Serial.println( saved_on);
   }
   if(saved_off > 0  ){
     ledOffTime = saved_off*1000 ;  
     Serial.print("off:"); 
     Serial.println( saved_off);
   }

     Serial.print("off cs:"); 
     Serial.println( ledOffTime);

   if(saved_xk == 0  ){
     autoMode = false ;

   }else if(saved_xk == 1 ){
     autoMode = true ;
   }

  timeClient.begin(); // 启动NTPClient

  server.on("/", handleRoot); // 当访问根路径时，调用handleRoot函数
  server.on("/set", handleSet); // 当访问/set路径时，调用handleSet函数
  server.on("/on", handleOn); // 当访问/on路径时，调用handleOn函数
  server.on("/off", handleOff); // 当访问/off路径时，调用handleOff函数

 // server.on("/update", HTTP_POST, handleUpdate); // 将处理固件上传请求的函数绑定到Web服务器对象

  server.begin(); // 启动Web服务器
  Serial.println("HTTP server started");
    on_remiant = ledOnTime/1000 ;
    off_remiant = ledOffTime/1000 ;
}

void loop() {
   server.handleClient(); // 处理客户端请求
   currentTime = millis(); // 获取当前时间
  //timeClient.update(); // 更新时间
if(timeClient.getHours() == 8  ) {
    timeClient.update(); // 更新时间
    Serial.println("Time connecting...!");
 }
if (currentTime - lastUpdateTime >= 21600000) {
    timeClient.update();
    Serial.print("6小时更新时间:");
    lastUpdateTime = currentTime;
 }

  epochTime = timeClient.getEpochTime(); //获取Unix时间戳
 
  if (autoMode) { // 如果是自动模式    
    if (digitalRead(ledPin) == LOW && currentTime - lastChangeTime >= ledOffTime && checkTime(startHour, startMinute, endHour, endMinute, timeClient.getHours(), timeClient.getMinutes())) { // 如果LED熄灭且到了点亮时间且在定时范围内
      digitalWrite(ledPin, HIGH); // 点亮LED
      on_remiant = ledOnTime/1000 ;
      off_remiant = ledOffTime/1000 ;
      lastChangeTime = currentTime; // 更新上一次改变LED状态的时间
    }else if (digitalRead(ledPin) == HIGH && currentTime - lastChangeTime >= ledOnTime && checkTime(startHour, startMinute, endHour, endMinute, timeClient.getHours(), timeClient.getMinutes())) { // 如果LED点亮且到了熄灭时间且在定时范围
      digitalWrite(ledPin, LOW); // 熄灭LED
      on_remiant = ledOnTime/1000 ;
      off_remiant = ledOffTime/1000 ;

      lastChangeTime = currentTime; // 更新上一次改变LED状态的时间
    }


   if(checkTime(startHour, startMinute, endHour, endMinute, timeClient.getHours(), timeClient.getMinutes())){
        if(currentTime - lastChangeTime1 >= 1000){    
           if(digitalRead(ledPin) == LOW){
                    off_remiant  = off_remiant -1 ;  
                    lastChangeTime1 = currentTime;
          }else if (digitalRead(ledPin) == HIGH){
                    on_remiant  = on_remiant -1 ;
       
                   lastChangeTime1 = currentTime;
     }
  }
   }
    
  }
  delay(300) ;
}

void handleRoot() {
  String html = "<html><head><style>";
  html += "button {";
  html += "background-color: #FF4949;";
  html += "color: #FFFFFF;";
  html += "border: none;";
  html += "border-radius: 4px;";
  html += "cursor: pointer;";
  html += "}";
  html += "</style>";
  
  html +="<meta charset=\"utf-8\"><body style=\"text-align: center; font-size: 20px;\">"; // 创建一个HTML字符串，并设置居中和字体放大
  html += "<h1>Motor Control</h1>"; // 添加标题
  html += "<div style=\"font-size: 24px;\">"; // 设置字体放大
  html += "Time: " +  timeClient.getFormattedTime() + "<br>"; // 显示当前时间
  html += "</div>";
  html += "<form action=\"/set\" method=\"get\">"; // 创建一个表单，提交到/set路径
  html += " Auto mode: <input type=\"checkbox\" name=\"autoMode\" value=\"1\" style=\"width: 25px; height: 25px;\""; // 创建一个复选框，name为autoMode，value为1或0，并设置宽度和高度
  html += (autoMode ? "checked" : ""); // 如果autoMode为true，则在复选框上添加checked属性
  html += "><br>";
  html += "LED on time  (s): <input type=\"number\" name=\"onTime\" value=\"" + String(ledOnTime/1000) + "\" style=\"font-size: 21px;height:30px ; width:85;\">"; // 创建一个文本框，name为onTime，value为ledOnTime，并设置字体放大
  html += " 剩余: " + String(on_remiant)+ " s<br>" ;
  
  html += "LED off time  (s): <input type=\"number\" name=\"offTime\" value=\"" + String(ledOffTime/1000) + "\" style=\"font-size: 21px;height:30px ; width:85;\">"; // 创建一个文本框，name为offTime，value为ledOffTime，并设置字体放大
  html += " 剩余: " + String(off_remiant)+ " s" ;
  html += "<br>"; 
  html += "自动开始时间: <input type=\"number\" name=\"startHour\" value=\"" + String(startHour) + "\" style=\"font-size: 21px;height:30px ; width:60;\">:<input type=\"number\" name=\"startMinute\" value=\"" + String(startMinute) + "\" style=\"font-size: 21px;height:30px ; width:60;\"><br>"; // 创建一个文本框，name为startHour和startMinute，value为startHour和startMinute，并设置字体放大
  html += "自动结束时间: <input type=\"number\" name=\"endHour\" value=\"" + String(endHour) + "\" style=\"font-size: 21px;height:30px ; width:60;\">:<input type=\"number\"name=\"endMinute\" value=\"" + String(endMinute) + "\" style=\"font-size: 21px;height:30px ; width:60;\"><br>";  // 创建一个文本框，name为endHour和endMinute，value为endHour和endMinute，并设置字体放大 
 // html += "<input type="submit" value="Submit" style=\"width: 120px; height: 50px; font-size: 21px;\"><br>";  // 创建一个提交按钮，并设置宽度、高度和字体放大
  html += "<br>";
  html += "<input type=\"submit\" value=\"提交修改\" style=\"width: 120px; height: 50px; font-size: 21px;\"><br>";
  html += "</form>";
  html += "<br>";
  html += "<font >▼手动模式▼</font><br>";
  html += "<font >点击后自动模式自动取消</font><br>";
  //html += "<font >Cancel automatic mode using manual mode first.</font><br>";
  html += "<a href=\"/on\"><button style=\"width: 120px; height: 50px; font-size: 21px;\">打 开</button></a>&nbsp &nbsp"; // 创建一个链接，点击后调用handleOn函数，并设置宽度、高度和字体放大
  html += "<a href=\"/off\"><button style=\"width: 120px; height: 50px; font-size: 21px;\">关 闭</button></a>"; // 创建一个链接，点击后调用handleOff函数，并设置宽度、高度和字体放大
  html += "<br>";
   html += "<br>";
  html += "<form method='POST' action='/update' enctype='multipart/form-data'>"; // 创建一个上传表单
  html += "<input type='file' name='update'>";  // 添加文件上传按钮
  html += "<input type='submit' value='Update'>"; // 添加提交按钮
  html += "</form>";  // 结束上传表单
  
  html += "<br>";
  html += "版本: " + String(ver)+ "<br>"; //

  html += "</body></head></html>";

server.send(200, "text/html", html); // 返回网页内容 
}

void handleSet() {
     if (server.hasArg("autoMode")  ) { // 如果有autoMode参数 
         autoMode = server.arg("autoMode").toInt() == 1; // 设置autoMode 
        EEPROM.begin(500) ;
        EEPROM.put(16, 1);  //保存选中状态
        EEPROM.commit();
       } else { 
         autoMode = false;
        EEPROM.begin(500) ;
        EEPROM.put(16, 0);  // 
        EEPROM.commit(); 
          
         }
     if (server.hasArg("onTime")  && server.arg("onTime").toInt()*1000 != ledOnTime ) { // 如果有onTime参数 
        ledOnTime = server.arg("onTime").toInt()*1000; // 设置ledOnTime 
        EEPROM.begin(500) ;
        EEPROM.put(8, ledOnTime/1000);  // 将变量ledOnTime的值保存到EEPROM中
        EEPROM.commit();
        Serial.print(ledOnTime);
        on_remiant = ledOnTime/1000 ;
        off_remiant = ledOffTime/1000 ;
        Serial.println("ledOnTime已保存到EEPROM中");       
        lastChangeTime1 = currentTime;
       } 
     if (server.hasArg("offTime") && server.arg("offTime").toInt()*1000 != ledOffTime ) { // 如果有offTime参数 
         ledOffTime = server.arg("offTime").toInt()*1000; // 设置ledOffTime 
        EEPROM.begin(1000) ;
        EEPROM.put(12, ledOffTime/1000);  // 将变量ledOnTime的值保存到EEPROM中
        EEPROM.commit();
        on_remiant = ledOnTime/1000 ;
        off_remiant = ledOffTime/1000 ;
        Serial.print(ledOffTime);
        Serial.println("ledOffTime已保存到EEPROM中");     
        lastChangeTime1 = currentTime;   
       }
     if (server.hasArg("startHour")) { // 如果有startHour参数 
        startHour = server.arg("startHour").toInt(); // 设置startHour
       } 
     if (server.hasArg("startMinute")) { // 如果有startMinute参数 
         startMinute = server.arg("startMinute").toInt(); // 设置startMinute 
       } 
     if (server.hasArg("endHour")) { // 如果有endHour参数 
         endHour = server.arg("endHour").toInt(); // 设置endHour 
       } 
     if (server.hasArg("endMinute")) { // 如果有endMinute参数 
         endMinute = server.arg("endMinute").toInt(); // 设置endMinute
       }
       
server.sendHeader("Location", "/", true); // 设置重定向到根路径 
server.send(302, "text/plain", ""); // 发送302状态码
}

void handleOn() {
     digitalWrite(ledPin, LOW); // 点亮LED 
     autoMode = false;
     server.sendHeader("Location", "/", true); // 设置重定向到根路径 
     server.send(302, "text/plain", ""); // 发送302状态码 
      }

void handleOff() { 
  digitalWrite(ledPin, HIGH); // 熄灭LED 
  autoMode = false;
  server.sendHeader("Location", "/", true); // 设置重定向到根路径 
  server.send(302, "text/plain", ""); // 发送302状态码 
}

bool checkTime(int startHour, int startMinute, int endHour, int endMinute, int currentHour, int currentMinute) {

 if (startHour < endHour) { // 如果开始时间在结束时间之前 
    if (currentHour > startHour && currentHour < endHour) { // 如果当前时间在开始时间和结束时间之间 
        return true;
        } else if (currentHour == startHour && currentMinute >= startMinute) { // 如果当前时间在开始时间，并且分钟数大于等于开始分钟数 
        return true; 
        } else if (currentHour == endHour && currentMinute < endMinute) { // 如果当前时间在结束时间，并且分钟数小于结束分钟数
        return true;
       } else { 
        digitalWrite(ledPin, HIGH);
        return false; 
      }
      } else if (startHour > endHour) { // 如果开始时间在结束时间之后
          if (currentHour > startHour || currentHour < endHour) { // 如果当前时间在开始时间和结束时间之间 
              return true; 
            } else if (currentHour == startHour && currentMinute >= startMinute) { // 如果当前时间在开始时间，并且分钟数大于等于开始分钟数 
              return true;
            }
            
          }
       
       return false; 
  }
//配网相关
void configModeCallback(WiFiManager *myWiFiManager) {
  Serial.println("进入配置模式");
  Serial.print("连接到AP: ");
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

    
