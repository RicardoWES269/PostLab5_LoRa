#include <Arduino.h>
#include <RadioLib.h>
#include <SPI.h>

/*~~~~~ Instance Configuration ~~~~*/

// prefix JAMNET

#define IS_GATEWAY        1      // 0 means is end device
#define CHANNEL_UPLINK   10      // 0-63?
#define CHANNEL_DOWNLINK 11
#define JN_ADDRESS       0xCAFE // 16bit?

#define JN_LORA_SPREAD   7     // low spreading factor, fast TX
#define JN_LORA_BW       125.0 // bw?
#define JN_INIT_FREQ     902.5 // initial frequency for Radio.begin?
                               // not the right thing, we'll use tune_to_rx() to init properly

/*~~~~~Hardware Definitions~~~~~*/

// These are hardware specific to the Heltec WiFi LoRa 32 V3
// Cite: https://resource.heltec.cn/download/WiFi_LoRa32_V3/HTIT-WB32LA(F)_V3_Schematic_Diagram.pdf
#define PRG_BUTTON 0
#define LORA_NSS_PIN 8
#define LORA_SCK_PIN 9
#define LORA_MOSI_PIN 10
#define LORA_MISO_PIN 11
#define LORA_RST_PIN 12
#define LORA_BUSY_PIN 13
#define LORA_DIO1_PIN 14

/*~~~~~Radio Configuration~~~~~*/

// Initialize SX1262 radio
// Make a custom SPI device because *of course* Heltec didn't use the default SPI pins
SPIClass spi(FSPI);
SPISettings spiSettings(2000000, MSBFIRST, SPI_MODE0); // Defaults, works fine
SX1262 radio = new Module(LORA_NSS_PIN, LORA_DIO1_PIN, LORA_RST_PIN, LORA_BUSY_PIN, spi, spiSettings);

/*~~~~~Function Prototypes~~~~~*/
void error_message(const char* message, int16_t state);

/*~~~~~Interrupt Handlers~~~~~*/
volatile bool receivedFlag = false;
volatile bool buttonFlag = false;
volatile bool repeatFlag = false;

// This function should be called when a complete packet is received.
//  It is placed in RAM to avoid Flash usage errors
#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void receiveISR(void) {
  // WARNING:  No Flash memory may be accessed from the IRQ handler: https://stackoverflow.com/a/58131720
  //  So don't call any functions or really do anything except change the flag
  receivedFlag = true;
}

// This function should be called when a complete packet is received.
//  It is placed in RAM to avoid Flash usage errors
#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void buttonISR(void) {
  // WARNING:  No Flash memory may be accessed from the IRQ handler: https://stackoverflow.com/a/58131720
  //  So don't call any functions or really do anything except change the flag
  buttonFlag = true;
}

/*~~~~~Helper Functions~~~~~*/
void error_message(const char* message, int16_t state) {
  Serial.printf("ERROR!!! %s with error code %d\n", message, state);
  while(true); // loop forever
}

/*====== My globals =====*/
String last_packet;

/*====== CHANNEL SCANNING =====*/
int curr_channel = -1;
// max channel = 40
float chan_i_to_freq(int chan) {
  return 902.3 + 0.2*chan;
}

void advance_channel() {
  curr_channel += 1;
  if (curr_channel > 40) { curr_channel = 0; }
  float freq = chan_i_to_freq(curr_channel);
  Serial.printf("Switching to channel %d, freq %f\n", curr_channel, freq);

  if (radio.setFrequency(freq) == RADIOLIB_ERR_INVALID_FREQUENCY) {
    Serial.println(F("Selected frequency is invalid for this module!"));
    while (true);
  }

  int state = radio.startReceive();
  if (state != RADIOLIB_ERR_NONE) {
    error_message("Starting reception failed", state);
  }
}

