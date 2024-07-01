#include <Arduino.h>
#include "AsyncUDP.h"
#include <string.h>
#include <ETH.h>

AsyncUDP udp;

//IPv4
unsigned char dhcp_active = 1;
String IPaddress;
String Subnet;
String Gateway;
String DNS_1;
String DNS_2;
String Hostname;
bool eth_got_ip = false;

//MAC
String eth_mac_adress;
String eth_mac_adress_temp;
byte macBuffer[6];

//IPv6
String linklocalv6;
String linkglobalv6;
String IPv6address;

//global
bool eth_connected = false;
bool eth_has_connection = false;
String ethernet_connection_state = "no connection established";
unsigned char send_ack = 0;
unsigned long runtime_ms_connection = 0;
unsigned char first_start_up = 1;

bool eth_got_ip_v6 = false;
unsigned char eth_uses_ip_v6 = 1;

int udp_port = 10009;


void system_logging_line(String buffer, unsigned char command_direction){
    Serial.println(buffer);
}

void NetworkEvent(arduino_event_id_t event){
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      eth_has_connection = true;
      eth_got_ip = false;
      system_logging_line("ETH Started", 5);
      if (first_start_up == 1){
        eth_mac_adress_temp = ETH.macAddress();
        eth_mac_adress_temp.replace(":", "");
        Hostname = Hostname + "-" + eth_mac_adress_temp;
      }
      ETH.setHostname(Hostname.c_str());
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      eth_has_connection = true;
      eth_got_ip = false;
      system_logging_line("ETH Connected", 5);
      break;
    case ARDUINO_EVENT_ETH_GOT_IP6:
      eth_has_connection = true;
      eth_got_ip_v6 = true;

      if (ETH.hasLinkLocalIPv6()){
        system_logging_line("ETH IPv6 local: " + ETH.linkLocalIPv6().toString(), 5);
        linklocalv6 = ETH.linkLocalIPv6().toString();
      }

      if (ETH.hasGlobalIPv6()){
        system_logging_line("ETH IPv6 global: " + ETH.globalIPv6().toString(), 5);
        linkglobalv6 = ETH.globalIPv6().toString();
        IPv6address = ETH.globalIPv6().toString();
      }
      eth_connected = true;
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      eth_has_connection = true;
      eth_got_ip = true;
      system_logging_line("ETH MAC: " + ETH.macAddress(), 5);
      eth_mac_adress = ETH.macAddress();
      system_logging_line("IPv4: " + ETH.localIP().toString(), 5);
      if (ETH.fullDuplex()) {
        system_logging_line("FULL_DUPLEX, " + String(ETH.linkSpeed(), DEC) + "Mbps", 5);
      }else{
        system_logging_line("HALF_DUPLEX, " + String(ETH.linkSpeed(), DEC) + "Mbps", 5);
      }

      system_logging_line("Use this URL to connect: http://" + ETH.localIP().toString() + "/", 5);
      IPaddress = ETH.localIP().toString();
      Subnet = ETH.subnetMask().toString();
      Gateway = ETH.gatewayIP().toString();
      DNS_1 = ETH.dnsIP(0).toString();
      DNS_2 = ETH.dnsIP(1).toString();
      eth_connected = true;
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      eth_has_connection = false;
      eth_got_ip = false;
      system_logging_line("ETH Disconnected", 5);
      eth_connected = false;
      break;
    case ARDUINO_EVENT_ETH_STOP:
      eth_has_connection = false;
      eth_got_ip = false;
      system_logging_line("ETH Stopped", 5);
      eth_connected = false;
      break;
    default:
      break;
  }
}

void setup() {
  IPAddress udp_ip_v6;
  bool listen_v4 = false;
  bool listen_v6 = false;
  bool listen_multicast_v6 = false;

  Serial.begin(115200); 

  Serial.setDebugOutput(true);

  system_logging_line("Debug Console ready", 5);

  Network.onEvent(NetworkEvent);

  if (eth_uses_ip_v6 == 1){
    system_logging_line("ETH enable v6", 5);
    ETH.enableIPv6(true);
  }

  ETH.begin(ETH_PHY_KSZ8081, ETH_PHY_ADDR, ETH_PHY_MDC, ETH_PHY_MDIO, ETH_PHY_POWER, ETH_CLOCK_GPIO0_OUT);

  delay(20000); //Wait till Ethernet is ready

  udp_ip_v6.fromString("ff02::1"); //"ff02::1"
  listen_multicast_v6 = udp.listenMulticast(udp_ip_v6, udp_port, 0, TCPIP_ADAPTER_IF_ETH);

  // listen_v4 = udp.listen(udp_port);
  // listen_v6 = udp.listenIPv6();

  system_logging_line("UDP-Listen: MC[" + String(listen_multicast_v6) + "] v4:[" + String(listen_v4) + "] v6:[" + String(listen_v6) + "]", 5);

  if ((listen_v4)||(listen_v6)||(listen_multicast_v6)){
    ethernet_connection_state = "UDP Listening on IPv4+v6: "+IPaddress+" Port: "+String(udp_port, DEC);
    system_logging_line(ethernet_connection_state, 5);
    udp.onPacket([](AsyncUDPPacket packet) {
        String udp_data_rec = (char*)packet.data();
        system_logging_line("REC: " + udp_data_rec, 5);
    });
  }
}

void loop() {
  String temp_casambi_command = "";
  IPAddress udp_ip_2;
  int udp_send_rv = 0;

  delay(1000);
  //Send multicast
  udp_ip_2.fromString("ff02::1");
  //udp_ip_2.fromString(IPv6TestAdress);

  temp_casambi_command = "test";
  udp_send_rv = udp.writeTo((uint8_t*)(temp_casambi_command.c_str()),temp_casambi_command.length(), udp_ip_2, udp_port, TCPIP_ADAPTER_IF_MAX);

  system_logging_line("UDP_SEND [" + String(udp_send_rv) + "] to (" + udp_ip_2.toString() + ") " + temp_casambi_command, 5);  
}
