// Import required libraries
#ifdef ESP32
  #include <WiFi.h>
  #include <AsyncTCP.h>
#else
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
#endif
#include "DHTesp.h" // Click here to get the library: http://librarymanager/All#DHTesp
#include <ESPAsyncWebServer.h>
#include <ESP8266HTTPClient.h> // HTTP 클라이언트

// Replace with your network credentials
const char* ssid = "iPhone";
const char* password = "1670017359";

const char* http_username = "admin";
const char* http_password = "admin";

const char* PARAM_INPUT_1 = "state";

const int output = 2;


String sido = "전북"; // 서울, 부산, 대구, 인천, 광주, 대전, 울산, 경기, 강원, 충북, 충남, 전북, 전남, 경북, 경남, 제주, 세종 중 입력
String key = "PS0gkbQMcOlN0FgO6E17eg8xpV2Jx8pJENTftDxFFqpRPV9UC5oIcQZcklIykS8iRYJc674TSSR%2BnNo5KchDsw%3D%3D";
String url = "http://apis.data.go.kr/B552584/ArpltnInforInqireSvc/getCtprvnRltmMesureDnsty?serviceKey=PS0gkbQMcOlN0FgO6E17eg8xpV2Jx8pJENTftDxFFqpRPV9UC5oIcQZcklIykS8iRYJc674TSSR%2BnNo5KchDsw%3D%3D&returnType=xml&numOfRows=10&pageNo=1&sidoName=%EC%A0%84%EB%B6%81&ver=1.0";


float so2, co, o3, no2, pm10, pm25 = 0; // 아황산가스(이산화황), 일산화탄소, 오존, 이산화질소, 미세먼지, 초미세먼지
int score = 0;

float humidity=0;
float temperature=0;

 int RED = 0;
 int GREEN = 4;
 int BLUE = 5;


DHTesp dht;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 2.6rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 10px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 34px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 68px}
    input:checked+.slider {background-color: #2196F3}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
  </style>
</head>
<body>
  <h2>ESP Web Server</h2>
  <button onclick="logoutButton()">Logout</button>
  <button onclick="IoTButton()">IoT</button> 
  <p>Ouput - GPIO 2 - State <span id="state">%STATE%</span></p>
  %BUTTONPLACEHOLDER%
<script>function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ 
    xhr.open("GET", "/update?state=1", true); 
    document.getElementById("state").innerHTML = "ON";  
  }
  else { 
    xhr.open("GET", "/update?state=0", true); 
    document.getElementById("state").innerHTML = "OFF";      
  }
  xhr.send();
}
function logoutButton() {
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/logout", true);
  xhr.send();
  setTimeout(function(){ window.open("/logged-out","_self"); }, 1000);
}
function IoTButton(){
   var xhr = new XMLHttpRequest();
  xhr.open("GET", "/IoT", true);
  xhr.send();
  setTimeout(function(){ window.open("/IoT-page","_self"); }, 1000);

}
</script>
</body>
</html>
)rawliteral";

const char logout_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
          <p>Logged out or <a href="/">return to homepage</a>.</p>
  <p><strong>Note:</strong> close all web browser tabs to complete the logout process.</p>

        </div>
        <script>
       
        </script>
        </body></html>

)rawliteral";