void tune_to_channel(int ch) {
  float freq = chan_i_to_freq(ch);
  Serial.printf("Switching to channel %d, freq %f\n", ch, freq);

  if (radio.setFrequency(freq) == RADIOLIB_ERR_INVALID_FREQUENCY) {
    Serial.println(F("Selected frequency is invalid for this module!"));
    while (true);
  }

  int state = radio.startReceive();
  if (state != RADIOLIB_ERR_NONE) {
    error_message("Starting reception failed", state);
  }
}

// Note: TX and RX are swapped for gateways and end devices, handle this
void tune_to_tx() { // Gateways transmits on downlink
  if (IS_GATEWAY) { tune_to_channel(CHANNEL_DOWNLINK); }
  else            { tune_to_channel(CHANNEL_UPLINK); }
}

void tune_to_rx() { // Gateway listens to uplink
  if (IS_GATEWAY) { tune_to_channel(CHANNEL_UPLINK); }
  else            { tune_to_channel(CHANNEL_DOWNLINK); }
}


/*~~~~~Application~~~~~*/
void setup() {
  Serial.begin(115200);

  // Set up GPIO pin for "PRG" button and enable interrupts for it
  pinMode(PRG_BUTTON, INPUT);
  attachInterrupt(PRG_BUTTON, buttonISR, FALLING);

  // Set up SPI with our specific pins
  spi.begin(LORA_SCK_PIN, LORA_MISO_PIN, LORA_MOSI_PIN, LORA_NSS_PIN);

  // TODO: Update this configuration
  // When setting up the radio, you can configure the parameters below:
  // carrier frequency:           TODO: pick frequency based on your channel
  // bandwidth:                   TODO: pick bandwidth based on your channel
  // spreading factor:            TODO: pick a SF based on your data rate
  // coding rate:                 5 (CR 4/5 for LoRaWAN)
  // sync word:                   0x34 (LoRaWAN sync word)
  // output power:                0 dBm
  // preamble length:             8 symbols (LoRaWAN preamble length)
  Serial.print("(Gateway) Initializing radio...");
  int16_t state = radio.begin(JN_INIT_FREQ, JN_LORA_BW, JN_LORA_SPREAD, 5, 0x34, 0, 8);
  if (state != RADIOLIB_ERR_NONE) {
      error_message("Radio initializion failed", state);
  }

  // Current limit of 140 mA (max)
  state = radio.setCurrentLimit(140.0);
  if (state != RADIOLIB_ERR_NONE) {
      error_message("Current limit intialization failed", state);
  }

  // Hardware uses DIO2 on the SX1262 as an RF switch
  state = radio.setDio2AsRfSwitch(true);
  if (state != RADIOLIB_ERR_NONE) {
      error_message("DIO2 as RF switch intialization failed", state);
  }

  // LoRa explicit header mode is used for LoRaWAN
  state = radio.explicitHeader();
  if (state != RADIOLIB_ERR_NONE) {
      error_message("Explicit header intialization failed", state);
  }

  // LoRaWAN uses a two-byte CRC
  state = radio.setCRC(2);
  if (state != RADIOLIB_ERR_NONE) {
      error_message("CRC intialization failed", state);
  }
  Serial.println("Complete!");

  // ========== List boot info
  if(IS_GATEWAY) {
    Serial.print("### Starting Bluemolar gateway, listening on uplink channel\n");
    tune_to_rx();
  } else {
    Serial.print("### Starting Bluemolar end device, listening on downlink channel\n");
    tune_to_rx();
  }

  // set the function that will be called when a new packet is received
  radio.setDio1Action(receiveISR);

  // start continuous reception
  Serial.print("Beginning continuous reception...");
  state = radio.startReceive();
  if (state != RADIOLIB_ERR_NONE) {
    error_message("Starting reception failed", state);
  }
  Serial.println("Complete!");
}

