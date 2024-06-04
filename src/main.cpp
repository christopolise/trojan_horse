#include <WiFi.h>
#include <esp_wifi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include "SPIFFS.h"

#define SERVER_LOC "http://192.168.0.196:5000/add"
#define PKT_NUM 50

// Define your network credentials
String ssid, passwd, addrPktList;      // Replace with your network SSID
bool loggedIn;
AsyncWebServer * server;

unsigned long previousMillis = 0;
const long interval = 10000;  // interval to wait for Wi-Fi connection

const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";
const char* addrListPath = "/addrList.txt";

// Structure for capturing Wi-Fi packets
typedef struct {
  unsigned frame_ctrl:16;
  unsigned duration_id:16;
  uint8_t addr1[6];
  uint8_t addr2[6];
  uint8_t addr3[6];
  unsigned sequence_ctrl:16;
  uint8_t addr4[6];
} wifi_ieee80211_mac_hdr_t;

typedef struct {
  wifi_ieee80211_mac_hdr_t hdr;
  wifi_promiscuous_pkt_type_t type;
  uint8_t * payload;
} sniff_pkt_t;

sniff_pkt_t pktList[PKT_NUM];
static int pktListIndex = 0;

typedef struct {
  wifi_ieee80211_mac_hdr_t hdr;
  uint8_t payload[];
} wifi_ieee80211_packet_t;

void sniffer_callback(void* buf, wifi_promiscuous_pkt_type_t type);
void handlePost(AsyncWebServerRequest * request);
void handleLogin(AsyncWebServerRequest * request);
void switchNetworks();
void sendPOSTRequest(String pkt);
const char *wifi_sniffer_packet_type2str(wifi_promiscuous_pkt_type_t type);


// Initialize SPIFFS
void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");
}

// Read File from SPIFFS
String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if(!file || file.isDirectory()){
    Serial.println("- failed to open file for reading");
    return String();
  }
  
  String fileContent;
  while(file.available()){
    fileContent = file.readStringUntil('\n');
    break;     
  }
  return fileContent;
}

// Write file to SPIFFS
void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
}

// Initialize WiFi
bool initWiFi() {
  if(ssid==""){
    Serial.println("Undefined SSID.");
    return false;
  }

  //IPAddress subnet = IPAddress(255, 255, 255, 0);

  WiFi.mode(WIFI_STA);
 // IPAddress localIP = IPAddress(10, 32, 123, 74);
//  localIP.fromString(ip.c_str());
 // IPAddress localGateway = IPAddress(10, 32, 123, 1);
//  localGateway.fromString(gateway.c_str());


  //if (!WiFi.config(localIP, localGateway, subnet)){
  //  Serial.println("STA Failed to configure");
  //  return false;
  //}
  WiFi.begin(ssid.c_str(), passwd.c_str());
  Serial.println("Connecting to WiFi...");

  unsigned long currentMillis = millis();
  previousMillis = currentMillis;

  while(WiFi.status() != WL_CONNECTED) {
    currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      Serial.println("Failed to connect.");
      return false;
    }
  }

  Serial.println(WiFi.localIP());
  return true;
}

void setup() {
  //loggedIn = false;
  initSPIFFS();
  Serial.begin(115200);
  delay(10);

  // Load values saved in SPIFFS
  ssid = readFile(SPIFFS, ssidPath);
  passwd = readFile(SPIFFS, passPath);
  Serial.println(ssid);
  Serial.println(passwd);

  // If pkt file exists
  if (SPIFFS.exists(addrListPath)) {
    addrPktList = readFile(SPIFFS, addrListPath);
    Serial.println(addrPktList);

    // Erase the file
    SPIFFS.remove(addrListPath);
  }

  if(initWiFi())
  {
    sendPOSTRequest(addrPktList);
    // Do the packet sniffing junk
    switchNetworks();
  }
  else
  {

    server = new AsyncWebServer(80);
    // Set up routes
    server->on("/post", HTTP_POST, &handlePost);
    server->on("/", HTTP_GET, &handleLogin);

    // Set up Wi-Fi as both AP and STA
    WiFi.mode(WIFI_AP_STA);

    // Set up the AP
    WiFi.softAP("IoT_Device");

    IPAddress address = WiFi.softAPIP();

    Serial.print("AP IP is: ");
    Serial.println(address);

    // Start the web server
    server->begin();
  }
}

void loop() {
  // if (loggedIn) switchNetworks();
  // sendPOSTRequest("00:00:00:00:00:00", "00:00:00:00:00:00", "00:00:00:00:00:00");
}

