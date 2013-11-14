#include "xbee_pro_868.h"

#include <string.h>
#include <unistd.h>

#include <iostream>
using namespace std;

int main(const int argc, const char ** argv)
{
	const char * file = "/dev/ttyUSB0";
	if(argc > 1)
		file = argv[1];
	int fd = xbee_api_open(file);
	xbee_address dest[8];
	dest[0] = 0x00;
	dest[1] = 0x00;	
	dest[2] = 0x00;	
	dest[3] = 0x00;	
	dest[4] = 0x00;	
	dest[5] = 0x00;	
	dest[6] = 0xff;	
	dest[7] = 0xff;	
	
	cout << "Connection to XBee open" << endl;

	//int xbee_api_send_tx_req(int fd, xbee_address dest[8], const uint8_t * data, uint16_t len)
	for(int i=0; i<250; i++)
	{
		const char * testdata = "TxData1B";
		xbee_api_send_tx_req(fd, dest, (const uint8_t *)testdata, strlen(testdata) + 1);
		cout << "Sent frame " << i << endl;
		xbee_api_read_frame(fd);
		xbee_api_read_frame(fd);
		xbee_api_read_frame(fd);
		sleep(1);
	}

	cout << "Closing XBee" << endl;
	xbee_api_close(fd);



	return 0;
}