const char IoTpage_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
        <html>
       
        <head><title>NodeMCU Webserver.</title> 
        <meta http-equiv=Content-Type Content=text/html charset=utf-8>
        <meta name=viewport content=width=device-width, user-scalable=no>
        
        <link rel=stylesheet href=https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css>
        <style type=text/css><!--
        body {font-size:11pt}
        h3 {color: #85144b font-size: 14pt}
        #footer {text-align: center margin-top: 20px}
        #footer p {font-size: 9pt color: #85144b}
        #login {max-width: 400pxmargin: 0 autopadding: 5px background: #E8D9FF} 
        #password {max-width: 400pxmargin: 0 autopadding: 5px background: #E8D9FF} 
        --></style>
        
        <script src=https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js></script>
        <script src=https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/js/bootstrap.min.js></script>
        </head>
        <body>
        
    <div class=container><h3>사물인터넷 홈페이지 입니다.</h3>
        <div><br>NodeMCU에서 돌아가는 웹서버입니다. <br>온습도 센서를 이용해서 실시간 정보를 볼 수 있습니다..<br>또는 미세먼지 농도를 체크할수 있습니다..<br><br></div>

        <h2>모니터링 화면</h2><ul class='nav nav-tabs'><li class='active'><a data-toggle='tab' href='#home'>Home</a></li><li><a data-toggle='tab' href='#menu1'>온도/습도</a></li>
        <li><a data-toggle='tab' href='#menu2'>LED</a></li><li><a data-toggle='tab' href='#menu3'>미세먼지 농도</a></li></ul>
        
        <div class='tab-content'><div id='home' class='tab-pane fade in active'><h3>안내</h3><p>보드에 연결된 각종 센서, 미세먼지를 체크할 수 있습니다. <br>각각의 탭을 선택하시기 바랍니다.</p></div>
        <div id='menu1' class='tab-pane fade'><h3>온도/습도</h3>
        <button onclick="testClick()">온습도확인</button>
        <p>온도,습도 센서의 값을 확인합니다.<br>
          <span id="temp"></span>
        </p></div>
        <div id='menu2' class='tab-pane fade'><h3>LED</h3><p>미세먼지 농도에따른 LED를 표시합니다. 30이하일때 "BLUE", 30초과 80 미만일때 "GREEN", 80 이상일때 "RED"</p></div>
        <div id='menu3' class='tab-pane fade'>
        <h3>미세먼지 농도</h3>
        <button onclick="testClick2()">미세먼지농도확인</button>
        <p>미세먼지 농도를 체크합니다..</p>
        <span id="co"></span>
        <br>
         
         </div>
        </div>
        <script> function testClick() {
             var xhr = new XMLHttpRequest();
             xhr.open("GET", "/test", true);
             xhr.send();
             xhr.onload = () => {
            //통신 성공
            if (xhr.status == 200) {
                var temp = document.getElementById('temp');
                temp.innerHTML=xhr.response
                console.log(xhr.response);
                console.log("통신 성공");
            } else {
                //통신 실패
                console.log("통신 실패");
            }
        }
           
             //setTimeout(function(){ window.open("/logged-out","_self"); }, 1000);
        }   
              function testClick2() {
             var xhr = new XMLHttpRequest();
             xhr.open("GET", "/test2", true);
             xhr.send();
               xhr.onload = () => {
            //통신 성공
            if (xhr.status == 200) {
                var co = document.getElementById('co');
                co.innerHTML=xhr.response
                console.log(xhr.response);
                console.log("통신 성공");
            } else {
                //통신 실패
                console.log("통신 실패");
            }
        }
             //setTimeout(function(){ window.open("/logged-out","_self"); }, 1000);
        }
        </script>
       
        <div id='footer'><p>201668044 양진석, 201668013 장경원, 201668037 박형환</p></div>
         <p> <a href="/">return to homepage</a>.</p>
         </body></html>
)rawliteral";

  
// Replaces placeholder with button section in your web page
String processor(const String& var){
  //Serial.println(var);
  if(var == "BUTTONPLACEHOLDER"){
    String buttons ="";
    String outputStateValue = outputState();
    buttons+= "<p><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"output\" " + outputStateValue + "><span class=\"slider\"></span></label></p>";
    return buttons;
  }
  if (var == "STATE"){
    if(digitalRead(output)){
      return "ON";
    }
    else {
      return "OFF";
    }
  }
  return String();
}

String outputState(){
  if(digitalRead(output)){
    return "checked";
  }
  else {
    return "";
  }
  return "";
}