const char *wifi_sniffer_packet_type2str(wifi_promiscuous_pkt_type_t type)
{
    switch (type)
    {
    case WIFI_PKT_MGMT:
        return "MGMT";
    case WIFI_PKT_DATA:
        return "DATA";
    default:
    case WIFI_PKT_MISC:
        return "MISC";
    }
}

void sendPOSTRequest(String pkt) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    Serial.println("Connecting to server...");
    // Ensure the URL includes the protocol and the IP address is correct
    if (!http.begin(SERVER_LOC)) {
      Serial.println("HTTP begin failed");
      return;
    }

    http.addHeader("Content-Type", "application/json");

    // String jsonPayload = "{\"addr1\":\"" + String(addr1) + "\", \"addr2\":\"" + String(addr2) + "\", \"addr3\":\"" + String(addr3) + "\"}";
    // Serial.println("JSON Payload: " + jsonPayload);

    int httpResponseCode = http.POST(pkt);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("HTTP Response code: " + String(httpResponseCode));
      Serial.println("Response: " + response);
    } else {
      String errorMsg = http.errorToString(httpResponseCode).c_str();
      Serial.println("Error on HTTP request");
      Serial.println("HTTP Response code: " + String(httpResponseCode));
      Serial.println("Error: " + errorMsg);

      // Additional error handling for connection refused
      if (httpResponseCode == -1) {
        Serial.println("Connection refused. Please check the server and network configuration.");
      }
    }

    http.end(); // Free resources
  } else {
    Serial.println("WiFi Disconnected");
    ESP.restart();
  }
}

// Callback function to process captured packets
void sniffer_callback(void* buf, wifi_promiscuous_pkt_type_t type) {
  //if (type != WIFI_PKT_DATA || type != WIFI_PKT_MISC) return;

  wifi_promiscuous_pkt_t *p = (wifi_promiscuous_pkt_t *)buf;
  wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)p->payload;
  wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;

  Serial.printf("Addr1:\t%02X:%02X:%02X:%02X:%02X:%02X\tAddr2:\t%02X:%02X:%02X:%02X:%02X:%02X\tAddr3:\t%02X:%02X:%02X:%02X:%02X:%02X\n",
      hdr->addr1[0],
      hdr->addr1[1],
      hdr->addr1[2],
      hdr->addr1[3],
      hdr->addr1[4],
      hdr->addr1[5],
      hdr->addr2[0],
      hdr->addr2[1],
      hdr->addr2[2],
      hdr->addr2[3],
      hdr->addr2[4],
      hdr->addr2[5],
      hdr->addr3[0],
      hdr->addr3[1],
      hdr->addr3[2],
      hdr->addr3[3],
      hdr->addr3[4],
      hdr->addr3[5]
      );

  // Add the packet to the list
  pktList[pktListIndex].hdr = *hdr;
  pktList[pktListIndex].type = type;
  pktList[pktListIndex].payload = ipkt->payload;
  pktListIndex++;

  if (pktListIndex == PKT_NUM) {
    // Write the list to a file
    File file = SPIFFS.open(addrListPath, FILE_WRITE);
    if (!file) {
      Serial.println("Failed to open file for writing");
      return;
    }

    String pktString;

    // Write the packet list to the file in JSON format with zero padding
    pktString = "[";
    for (int i = 0; i < PKT_NUM; i++) {
      pktString += "{\"addr1\":\"";
      pktString += String(pktList[i].hdr.addr1[0], HEX);
      pktString += ":";
      pktString += String(pktList[i].hdr.addr1[1], HEX);
      pktString += ":";
      pktString += String(pktList[i].hdr.addr1[2], HEX);
      pktString += ":";
      pktString += String(pktList[i].hdr.addr1[3], HEX);
      pktString += ":";
      pktString += String(pktList[i].hdr.addr1[4], HEX);
      pktString += ":";
      pktString += String(pktList[i].hdr.addr1[5], HEX);
      pktString += "\",\"addr2\":\"";
      pktString += String(pktList[i].hdr.addr2[0], HEX);
      pktString += ":";
      pktString += String(pktList[i].hdr.addr2[1], HEX);
      pktString += ":";
      pktString += String(pktList[i].hdr.addr2[2], HEX);
      pktString += ":";
      pktString += String(pktList[i].hdr.addr2[3], HEX);
      pktString += ":";
      pktString += String(pktList[i].hdr.addr2[4], HEX);
      pktString += ":";
      pktString += String(pktList[i].hdr.addr2[5], HEX);
      pktString += "\",\"payload\":\"";
      for (int j = 0; j < 46; j++) {
        pktString += String(pktList[i].payload[j], HEX);
        pktString += " ";
      }
      pktString += "\",\"type\":\"";
      pktString += String(wifi_sniffer_packet_type2str(pktList[i].type));
      pktString += "\"}";

      if (i < (PKT_NUM - 1)) {
        pktString += ",";
      }
    }

    pktString += "]";

    file.print(pktString);

    Serial.printf("Packet list written to file: %s\n", pktString.c_str());

    file.close();
    ESP.restart();
  }

  // Disable promiscuous mode
  // esp_wifi_set_promiscuous(false);
  //initWiFi();
  //sendPOSTRequest(addr1, addr2, addr3);
