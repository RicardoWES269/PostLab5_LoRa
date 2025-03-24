#include "jamnet.h"

void parse_packet(uint8_t length, uint8_t * data){
    uint8_t JN_PacketType = data[0];

    switch (JN_PacketType)
    {
    case PACKET_SCAN:
        if (length < sizeof(SCAN)) {
            printf("Invalid SCAN packet\n");
            return;
        }
        SCAN *scan_packet = (SCAN *)(data + 1);
        printf("SCAN Packet - Uplink Channel: %d\n", scan_packet->uplink_channel);
        break;

    case PACKET_SCAN_RESPONSE:
        if (length < sizeof(SCAN_RESPONSE)) {
            printf("Invalid SCAN_RESPONSE packet\n");
            return;
        }
        SCAN_RESPONSE *scan_response_packet = (SCAN_RESPONSE *)(data + 1);
        printf("SCAN Response - Uplink Channel: %d - Downlink Channel: %d - Network Name: %s\n", scan_response_packet->uplink_channel, scan_response_packet->downlink_channel, scan_response_packet->network_name);
        break;

    case PACKET_JOIN_REQUEST:
        if (length < sizeof(JOIN_REQUEST)) {
            printf("Invalid JOIN_REQUEST packet\n");
            return;
        }
        JOIN_REQUEST *join_request_packet = (JOIN_REQUEST *)(data + 1);
        printf("Join Request - Source Address: %d\n", join_request_packet->source_address);
        break;

    case PACKET_JOIN_RESPONSE:
        if (length < sizeof(JOIN_RESPONSE)) {
            printf("Invalid JOIN_RESPONSE packet\n");
            return;
        }
        JOIN_RESPONSE *join_response_packet = (JOIN_RESPONSE *)(data + 1);
        printf("JOIN_RESPONSE - Resp Code: %d - Target Address: %d - Source Address: %d\n", join_response_packet->resp, join_response_packet->target_address, join_response_packet->source_address);
        break;

    case PACKET_DATA_UPLINK:
        if (length < sizeof(DATA_UPLINK)) {
            printf("Invalid DATA_UPLINK packet\n");
            return;
        }
        DATA_UPLINK *data_uplink_packet = (DATA_UPLINK *)(data + 1);
        printf("DATA_UPLINK - Length: %d - Target Address: %d - Source Address: %d - JN App: %d - Data: %s\n", data_uplink_packet->packet_length, data_uplink_packet->target_address, data_uplink_packet->source_address, data_uplink_packet->app, data_uplink_packet->data);
        break;

    case PACKET_ACK:
        if (length < sizeof(ACK)) {
            printf("Invalid ACK packet\n");
            return;
        }
        ACK *ack_packet = (ACK *)(data + 1);
        printf("ACK - Resp Code: %d - Packet Queue: %d - Target Address: %d\n", ack_packet->resp, ack_packet->packet_queue, ack_packet->target_address);
        break;

    case PACKET_DATA_REQUEST:
        if (length < sizeof(DATA_REQUEST)) {
            printf("Invalid DATA_REQUEST packet\n");
            return;
        }
        DATA_REQUEST *data_request_packet = (DATA_REQUEST *)(data + 1);
        printf("DATA_REQUEST - Source Address: %d\n", data_request_packet->source_address);
        break;

    case PACKET_DATA_RESPONSE:
        if (length < sizeof(DATA_RESPONSE)) {
            printf("Invalid DATA_RESPONSE packet\n");
            return;
        }
        DATA_RESPONSE *data_response_packet = (DATA_RESPONSE *)(data + 1);
        printf("DATA_RESPONSE - Resp: %d - Packet Queue: %d - Packet Length: %d - App: %d - Data: %s\n", data_response_packet->resp, data_response_packet->packet_queue, data_response_packet->packet_length, data_response_packet->app, data_response_packet->data);
        break;

    default:
        printf("Unknown Packet Type: 0x%02X\n", JN_PacketType);
        break;
    }
}