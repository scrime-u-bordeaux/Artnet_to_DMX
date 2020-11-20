#include <Bounce.h>
Bounce b1 = Bounce(21, 20);
Bounce b2 = Bounce(20, 20);
Bounce b3 = Bounce(23, 20);
Bounce b4 = Bounce(22, 20);
#include <EEPROM.h>
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>  / UDP library from: bjoern@cs.stanford.edu 12/30/2008
#include <TeensyDMX.h>
namespace teensydmx = ::qindesign::teensydmx; //
//constexpr uint8_t kTXPin = 17;
// Create the DMX sender on Serial1.
teensydmx::Sender dmxTx{Serial1};
#include <Tiny4kOLED.h>
//#include <DmxSimple.h> // !!!!!!!!!!!!!!!!!!!! NE MARCHE PAS SUR TEENSY LC §§§§§§§§§§§§§§!!!!!!!!!!!!!!!!!!
#include "ModernDos.h"
const DCfont *currentFont = FONT8X16MDOS; //!!!!!!!!!
#define short_get_high_byte(x) ((HIGH_BYTE & x) >> 8)
#define short_get_low_byte(x)  (LOW_BYTE & x)
#define bytes_to_short(h,l) ( ((h << 8) & 0xff00) | (l & 0x00FF) );
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};//the mac adress in HEX of ethernet shield or uno shield board
byte ip[] = {EEPROM.read(0), EEPROM.read(1), EEPROM.read(2), EEPROM.read(3)}; // the IP adress of your device, that should be in same universe of the network you are using
// the next two variables are set when a packet is received
byte remoteIp[4];        // holds received packet's originating IP
unsigned int remotePort; // holds received packet's originating port
//customisation: Artnet SubnetID + UniverseID //edit this with SubnetID + UniverseID you want to receive
byte SubnetID = {0};
byte UniverseID = {0};
short select_universe = ((SubnetID * 16) + UniverseID);
//customisation: edit this if you want for example read and copy only 4 or 6 channels from channel 12 or 48 or whatever.
const int number_of_channels = 512; //512 for 512 channels!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
const int start_address = 0; // 0 if you want to read from channel 1

//buffers
const int MAX_BUFFER_UDP = 768;//580; //768;
char packetBuffer[MAX_BUFFER_UDP]; //buffer to store incoming data
byte buffer_channel_arduino[number_of_channels]; //buffer to store filetered DMX data
// art net parameters
unsigned int localPort = 6454;      // artnet UDP port is by default 6454
const int art_net_header_size = 17;
const int max_packet_size = 530;//530;//576;
char ArtNetHead[8] = "Art-Net";
char OpHbyteReceive = 0;
char OpLbyteReceive = 0;
//short is_artnet_version_1=0;
//short is_artnet_version_2=0;
//short seq_artnet=0;
//short artnet_physical=0;
short incoming_universe = 0;
boolean is_opcode_is_dmx = 0;
boolean is_opcode_is_artpoll = 0;
boolean match_artnet = 1;
short Opcode = 0;
EthernetUDP Udp;

byte pos[12] = {0, 8, 16, 32, 40, 48, 64, 72, 80, 96, 104, 112};
int setPos = 0;
int temp;

