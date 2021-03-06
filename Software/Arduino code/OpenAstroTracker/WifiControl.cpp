#include "WifiControl.hpp"
#include "Utility.hpp"

#ifdef WIFI_ENABLED

#define PORT 4030

WifiControl::WifiControl(Mount* mount, LcdMenu* lcdMenu) 
{
    _mount = mount;
    _lcdMenu = lcdMenu;
}

void WifiControl::setup() {

    LOGV2(DEBUG_WIFI,"Wifi: Starting up Wifi As Mode %d\n", WIFI_MODE);

  _cmdProcessor = MeadeCommandProcessor::instance();

  switch (WIFI_MODE) {
  case 0: // startup Infrastructure Mode
      startInfrastructureMode();
      break;
  case 1: // startup AP mode
      startAccessPointMode();
      break;
  case 2: // Attempt Infra, fail over to AP
      startInfrastructureMode();
      _infraStart = millis();
      break;
  }
}

void WifiControl::startInfrastructureMode()
{
    LOGV1(DEBUG_WIFI,"Wifi: Starting Infrastructure Mode Wifi");
    LOGV2(DEBUG_WIFI,"Wifi:    with host name: %s", String(HOSTNAME).c_str());
    LOGV2(DEBUG_WIFI,"Wifi:          for SSID: %s", String(INFRA_SSID).c_str());
    LOGV2(DEBUG_WIFI,"Wifi:       and WPA key: %s", String(INFRA_WPAKEY).c_str());

#if defined(ESP8266)
    WiFi.hostname(HOSTNAME);
#elif defined(ESP32)
    WiFi.setHostname(HOSTNAME);
#endif
    WiFi.begin(INFRA_SSID, INFRA_WPAKEY);
}

void WifiControl::startAccessPointMode()
{
    LOGV1(DEBUG_WIFI,"Wifi: Starting AP Mode Wifi");
    IPAddress local_ip(192, 168, 1, 1);
    IPAddress gateway(192, 168, 1, 1);
    IPAddress subnet(255, 255, 255, 0);
    
#if defined(ESP8266)
    WiFi.hostname(HOSTNAME);
#elif defined(ESP32)
    WiFi.setHostname(HOSTNAME);
#endif

    WiFi.softAP(HOSTNAME, OAT_WPAKEY);

    WiFi.softAPConfig(local_ip, gateway, subnet);
}

String wifiStatus(int status){
    if (status==WL_IDLE_STATUS) return "Idle.";
    if (status==WL_NO_SSID_AVAIL) return "No SSID available.";
    if (status==WL_SCAN_COMPLETED  ) return "Scan completed.";
    if (status==WL_CONNECTED       ) return "Connected!";
    if (status==WL_CONNECT_FAILED   ) return "Connect failed.";
    if (status==WL_CONNECTION_LOST  ) return "Connection Lost.";
    if (status==WL_DISCONNECTED     ) return "Disconnected.";
    return "#"+String(status);
}

String WifiControl::getStatus()
{
  String result = "1," + wifiStatus(WiFi.status()) + ",";
#ifdef ESP8266
  result += WiFi.hostname();
#elif defined(ESP32)
  result += WiFi.getHostname();
#endif

  result += "," + WiFi.localIP().toString() + ":" + PORT; 
  return result;
}

void WifiControl::loop()
{
    if (_status != WiFi.status()) {
        _status = WiFi.status();
        LOGV2(DEBUG_WIFI,"Wifi: Connected status changed to %s", wifiStatus(_status).c_str());
        if (_status == WL_CONNECTED) {
            _tcpServer = new WiFiServer(PORT);
            _tcpServer->begin();
            _tcpServer->setNoDelay(true);
#if defined(ESP8266)
            LOGV2(DEBUG_WIFI,"Wifi: Server status is %s", wifiStatus( _tcpServer->status()).c_str());
#endif
            _udp = new WiFiUDP();
            _udp->begin(4031);

            LOGV4(DEBUG_WIFI,"Wifi: Connecting to SSID %s at %s:%d", INFRA_SSID, WiFi.localIP().toString().c_str(), PORT);
        }
    }

    _mount->loop();

    if (_status != WL_CONNECTED) {
        infraToAPFailover();
        return;
    }

    tcpLoop();
    udpLoop();
}

void WifiControl::infraToAPFailover() {
    if (_infraStart != 0 && 
        !WiFi.isConnected() && 
        _infraStart + _infraWait < millis()) {

        WiFi.disconnect();
        startAccessPointMode();
        _infraStart = 0;

        LOGV1(DEBUG_WIFI,"Wifi: Could not connect to Infra, Starting AP.");
    }
}

void WifiControl::tcpLoop() {
    if (client && client.connected()) {
        while (client.available()) {
            String cmd = client.readStringUntil('#');
            LOGV2(DEBUG_WIFI,"WifiTCP: Query <-- %s#", cmd.c_str());
            String retVal = _cmdProcessor->processCommand(cmd);

            if (retVal != "") {
                client.write(retVal.c_str());
                LOGV2(DEBUG_WIFI,"WifiTCP: Reply --> %s", retVal.c_str());
            }

            _mount->loop();
        }
    }
    else {
        client = _tcpServer->available();
    }
}

void WifiControl::udpLoop()
{
    int packetSize = _udp->parsePacket();
    if (packetSize)
    {
        String lookingFor = "skyfi:";;

        String reply = "skyfi:";
        reply += HOSTNAME;
        reply += "@";
        reply += WiFi.localIP().toString();
        LOGV4(DEBUG_WIFI,"WifiUDP: Received %d bytes from %s, port %d", packetSize, _udp->remoteIP().toString().c_str(), _udp->remotePort());
        char incomingPacket[255];
        int len = _udp->read(incomingPacket, 255);
        incomingPacket[len] = 0;
        LOGV2(DEBUG_WIFI,"WifiUDP: Received: %s", incomingPacket);

        incomingPacket[lookingFor.length()] = 0;
        if (lookingFor.equalsIgnoreCase(incomingPacket)) {
            _udp->beginPacket(_udp->remoteIP(), 4031);
            /*unsigned char bytes[255];
            reply.getBytes(bytes, 255);
            _udp->write(bytes, reply.length());*/

#if defined(ESP8266)
            _udp->write(reply.c_str());
#elif defined(ESP32)
            _udp->print(reply.c_str());
#endif
            
            _udp->endPacket();
            LOGV2(DEBUG_WIFI,"WifiUDP: Replied: %s", reply.c_str());
        }
    }
}
#endif
