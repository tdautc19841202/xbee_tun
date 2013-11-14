#ifndef XBEE_PRO_868_H
#define XBEE_PRO_868_H

#include <stdint.h>
#include <termios.h>
#include <unistd.h>


const uint8_t		xbee_frametype_at_cmd 				= 0x08;
const uint8_t		xbee_frametype_at_cmd_queue 			= 0x09;
const uint8_t		xbee_frametype_tx_req				= 0x10;
const uint8_t		xbee_frametype_explicit_addressing_cmd		= 0x11;
const uint8_t		xbee_frametype_remote_cmd_req			= 0x17;
const uint8_t		xbee_frametype_at_cmd_response			= 0x88;
const uint8_t		xbee_frametype_modem_status			= 0x8a;
const uint8_t		xbee_frametype_tx_status			= 0x8b;
const uint8_t		xbee_frametype_rx_packet			= 0x90;
const uint8_t		xbee_frametype_explicit_rx_indicator		= 0x91;
const uint8_t		xbee_frametype_node_id_indicator		= 0x95;
const uint8_t		xbee_frametype_remote_cmd_response		= 0x97;

const uint8_t		tx_req_option_disable_ack	= 0x01;
const uint8_t		tx_req_option_duty_purge	= 0x10;

typedef uint8_t		xbee_address;
typedef uint8_t		xbee_net;


int xbee_api_open(const char * filename);
int xbee_api_close(int fd);

int xbee_api_read_frame(int fd);

void xbee_api_at_cmd(int fd,const uint8_t * buf, const uint16_t len);
void xbee_api_at_cmd_queue(int fd,const uint8_t * buf, const uint16_t len);
void xbee_api_tx_req(int fd,const uint8_t * buf, const uint16_t len);
void xbee_api_explicit_addressing_cmd(int fd,const uint8_t * buf, const uint16_t len);
void xbee_api_remote_cmd_req(int fd,const uint8_t * buf, const uint16_t len);
void xbee_api_at_cmd_response(int fd,const uint8_t * buf, const uint16_t len);
void xbee_api_modem_status(int fd,const uint8_t * buf, const uint16_t len);
void xbee_api_tx_status(int fd,const uint8_t * buf, const uint16_t len);
void xbee_api_rx_packet(int fd,const uint8_t * buf, const uint16_t len);
void xbee_api_explicit_rx_indicator(int fd,const uint8_t * buf, const uint16_t len);
void xbee_api_node_id_indicator(int fd,const uint8_t * buf, const uint16_t len);
void xbee_api_remote_cmd_response(int fd,const uint8_t * buf, const uint16_t len);


int xbee_api_send_tx_req(int fd, xbee_address dest[8], const uint8_t * data, uint16_t len);
int xbee_api_send_at_cmd(int fd, unsigned char cmd[2], uint8_t data[2]);

struct xbee_api_head
{
	uint8_t 	start_delim;
	uint16_t	length;
	uint8_t		type;
} __attribute__((__packed__));

struct xbee_api_checksum
{
	uint8_t		checksum;
} __attribute__((__packed__));

struct xbee_api_frame_tx_req
{
	uint8_t   	frame_id;       //Identifies the frame, for Transmit Status frames
	xbee_address   	dest_host[8];   //Broadcast = 0xff
	xbee_net   	dest_net[2];    //Should be 0xfffe
	uint8_t   	bcast_rad;      //Should be 0x00
	uint8_t   	tx_options;     //0x01 = Disable ACK; 0x10 = Duty purge packet
} __attribute__((__packed__));

struct xbee_api_frame_tx_response
{
	uint8_t   frame_id;       //Identifies the frame
	xbee_net   dest_net[2];    //Should be 0xfffe
	uint8_t   retries;        //Number of retries it took to send the packet
	uint8_t   status;         //0x00 = Success; 0x01 = Mac ACK fail; 0x03 = Purged
	uint8_t   overhead;       //Discovery overhead; 0x00 = none
} __attribute__((__packed__));

struct xbee_api_frame_rx
{
        xbee_address   	src_host[8];	//Source address
        xbee_net	src_net[2];	//Source network
        uint8_t		recv_opt;       //0x01 packet ACKed, 0x02 packet was broadcast
}  __attribute__((__packed__));

struct xbee_api_frame_at_cmd
{
	uint8_t		frame_id;	//To correlate with responses
	unsigned char	at_cmd[2];	//AT command
//	uint8_t		at_data[2];	//AT command data
} __attribute__((__packed__));

struct xbee_api_frame_at_cmd_response
{
	uint8_t		frame_id;
	unsigned char	at_cmd[2];
	uint8_t		at_status;
} __attribute__((__packed__));

#endif