void setup() {
  pinMode(20, INPUT_PULLUP);
  pinMode(21, INPUT_PULLUP);
  pinMode(22, INPUT_PULLUP);
  pinMode(23, INPUT_PULLUP);
  Serial.begin(115200);
  Serial.println("hello!");
  oled.begin();
  oled.setFont(currentFont);
  oled.on();
  oled.switchRenderFrame();
  oled.clear();
  oled.print("Connexion");
  oled.switchFrame();
  delay(500);
  Ethernet.begin(mac, ip);//Ethernet.begin(mac) for DHCP use

  //  if (Ethernet.begin(mac) == 0) {
  //    Serial.println("Failed to configure Ethernet using DHCP");
  //    // try to congifure using IP address instead of DHCP:
  //    Ethernet.begin(mac, ip);
  //  }
  Udp.begin(localPort);
  dmxTx.begin();

  for (int a = 0; a < 5; a++) {
    for (int i = 0; i < 512; i++) {
      dmxTx.set(i, 255);
    }
    delay(200);
    for (int i = 0; i < 512; i++) {
      dmxTx.set(i, 0);
    }
    delay(200);
  }
  dispOled();
}
void dispOled() {
  oled.clear();
  oled.setCursor(0, 0);
  //IPAddress ip = Ethernet.localIP();
  if (ip[0] < 100)oled.print("0"); if (ip[0] < 10)oled.print("0"); oled.print(ip[0]); oled.print(".");
  if (ip[1] < 100)oled.print("0"); if (ip[1] < 10)oled.print("0"); oled.print(ip[1]); oled.print(".");
  if (ip[2] < 100)oled.print("0"); if (ip[2] < 10)oled.print("0"); oled.print(ip[2]); oled.print(".");
  if (ip[3] < 100)oled.print("0"); if (ip[3] < 10)oled.print("0"); oled.print(ip[3]);
  oled.setCursor(pos[setPos], 2);
  oled.print("^");
  oled.switchFrame();
  //delay(50);
  //Serial.println(Ethernet.localIP());
}

