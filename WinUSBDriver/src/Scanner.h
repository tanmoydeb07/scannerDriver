#pragma once

#include "USBInterface.h"



enum ScanColor
{
	COLOR_GRAY = 0,
	COLOR_GRAY_16_BIT,
	COLOR_RGB,
	COLOR_RGB_16_BIT,
};

enum ScanDPI
{
	DPI_600 = 600,
	DPI_300 = 300,
	DPI_150 = 150,
	DPI_75  = 75
};

struct ScanInfo
{
	uint16_t startX;
	uint16_t startY;
	uint16_t width;
	uint16_t height;
	ScanDPI DPI;
	ScanColor color;
};


/*class RawImage
{
	int _width;
	int _height;
	ScanColor _color;
	std::vector<uint8_t> _data;

public:
	RawImage(int width, int height, ScanColor color):
		_width(width), _height(height), _color(color)
	{
		_data.resize(_width * _height * bytesPerPixel());
	}

	int bytesPerPixel()
	{
		switch(_color)
		{
		case COLOR_GREY:
			return 1;
		case COLOR_RGB:
			return 3;
		case COLOR_48_BIT:
			return 6;
		};
	}

	int width()
		{ return _width; }

	int height()
		{ return _height; }

	uint8_t* data()
		{ return &_data[0]; }
};
*/

struct ControlChannel;

class Scanner
{
	USBInterface _usb;

	static const int RCV_BUFF_SIZE = 64;
	uint8_t _rcvBuffer[RCV_BUFF_SIZE];

	//static const int MAX_ROW_SIZE = 5120;
	static const int MAX_ROW_SIZE = 8192;
	uint8_t _rowBuffers[2][MAX_ROW_SIZE * 3 * 2]; // for 16-bit RGB channels

	uint8_t *_oldRow, *_newRow;

	uint8_t _rgbRow[MAX_ROW_SIZE * 2 * 3];

	// current scan information
	bool _scanning;
	long _bytesReady;
	uint8_t *_bytesPtr;
	ScanInfo _currentScanInfo;

	void swapBuffers() 
		{ std::swap(_oldRow, _newRow); }

	int sendRcvMsg(	ControlChannel chan,
					const uint8_t* sendBuff,	uint16_t sendSize,
					uint8_t* rcvBuff,	uint16_t rcvSize,
					bool silent = false);

	int sendFirmware();

	
	int sendMsg73(ControlChannel chan); // basic info query?
	int sendMsg70(ControlChannel chan); // check firmware instaled ?
	int sendMsg69(ControlChannel chan); // install firmware
	int sendMsg2e(ControlChannel chan); // ????
	int sendMsg74(ControlChannel chan); // button polling
	int sendMsg17(ControlChannel chan); // query?
	int sendMsg3f(ControlChannel chan); // query?
	int sendMsg12(ControlChannel chan); // query? 

	int sendMsg22(ControlChannel chan); // send intensity parameters
	int sendMsg22color(ControlChannel chan, uint16_t color);
	int sendMsg22param(ControlChannel chan,
							uint8_t r1, uint8_t r2, 
							uint8_t g1, uint8_t g2,
							uint8_t b1, uint8_t b2);

	int sendMsg20info(ControlChannel chan, ScanInfo info); // send scanning information

	int sendMsg43(ControlChannel chan); // start scanning
	int sendMsg35(ControlChannel chan, bool silent = false); // polling till data ready
	int sendMsg41(ControlChannel chan); // stop scanning

	////////////////////

	int rcvRowData(uint8_t* rcvBuff, unsigned long buffLen, unsigned long& accLen);
	AsyncIOHandle rcvRowDataAsync(uint8_t* buffer, unsigned long bufferSize);

	AsyncIOHandle _rowOper;
	void rcvRowAsync();

	void processRow();

	

public:
	static const uint16_t vendorID = 0x07b3;
	static const uint16_t productID = 0x0413;

	Scanner(HANDLE subDevHandle);
	~Scanner();

	int init();
	
	int initScan(ScanInfo scan);
	int initScan2(ScanInfo scan, uint8_t v1, uint8_t v2);
	int rcvData(long length ,uint8_t* buffer, long* outRealLen);
	int finishScan();
};