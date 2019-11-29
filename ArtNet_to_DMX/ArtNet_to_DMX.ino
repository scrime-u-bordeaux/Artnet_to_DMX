/*
  Art-Net DMX Controller
  Created by Rory McMahon
  Conceptinetics DMX Shield
  Arduino Mega 2560 // uno
  Ethernet Shield
*/
#include <DmxSimple.h>
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>         // UDP library from: bjoern@cs.stanford.edu 12/30/2008
#define short_get_high_byte(x) ((HIGH_BYTE & x) >> 8)
#define short_get_low_byte(x)  (LOW_BYTE & x)
#define bytes_to_short(h,l) ( ((h << 8) & 0xff00) | (l & 0x00FF) );
#include "FastLED.h"
#define NUM_LEDS 4
#define DATA_PIN 6
CRGB leds[NUM_LEDS];
byte mac[] = {0x90, 0xA2, 0xDA, 0x0D, 0xF5, 0x09} ; //the mac adress in HEX of ethernet shield or uno shield board
//byte ip[] = {192, 168, 0, 99}; // the IP adress of your device, that should be in same universe of the network you are using

// the next two variables are set when a packet is received
byte remoteIp[4];        // holds received packet's originating IP
unsigned int remotePort; // holds received packet's originating port

//customisation: Artnet SubnetID + UniverseID
//edit this with SubnetID + UniverseID you want to receive
byte SubnetID = {0};
byte UniverseID = {0};
short select_universe = ((SubnetID * 16) + UniverseID);

//customisation: edit this if you want for example read and copy only 4 or 6 channels from channel 12 or 48 or whatever.
const int number_of_channels = 512; //512 for 512 channels!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
const int start_address = 0; // 0 if you want to read from channel 1

//buffers
const int MAX_BUFFER_UDP = 580; //768;
char packetBuffer[MAX_BUFFER_UDP]; //buffer to store incoming data
byte buffer_channel_arduino[number_of_channels]; //buffer to store filetered DMX data
// art net parameters
unsigned int localPort = 6454;      // artnet UDP port is by default 6454
const int art_net_header_size = 17;
const int max_packet_size = 530;//576;
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

void setup() {
  Serial.begin(115200);
  //setup ethernet and udp socket
  Ethernet.begin(mac);
  //Ethernet.begin(mac, ip);
  Serial.println(Ethernet.localIP());
  Udp.begin(localPort);
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  DmxSimple.usePin(3);
  DmxSimple.maxChannel(number_of_channels);

  for (int a = 0; a < 7; a++) {
    for (int b = 0; b < NUM_LEDS; b++) {
      leds[b].r = random(255);
      leds[b].g = random(255);
      leds[b].b = random(255);
    }
    FastLED.show();
    DmxSimple.write(1, 255);
    delay(100);
    DmxSimple.write(1, 0);
    DmxSimple.write(2, 255);
    delay(100);
    DmxSimple.write(2, 0);
    DmxSimple.write(3, 255);
    delay(100);
    DmxSimple.write(3, 0);
  }
  for (int b = 0; b < NUM_LEDS; b++) {
    leds[b].r = 1;
    leds[b].g = 1;
    leds[b].b = 1;
  }
  FastLED.show();
}

void loop() {
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
      if (is_opcode_is_dmx = 1) {
        //read incoming universe
        incoming_universe = bytes_to_short(packetBuffer[15], packetBuffer[14])
                            //if it is selected universe DMX will be read
        if (incoming_universe == select_universe) {
          //getting data from a channel position, on a precise amount of channels
          for (int i = start_address; i < number_of_channels; i++) {
            //buffer_channel_arduino[i-start_address]= byte(packetBuffer[i+art_net_header_size+1]);
            DmxSimple.write(i, byte(packetBuffer[i + art_net_header_size]));
          }
          for (int b = 0; b < NUM_LEDS; b++) {
              leds[b].r = byte(packetBuffer[art_net_header_size + 501 + b * 3]);
              leds[b].g = byte(packetBuffer[art_net_header_size + 502 + b * 3]);
              leds[b].b = byte(packetBuffer[art_net_header_size + 503 + b * 3]);
            }
            FastLED.show();
        }
      }
    }//end of sniffing
  }
}