void buttonread() {
  b1.update(); b2.update(); b3.update(); b4.update();
  if ( b1.fallingEdge() ) { //////////////////// setting ++
    ++setPos;  if (setPos >= 12)setPos = 0;
    dispOled();
  }
  if ( b2.fallingEdge() ) { ///////////////////// setting --
    --setPos;  if (setPos < 0)setPos = 11;
    dispOled();
  }
  if ( b3.fallingEdge() ) { ////////////////// boutton ++
    switch (setPos) {
      case 0:
        temp = ip[0];
        ip[0] = constrain(temp + 100, 0, 255) ;
        break;
      case 1:
        temp = ip[0];
        ip[0] = constrain(temp + 10, 0, 255) ;
        break;
      case 2:
        temp = ip[0];
        ip[0] = constrain(temp + 1, 0, 255) ;
        break;
      case 3:
        temp = ip[1];
        ip[1] = constrain(temp + 100, 0, 255) ;
        break;
      case 4:
        temp = ip[1];
        ip[1] = constrain(temp + 10, 0, 255) ;
        break;
      case 5:
        temp = ip[1];
        ip[1] = constrain(temp + 1, 0, 255) ;
        break;
      case 6:
        temp = ip[2];
        ip[2] = constrain(temp + 100, 0, 255) ;
        break;
      case 7:
        temp = ip[2];
        ip[2] = constrain(temp + 10, 0, 255) ;
        break;
      case 8:
        temp = ip[2];
        ip[2] = constrain(temp + 1, 0, 255) ;
        break;
      case 9:
        temp = ip[3];
        ip[3] = constrain(temp + 100, 0, 255) ;
        break;
      case 10:
        temp = ip[3];
        ip[3] = constrain(temp + 10, 0, 255) ;
        break;
      case 11:
        temp = ip[3];
        ip[3] = constrain(temp + 1, 0, 255) ;
        break;
      default:
        // statements
        break;
    }
    EEPROM.write(0, ip[0]); EEPROM.write(1, ip[1]); EEPROM.write(2, ip[2]); EEPROM.write(3, ip[3]);
    //Ethernet.begin(mac, ip);//Ethernet.setLocalIP(ip);
    dispOled();
  }
  if ( b4.fallingEdge() ) { ////////////////// boutton ++
    switch (setPos) {
      case 0:
        temp = ip[0];
        ip[0] = constrain(temp - 100, 0, 255) ;
        break;
      case 1:
        temp = ip[0];
        ip[0] = constrain(temp - 10, 0, 255) ;
        break;
      case 2:
        temp = ip[0];
        ip[0] = constrain(temp - 1, 0, 255) ;
        break;
      case 3:
        temp = ip[1];
        ip[1] = constrain(temp - 100, 0, 255) ;
        break;
      case 4:
        temp = ip[1];
        ip[1] = constrain(temp - 10, 0, 255) ;
        break;
      case 5:
        temp = ip[1];
        ip[1] = constrain(temp - 1, 0, 255) ;
        break;
      case 6:
        temp = ip[2];
        ip[2] = constrain(temp - 100, 0, 255) ;
        break;
      case 7:
        temp = ip[2];
        ip[2] = constrain(temp - 10, 0, 255) ;
        break;
      case 8:
        temp = ip[2];
        ip[2] = constrain(temp - 1, 0, 255) ;
        break;
      case 9:
        temp = ip[3];
        ip[3] = constrain(temp - 100, 0, 255) ;
        break;
      case 10:
        temp = ip[3];
        ip[3] = constrain(temp - 10, 0, 255) ;
        break;
      case 11:
        temp = ip[3];
        ip[3] = constrain(temp - 1, 0, 255) ;
        break;
      default:
        // statements
        break;
    }
    EEPROM.write(0, ip[0]); EEPROM.write(1, ip[1]); EEPROM.write(2, ip[2]); EEPROM.write(3, ip[3]);
    //Ethernet.begin(mac, ip);//Ethernet.setLocalIP(ip);
    dispOled();
  }
}
void loop() {
  buttonread();
  int packetSize = Udp.parsePacket();
  //FIXME: test/debug check
  if (packetSize > art_net_header_size && packetSize <= max_packet_size) {
    // Identify the IP/Port of the device sending us artnet packets
    IPAddress remote = Udp.remoteIP();
    Serial.println(remote);
    Serial.println(packetSize);
    remotePort = Udp.remotePort();
    Udp.read(packetBuffer, MAX_BUFFER_UDP);
    //read header
    match_artnet = 1;
    for (int i = 0; i < 7; i++) {
      //if not corresponding, this is not an artnet packet, so we stop reading
      if (char(packetBuffer[i]) != ArtNetHead[i]) {
        match_artnet = 0; break;
      }
    }
    //if its an artnet header
    if (match_artnet == 1) {
      //artnet protocole revision, not really needed
      //is_artnet_version_1=packetBuffer[10];
      //is_artnet_version_2=packetBuffer[11];*/
      //sequence of data, to avoid lost packets on routeurs
      //seq_artnet=packetBuffer[12];*/
      //physical port of  dmx N°
      //artnet_physical=packetBuffer[13];*/
      //operator code enables to know wich type of message Art-Net it is
      Opcode = bytes_to_short(packetBuffer[9], packetBuffer[8]);
      //if opcode is DMX type
      if (Opcode == 0x5000) {
        is_opcode_is_dmx = 1; is_opcode_is_artpoll = 0;
      }
      //if opcode is artpoll
      else if (Opcode == 0x2000) {
        is_opcode_is_artpoll = 1; is_opcode_is_dmx = 0;
        //( we should normally reply to it, giving ip adress of the device)
      }
      //if its DMX data we will read it now
      if (is_opcode_is_dmx == 1) {
        //read incoming universe
        incoming_universe = bytes_to_short(packetBuffer[15], packetBuffer[14])
                            //if it is selected universe DMX will be read
        if (incoming_universe == select_universe) {
          //getting data from a channel position, on a precise amount of channels
          for (int i = start_address; i < number_of_channels; i++) {
            //buffer_channel_arduino[i-start_address]= byte(packetBuffer[i+art_net_header_size+1]);
            //DmxSimple.write(i, byte(packetBuffer[i + art_net_header_size]));
            // Set channel 1 to 128
            dmxTx.set(i, byte(packetBuffer[i + art_net_header_size]));
          }
        }
      }
    }//end of sniffing
  }
}