void loop() {

  // Handle packet receptions
  if (receivedFlag) {
    receivedFlag = false;

    // you can receive data as an Arduino String
    String packet_data;
    int state = radio.readData(packet_data);

    if (state == RADIOLIB_ERR_NONE) {
      // packet was successfully received
      Serial.println("Received packet!");

      // print the data of the packet
      Serial.print("[SX1262] Data:  ");
      Serial.println(packet_data);
      Serial.print("\t[");
      const char* data = packet_data.c_str();
      last_packet = packet_data;
      //data = packet_data.c_str();
      for (int i = 0; i < packet_data.length(); i++) {
        Serial.printf("%02X ", data[i]);
      }
      Serial.println("]");

      // print the RSSI (Received Signal Strength Indicator)
      // of the last received packet
      Serial.print("\tRSSI:\t\t");
      Serial.print(radio.getRSSI());
      Serial.println(" dBm");

      
      // Parse Origin -> Dest Values
      char FromTo[50];
      int from, to;
      from = (data[0]==0x31)?1:2;
      to = (data[1]==0x31)?1:2;
      //sprintf(FromTo, "Msg from End-device %d to %d", from, to);
      Serial.println(FromTo);
      Serial.printf("Msg from End-device %d to %d\n", from, to);
      repeatFlag = true;
 
    } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
      // timeout occurred while waiting for a packet
      Serial.println("timeout!");
    } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
      // packet was received, but is malformed
      Serial.println("CRC error!");
    } else {
      // some other error occurred
      Serial.print("failed, code ");
      Serial.println(state);
    }

    // resume listening
    state = radio.startReceive();
    if (state != RADIOLIB_ERR_NONE) {
      error_message("Resuming reception failed", state);
    }
  }

  // Handle button presses
  if (buttonFlag) {
    buttonFlag = false;

    // transmit packet
    Serial.print("Button pressed! Getting ready to TX\n");
    tune_to_tx();

    int16_t state = radio.transmit("CSE122/CSE222C/WES269 - Hello this is GATEWAY");
    if (state == RADIOLIB_ERR_NONE) {
      Serial.println("TX Complete!");
    } else if (state == RADIOLIB_ERR_PACKET_TOO_LONG) {
      // packet was longer than max size
      Serial.println("Packet too long to transmit");
    } else if (state == RADIOLIB_ERR_TX_TIMEOUT) {
      // timeout occurred while transmitting packet
      Serial.println("TX timeout occurred?");
    } else {
      // Some other error occurred
      Serial.printf("Error while transmitting! Error code: %d\n", state);
    }

    // transmitting drops us out of receiving mode as if we received a packet
    // reset the receivedFlag status and resume receiving
    receivedFlag = false;
    tune_to_rx();
    state = radio.startReceive();
    if (state != RADIOLIB_ERR_NONE) {
      error_message("Resuming reception failed", state);
    }
  }

  // Handle repeat message to destination end device
  if (repeatFlag) {
    repeatFlag = false;
        /// Repeat message to reach destination End_Device
        Serial.print("Gateway to repeat the message\n");      
        tune_to_tx();
        int16_t state = radio.transmit("Repeated by GATEWAY:");
        state = radio.transmit(last_packet);
        if (state == RADIOLIB_ERR_NONE) {
          Serial.println("TX Complete!");
        } else if (state == RADIOLIB_ERR_PACKET_TOO_LONG) {
          // packet was longer than max size
          Serial.println("Packet too long to transmit");
        } else if (state == RADIOLIB_ERR_TX_TIMEOUT) {
          // timeout occurred while transmitting packet
          Serial.println("TX timeout occurred?");
        } else {
          // Some other error occurred
          Serial.printf("Error while transmitting! Error code: %d\n", state);
        }

        // transmitting drops us out of receiving mode as if we received a packet
        // reset the receivedFlag status and resume receiving
        receivedFlag = false;
        tune_to_rx();
  }

  // If you want some actions to happen with a time delay, use this
  static unsigned long next_time = millis();
  if (millis() > next_time) {
    next_time += 15000;
    //advance_channel();

    // periodic actions here
  }
}