//  switchNetworks();
}

void handlePost (AsyncWebServerRequest * request){
  String msg;
  if (request->hasParam("ssid", true) && request->hasParam("passwd", true))
  {
    ssid = request->getParam("ssid", true)->value();
    passwd = request->getParam("passwd", true)->value();
    writeFile(SPIFFS, ssidPath, ssid.c_str());
    writeFile(SPIFFS, passPath, passwd.c_str());
    msg = "Received SSID and Password. Connecting...";
    ESP.restart();
  }
  else
  {
    msg = "SSID or Password missing!";
  }
  request->send(200, "text/plain", msg);
  loggedIn = true;
}

const char loginhtml[] = 
"<!DOCTYPE html>"
"<html lang=\"en\">"
"<head>"
"<meta charset=\"UTF-8\">"
"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
"<title>IoT Device Login</title>"
"<style>"
"body {"
"    display: flex;"
"    justify-content: center;"
"    align-items: center;"
"    height: 100vh;"
"    background-color: #f0f0f0;"
"    font-family: Arial, sans-serif;"
"}"
".login-container {"
"    background-color: #fff;"
"    padding: 20px;"
"    border-radius: 10px;"
"    box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);"
"}"
".login-container h2 {"
"    margin-bottom: 20px;"
"}"
".login-container label {"
"    display: block;"
"    margin-bottom: 5px;"
"}"
".login-container input {"
"    width: 100%;"
"    padding: 10px;"
"    margin-bottom: 15px;"
"    border: 1px solid #ccc;"
"    border-radius: 5px;"
"}"
".login-container button {"
"    width: 100%;"
"    padding: 10px;"
"    background-color: #28a745;"
"    border: none;"
"    border-radius: 5px;"
"    color: white;"
"    font-size: 16px;"
"    cursor: pointer;"
"}"
".login-container button:hover {"
"    background-color: #218838;"
"}"
"</style>"
"</head>"
"<body>"
"<div class=\"login-container\">"
"    <h2>Login to IoT Device</h2>"
"    <form action=\"/post\" method=\"POST\">"
"        <label for=\"ssid\">SSID:</label>"
"        <input type=\"text\" id=\"ssid\" name=\"ssid\" required>"
"        <label for=\"passwd\">Password:</label>"
"        <input type=\"password\" id=\"passwd\" name=\"passwd\" required>"
"        <button type=\"submit\">Login</button>"
"    </form>"
"</div>"
"</body>"
"</html>";

void handleLogin(AsyncWebServerRequest * request) {
  request->send(200, "text/html", loginhtml);
}

void switchNetworks(){ 
  
  //if (server)
  //{
  //  server->end();
  //  Serial.println("Server ended...");
  //  delete server;
  //  server = nullptr;
  //  Serial.println("Server freed...");
  //}

  // Disconnect from any existing Wi-Fi connection
  // esp_wifi_set_promiscuous(false);
  // Serial.println("Promiscuity disabled...");

  // Disconnect from any existing Wi-Fi connection
  //if (WiFi.status() == WL_CONNECTED || WiFi.status() == WL_DISCONNECTED) {
  //  WiFi.disconnect(true);
  //  Serial.println("WiFi disconnected...");
  //} else {
  //  Serial.println("WiFi was not connected...");
  //}

  //delay(5000);
  //Serial.println("Waiting to change to new AP");
  // WiFi.mode(WIFI_STA);
  //Serial.println("WiFi mode changed to STA...");
  //WiFi.begin(ssid.c_str(), passwd.c_str());
  //Serial.println("WiFi restarted...");

  //while (WiFi.status() != WL_CONNECTED) {
  //  delay(500);
  //  Serial.print(".");
  //}

  //Serial.println("\nConnected to new network!");
  
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&sniffer_callback);
  Serial.println("Promiscuous mode enabled");
  
  //loggedIn = false;

}
