#if !defined(_STRUCTS_H)
#define _STRUCTS_H

#include <stdint.h>


// Response code, success or failure
enum JN_RESP_CODE {
   JN_SUCCESS = 1,
   JN_FAIL = 0
};

enum JN_APP {
    JN_BUTTON,
    JN_HELLO_WORLD
};

enum JN_PACKET_TYPE {
    PACKET_SCAN = 0x01,
    PACKET_SCAN_RESPONSE = 0x02,
    PACKET_JOIN_REQUEST = 0x03,
    PACKET_JOIN_RESPONSE = 0x04,
    PACKET_DATA_UPLINK = 0x05,
    PACKET_ACK = 0x06,
    PACKET_DATA_REQUEST = 0x07,
    PACKET_DATA_RESPONSE = 0x08
};

// Purpose: Determine if a network exists on this channel
// Response type: Scan response
// Destingation: Gateway
typedef struct {
    uint8_t  uplink_channel;
} SCAN;

// Purpose: Indicates details about the network: at least a network name, possibly channels it uses, anything else that seems important
// Response type: None
// Destingation: End Device
typedef struct {
    uint8_t uplink_channel;
    uint8_t downlink_channel;
    char network_name[16];
} SCAN_RESPONSE;

// Purpose: Request to join the network. Includes the address to refer to this device by.
// Response type: Join Response
// Destingation: Gateway
typedef struct {
    uint16_t source_address;
} JOIN_REQUEST;

// Purpose:Indicates if the join was successful. Should reject a join request if the address is already in use.
// Response type: None
// Destingation: End Device
typedef struct {
    JN_RESP_CODE resp;
    uint16_t target_address;
    uint16_t source_address;
} JOIN_RESPONSE;

// Purpose:Packet data to be stored for another End Device that has joined the network. 
//  Packet payload may be zero length with a destination of the gateway, which is used to test network connectivity or check for stored packets
// Response type: Acknowledgement
// Destingation: Gateway
typedef struct {
    uint8_t packet_length;
    uint16_t target_address;
    uint16_t source_address;
    JN_APP app;
    uint8_t data;
} DATA_UPLINK;

// Purpose: Indicates whether data has been successfully received and stored.
//  A “Negative Acknowledgement” occurs when the Gateway fails to store the data because the destination is unknown or too many packets are already stored.
//  The Acknowledgement packet also indicates whether, and possibly how many, packets are available for this particular End Device (the one that originally transmitted the Data Uplink).
// Response type: None
// Destingation: End Device
typedef struct {
    JN_RESP_CODE resp;
    uint8_t packet_queue;
    uint16_t target_address;
} ACK;

// Purpose: Request a stored packet from the Gateway for this particular End Device address.
// Response type: Data Response
// Destingation: Gateway
typedef struct {
    uint16_t source_address;
} DATA_REQUEST;

// Purpose: Contains packet data destined for this End Device address if any exists. Indicates that no data exists, if there were no stored packets.
//  The Data Response also indicates whether, and possibly how many, remaining packets are stored on the Gateway for this End Device address.
// Response type: None
// Destingation: End Device
typedef struct {
    JN_RESP_CODE resp;
    uint8_t packet_queue;
    uint8_t packet_length;
    JN_APP app;
    uint8_t data;
} DATA_RESPONSE;

// Parse the packet
void parse_packet(uint8_t length, uint8_t * data);




#endif