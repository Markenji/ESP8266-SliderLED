#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#define LED_RED     04 // D1
#define LED_GREEN   12 // D6
#define LED_BLUE    13 // D7

const char* ssid = "SSD";
const char* password = "PSW";
int contconexion = 0;

String pagina ="<html>"
"<head>"
"<script>"
"var connection = new WebSocket('ws://'+location.hostname+':81/', ['arduino']);"
"connection.onopen = function ()       { connection.send('Connect ' + new Date()); };"
"connection.onerror = function (error) { console.log('WebSocket Error ', error);};"
"connection.onmessage = function (e)   { console.log('Server: ', e.data);};"
"function sendRGB() {"
" var r = parseInt(document.getElementById('r').value).toString(16);"
" var g = parseInt(document.getElementById('g').value).toString(16);"
" var b = parseInt(document.getElementById('b').value).toString(16);"
" if(r.length < 2) { r = '0' + r; }"
" if(g.length < 2) { g = '0' + g; }"
" if(b.length < 2) { b = '0' + b; }"
" var rgb = '#'+r+g+b;"
" console.log('RGB: ' + rgb);"
" connection.send(rgb);"
"}"
"</script>"
"</head>"
"<body>"
"LED Control:<br/><br/>"
"R: <input id='r' type='range' min='0' max='255' step='1' value='0' oninput='sendRGB();'/><br/>"
"G: <input id='g' type='range' min='0' max='255' step='1' value='0' oninput='sendRGB();'/><br/>"
"B: <input id='b' type='range' min='0' max='255' step='1' value='0'oninput='sendRGB();'/><br/>"
"</body>"
"</html>";

ESP8266WebServer server (80);
WebSocketsServer webSocket = WebSocketsServer(81);

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED: {
            IPAddress ip = webSocket.remoteIP(num);
            Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

            // send message to client
            webSocket.sendTXT(num, "Connected");
        }
            break;
        case WStype_TEXT:
            Serial.printf("[%u] get Text: %s\n", num, payload);

            if(payload[0] == '#') {
                // we get RGB data

                // decode rgb data
                uint32_t rgb = (uint32_t) strtol((const char *) &payload[1], NULL, 16);

                analogWrite(LED_RED,    abs(255 - (rgb >> 16) & 0xFF) );
                analogWrite(LED_GREEN,  abs(255 - (rgb >>  8) & 0xFF) );
                analogWrite(LED_BLUE,   abs(255 - (rgb >>  0) & 0xFF) );
            }
            break;
    }
}

void setup() {
  
  Serial.begin(115200);
  Serial.println();

  WiFi.mode(WIFI_STA); //para que no inicie el SoftAP en el modo normal
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED and contconexion <50) { //Cuenta hasta 50 si no se puede conectar lo cancela
    ++contconexion;
    delay(500);
    Serial.print(".");
  }
  if (contconexion <50) {
     
      
      Serial.println("");
      Serial.println("WiFi conectado");
      Serial.println(WiFi.localIP());
  }
  else { 
      Serial.println("");
      Serial.println("Error de conexion");
  }

  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);

  // start webSocket server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  if(MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  // handle index
  server.on("/", []() {
      server.send(200, "text/html", pagina);
  });

  server.begin();

  // Add service to MDNS
  MDNS.addService("http", "tcp", 80);
  MDNS.addService("ws", "tcp", 81);

  digitalWrite(LED_RED,   1); // 1 = apagado
  digitalWrite(LED_GREEN, 1);
  digitalWrite(LED_BLUE,  1);

  analogWriteRange(255);

}

void loop() {
    webSocket.loop();
    server.handleClient();
}
