#include <WiFi.h>
#include <esp_wifi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// Define your network credentials
String ssid, passwd;      // Replace with your network SSID
bool loggedIn;
AsyncWebServer * server;

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
  uint8_t payload[];
} wifi_ieee80211_packet_t;

void sniffer_callback(void* buf, wifi_promiscuous_pkt_type_t type);
void handlePost(AsyncWebServerRequest * request);
void handleLogin(AsyncWebServerRequest * request);
void switchNetworks();

void setup() {
  loggedIn = false;
  server = new AsyncWebServer(80);
  Serial.begin(115200);
  delay(10);

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

void loop() {
  if (loggedIn) switchNetworks();
}

// Callback function to process captured packets
void sniffer_callback(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_DATA) return;

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

}

void handlePost (AsyncWebServerRequest * request){
  String msg;
  if (request->hasParam("ssid", true) && request->hasParam("passwd", true))
  {
    ssid = request->getParam("ssid", true)->value();
    passwd = request->getParam("passwd", true)->value();
    msg = "Received SSID and Password. Connecting...";
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
  server->end();
  Serial.println("Server ended...");
  free(server);
  Serial.println("Server freed...");

  // Disconnect from any existing Wi-Fi connection
  esp_wifi_set_promiscuous(false);
  Serial.println("Promiscuity disabled...");
  // WiFi.disconnect(true);
  // Serial.println("WiFi disconnected...");
  delay(5000);
  WiFi.mode(WIFI_STA);
  Serial.println("WiFi mode changed to STA...");
  WiFi.begin(ssid.c_str(), passwd.c_str());
  Serial.println("WiFi restarted...");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("Connected to new network!");
  esp_wifi_set_promiscuous(true);
  Serial.println("Promiscuous mode enabled");
  loggedIn = false;
}
