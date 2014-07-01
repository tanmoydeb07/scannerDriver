#pragma once
#include <libusb.h>

#include <vector>

class BMP;
struct ControllChannel;

struct ScanInfo
{
	uint16_t startX;
	uint16_t startY;
	uint16_t width;
	uint16_t height;
	uint16_t DPI;
	uint16_t color;
};

const uint16_t COLOR_RGB		 = 0x1084;
const uint16_t COLOR_GREY		 = 0x1082;
const uint16_t COLOR_48_BIT		 = 0x1094;
const uint16_t COLOR_TEST		 = 0x1084;

class Scanner
{
	libusb_device_handle* _handle;

	static const long RCV_BUFF_SIZE = 64;
	uint8_t _rcvBuffer[RCV_BUFF_SIZE];

	static const long ROW_SIZE = 5120;
	libusb_transfer* _rowTransfer;
	std::vector<uint8_t> _rowBuffer[2];
	uint8_t* _newBuffer;
	uint8_t* _oldBuffer;

	int sendRcvMsg(	ControllChannel chan,
					const uint8_t* sendBuff,	uint16_t sendSize,
					uint8_t* rcvBuff,	uint16_t rcvSize,
					bool silent = false);

	int sendFirmware();

	
	int sendMsg73(ControllChannel chan); // basic info query?
	int sendMsg70(ControllChannel chan); // check firmware instaled ?
	int sendMsg69(ControllChannel chan); // install firmware
	int sendMsg2e(ControllChannel chan); // ????
	int sendMsg74(ControllChannel chan); // button polling
	int sendMsg17(ControllChannel chan); // query?
	int sendMsg3f(ControllChannel chan); // query?
	int sendMsg12(ControllChannel chan); // query? 
	int sendMsg22(ControllChannel chan); // send intensity parameters
	int sendMsg22color(ControllChannel chan, uint16_t color);
	int sendMsg22param(ControllChannel chan,
							uint8_t r1, uint8_t r2, 
							uint8_t g1, uint8_t g2,
							uint8_t b1, uint8_t b2);
	int sendMsg20info(ControllChannel chan, ScanInfo info); // send scanning information
	int sendMsg43(ControllChannel chan); // start scanning
	int sendMsg35(ControllChannel chan, bool silent = false); // polling till data ready
	int sendMsg41(ControllChannel chan); // stop scanning

	int rcvDataBMP(BMP& image, int row, uint16_t DPI , uint16_t color);
	int rcvRow(uint8_t* rcvBuff, int buffLen, int& accLen);

	int initiateRowTransfer(uint8_t* buffer, long bufferSize);
	void processRow(uint8_t* buffer, long bufferSize);
	void swapBuffers()
	{
		uint8_t* tmp = _newBuffer;
		_newBuffer = _oldBuffer;
		_oldBuffer = tmp;
	}
	static void LIBUSB_CALL rowReadCallback(libusb_transfer * transfer);

public:
	static const uint16_t vendorID = 0x07b3;
	static const uint16_t productID = 0x0413;

	Scanner(libusb_device_handle* handle);
	~Scanner();

	int init();
	int scanBMP(ScanInfo scan, const char* filename, uint8_t p1, uint8_t p2);
};