void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);

  pinMode(output, OUTPUT);
  digitalWrite(output, LOW);
  
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);
  
  delay(1000);
  Serial.println();
  Serial.println("Status\tHumidity (%)\tTemperature (C)\t(F)");
  String thisBoard= ARDUINO_BOARD;
  Serial.println(thisBoard);
  dht.setup(13, DHTesp::DHT11); 
   
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    if(!request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    request->send_P(200, "text/html", index_html, processor);
  });
    
  server.on("/logout", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(401);
  });

  server.on("/logged-out", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", logout_html, processor);
  });
  
   server.on("/IoT-page", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", IoTpage_html, processor);
  });

  server.on("/test", HTTP_GET, [](AsyncWebServerRequest *request){
    String output= "습도(%): "+String(humidity)+" "+"온도(C): "+String(temperature);
   request->send(200,"text/plain",output);
  });
  
  server.on("/test2", HTTP_GET, [](AsyncWebServerRequest *request){
    String output = "일산화탄소(co): "+String(co)+'\n'+"이산화항(so2): "+String(so2)+'\n'+"오존(o3): "+String(o3)+'\n'+"이산화질소(no2): "+String(no2)+'\n'+"미세먼지(pm10): "+String(pm10)+'\n'+"초미세먼지(pm25): "+String(pm25);
    request->send(200,"text/plain",output);
  });

  // Send a GET request to <ESP_IP>/update?state=<inputMessage>
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    if(!request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    String inputMessage;
    String inputParam;
    // GET input1 value on <ESP_IP>/update?state=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
      digitalWrite(output, inputMessage.toInt());
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    Serial.println(inputMessage);
    request->send(200, "text/plain", "OK");
  });
  
  // Start server
  
  server.begin();
  
}
  
void loop() {
  
  humidity = dht.getHumidity();
  temperature = dht.getTemperature();

  Serial.print(dht.getStatusString());
  Serial.print("\t");
  Serial.print(humidity, 1);
  Serial.print("\t\t");
  Serial.print(temperature, 1);
  Serial.print("\t\t");
  Serial.print(dht.toFahrenheit(temperature), 1);
  Serial.print("\n");

  delay(2000);
  
  
  if (WiFi.status() == WL_CONNECTED) // 와이파이가 접속되어 있는 경우
  {
    WiFiClient client; // 와이파이 클라이언트 객체
    HTTPClient http; // HTTP 클라이언트 객체

    if (http.begin(client, url)) {  // HTTP
      // 서버에 연결하고 HTTP 헤더 전송
      int httpCode = http.GET();

      // httpCode 가 음수라면 에러
      if (httpCode > 0) { // 에러가 없는 경우
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = http.getString(); // 받은 XML 데이터를 String에 저장
          int cityIndex = payload.indexOf("삼천동");
          so2 = getNumber(payload, "<so2Value>", cityIndex); // 아황산가스(이산화황)
          co = getNumber(payload, "<coValue>", cityIndex); // 일산화탄소
          o3 = getNumber(payload, "<o3Grade>", cityIndex); // 오존
          pm10 = getNumber(payload, "<pm10Value>", cityIndex); // 미세먼지
          no2 = getNumber(payload, "<khaiGrade>", cityIndex); // 이산화질소
          pm25 = getNumber(payload, "<pm25Value>", cityIndex); // 초미세먼지
        }
      } else {
        Serial.printf("[HTTP] GET… 실패, 에러코드: %s\n", http.errorToString(httpCode).c_str());
      }
      http.end();
       
    } else {
      Serial.printf("[HTTP] 접속 불가\n");
    }
     if (pm10 <= 30.0) {
   digitalWrite(RED,LOW);
   digitalWrite(GREEN,LOW);
   digitalWrite(BLUE,HIGH); 
   Serial.println(""); 
   Serial.println("BLUE");
  }
  else if(30.0 < pm10 && pm10 < 80.0) {
  digitalWrite(RED,LOW);
  digitalWrite(GREEN,HIGH);
  digitalWrite(BLUE,LOW);
  Serial.println(""); 
  Serial.println("GREEN");
  }
 else {
  digitalWrite(RED,HIGH);
  digitalWrite(GREEN,LOW);
  digitalWrite(BLUE,LOW);
  Serial.println(""); 
  Serial.println("RED");
  } 
    Serial.println(so2); // 아황산가스(이산화황)
    Serial.println(co); // 일산화탄소
    Serial.println(o3); // 오존
    Serial.println(no2); // 이산화질소
    Serial.println(pm10); // 미세먼지
    Serial.println(pm25); // 초미세먼지
    delay(600000);
  }
  

}

float getNumber(String str, String tag, int from) {
  float num = -1;
  int f = str.indexOf(tag, from) + tag.length();
  int t = str.indexOf("<", f);
  String s = str.substring(f, t);
  return s.toFloat();
}
