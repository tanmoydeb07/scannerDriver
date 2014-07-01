// WinUSBDriver.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "USBInterface.h"
#include "Scanner.h"

#define cimg_use_tiff
#include <CImg.h>

#include <sstream>
#include <bitset>

#include <queue>
#include <random>

//DeviceGUID = "{E69C86CD-66FE-43DE-85F8-800B31BC62FC}"
//DeviceClassGUID = "{78a1c341-4539-11d3-b88d-00c04fad5171}"
// sholud be:
// ClassGuid = {6bdd1fc6-810f-11d0-bec7-08002be2092f}
using namespace std;
using namespace cimg_library;

static const GUID DeviceGUID = 
	{0xE69C86CD, 0x66FE, 0x43DE , { 0x85, 0xF8, 0x80, 0x0B, 0x31, 0xBC, 0x62, 0xFC } };

static const GUID DeviceClassINFGUID = 
	{0x78a1c341, 0x4539, 0x11d3 , { 0xb8, 0x8d, 0x00, 0xc0, 0x4f, 0xad, 0x51, 0x71 } };

static const GUID DeviceClassGUID = 
	{0x6bdd1fc6, 0x810f, 0x11d0 , { 0xbe, 0xc7, 0x08, 0x00, 0x2b, 0xe2, 0x09, 0x2f } };

ostream& operator<<(ostream& out, const GUID& guid)
{
	out << hex << guid.Data1 << "-" << hex << guid.Data2 << "-";
	out << hex << guid.Data3 << "-" << hex << int(guid.Data4[0]) << hex << int(guid.Data4[1]) << "-";
	out << hex << int(guid.Data4[2]) << hex << int(guid.Data4[3]) << hex << int(guid.Data4[4]);
	out << hex << int(guid.Data4[5]) << hex << int(guid.Data4[6]) << hex << int(guid.Data4[7]);
	return out;
}



HANDLE getDeviceHandle(const GUID& guid)
{
	HDEVINFO hDeviceInfo = SetupDiGetClassDevs(
		&guid, 
		NULL,
		NULL,
		DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
	if(hDeviceInfo == INVALID_HANDLE_VALUE)
		{ throw system_error(GetLastError(), std::system_category()); }

	ScopeGuard sg1( [&](){ SetupDiDestroyDeviceInfoList(hDeviceInfo); } );

	SP_DEVINFO_DATA infoData;
	infoData.cbSize = sizeof(SP_DEVINFO_DATA);
	// TODO - lepsia chybova hlaska
	if(! SetupDiEnumDeviceInfo(hDeviceInfo, 0, &infoData))
		{ throw  std::system_error( GetLastError(), std::system_category()); }

	cout << "Found device " << infoData.ClassGuid << endl;
		
	SP_INTERFACE_DEVICE_DATA interfaceData;

	interfaceData.cbSize = sizeof(interfaceData);
	BOOL bResult = SetupDiEnumDeviceInterfaces( 
		hDeviceInfo,
		&infoData,
		&guid,
		0, 
		&interfaceData);

	//Interface data is returned in SP_DEVICE_INTERFACE_DETAIL_DATA
	//which we need to allocate, so we have to call this function twice.
	//First to get the size so that we know how much to allocate
	//Second, the actual call with the allocated buffer
		
	DWORD requiredLength = 0;

	bResult = SetupDiGetDeviceInterfaceDetail(
		hDeviceInfo,
		&interfaceData,
		NULL, 0,
		&requiredLength,
		NULL);

	PSP_INTERFACE_DEVICE_DETAIL_DATA pInterfaceDetailData = NULL;
	ScopeGuard sg2( [&](){ free(pInterfaceDetailData); });

	//Check for some other error
	if (!bResult) 
	{
		if ((ERROR_INSUFFICIENT_BUFFER==GetLastError()) && (requiredLength>0))
		{
			//we got the size, allocate buffer
			pInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA) malloc(requiredLength);
			memset(pInterfaceDetailData, 0, requiredLength);
		}
		else
			{ throw  std::system_error( GetLastError(), std::system_category()); }
	}

	//get the interface detailed data
	pInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

	//Now call it with the correct size and allocated buffer
	bResult = SetupDiGetDeviceInterfaceDetail(
			hDeviceInfo,
			&interfaceData,
			pInterfaceDetailData,
			requiredLength,
			NULL,
			&infoData);
		
	//Check for some other error
	if (!bResult) 
		{ throw  std::system_error( GetLastError(), std::system_category()); }

	//copy device path
	std::string path(pInterfaceDetailData->DevicePath);						
	cout << "Device path: " << path << endl;

	//Open the device
	HANDLE hDeviceHandle = CreateFile (
		path.c_str(),
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED,
		NULL);

	if (hDeviceHandle == INVALID_HANDLE_VALUE)
		{ throw  std::system_error( GetLastError(), std::system_category()); }

	return hDeviceHandle;
}

WINUSB_INTERFACE_HANDLE getWinUSBHandle(HANDLE hDeviceHandle)
{
	WINUSB_INTERFACE_HANDLE winUsbHandle;
	if(! WinUsb_Initialize(hDeviceHandle, &winUsbHandle) )
		{ throw  std::system_error( GetLastError(), std::system_category(), "WinUsb_Initialize Error"); }

	return winUsbHandle;
}

CImg<uint16_t> scanToImg(Scanner& scanner, uint16_t val)
{
	// image from 24 to 5184 xcoord
	//  ~170 - 175 ycoord
	ScanInfo scan;
	scan.color = COLOR_GRAY_16_BIT;
	scan.DPI = DPI_600;
	//scan.startY = 0x00b4;
	scan.startY = 180;
	//scan.height = 0x1b68;
	scan.height = 500;
	//scan.startX = 0x0018;
	scan.startX = 24;
	//scan.width  = 0x1400;
	scan.width  = 5120;

	int width  = (scan.width * scan.DPI) / 600;
	int height = (scan.height * scan.DPI) / 600;
	CImg<uint16_t> img(width,height,1,3);
	

	/*BMP img;
	img.SetBitDepth(24);
	img.SetDPI(scan.DPI,scan.DPI);
	img.SetSize(width,height);
	*/
	std::vector<uint16_t> rowBuffer;
	rowBuffer.resize(width);

	uint8_t V1 = val & 0xff;
	uint8_t V2 = val >> 8;

	if( scanner.initScan2(scan,V1,V2) < 0 ) { throw std::runtime_error("initScan failed"); }
	for(int i = 0; i < height; ++i) {
		long realLen = 0;
		if( scanner.rcvData(rowBuffer.size() * sizeof(uint16_t), (uint8_t*) &rowBuffer[0],  &realLen) < 0 )
			{ throw std::runtime_error("rcvData failed"); };
		
		for(int j = 0; j < width; ++j) {
			img(j,i,0,0) = rowBuffer[j];
			img(j,i,0,1) = rowBuffer[j];
			img(j,i,0,2) = rowBuffer[j];
		}
	}

	if(scanner.finishScan() < 0) { throw std::runtime_error("finishedScan failed"); }

	return img;
}

std::vector<int> randomIntervalSubdivision(int minVal, int maxVal, int numPts, float region, unsigned long seed = std::mt19937::default_seed)
{
	struct Interval
	{
		int a, b;

		bool operator<(const Interval& other) const
			{ return size() < other.size(); }

		int size() const { return b - a; }
	};

	mt19937 gen(seed);

	priority_queue<Interval> heap;

	Interval startInt = {minVal, maxVal};
	heap.push(startInt);

	vector<int> resPts(numPts);

	for(int i = 0; i < numPts; ++i)	{
		if(heap.empty()) { break; }

		Interval interval = heap.top();
		heap.pop();

		float center = float(interval.a + interval.b) / 2.0f;
		int iMin = ceil(  (float(interval.a) - center) * region + center);
		int iMax = floor( (float(interval.b) - center) * region + center);

		uniform_int_distribution<> dist(iMin, iMax);
		int pt = dist(gen);

		Interval int1 = {interval.a, pt};
		Interval int2 = {pt, interval.b};

		if(int1.size() > 1) { heap.push(int1); }
		if(int2.size() > 1) { heap.push(int2); }

		resPts[i] = pt;
	}

	return resPts;
}


int main(int argc, char* argv[])
{
	try	{
		HANDLE handle = getDeviceHandle(DeviceGUID);
		Scanner scanner(handle);
		if(scanner.init() < 0) {return 1;}
		
		/*const int valSize = 17;
		uint16_t values[valSize] =
		{
			0x1017,
			0x1001, 0x1002, 0x1004, 0x1008,
			0x1010, 0x1030, 0x1050, 0x1090,
			0x1110,	0x1210, 0x1410, 0x1810,
			0x1010, 0x3010, 0x5010, 0x9010
		};*/

		std::vector<int> values = randomIntervalSubdivision(0, 0x3ff, 1, 0.66f);
		std::sort(values.begin(), values.end());

		for(int i = 0; i < values.size(); ++i)
		{
			uint16_t val = values[i];
			CImg<uint16_t> img = scanToImg(scanner, val);		

			//img.display();

			ostringstream ostr;
			//ostr << "tmpImgs/" << bitset<16>(val) << ".tiff"; 
			ostr << "tmpImgs/" << val << ".tiff"; 
			//img.WriteToFile(ostr.str().c_str());
			img.save_tiff(ostr.str().c_str());
		}
	}

	catch(std::system_error& ex) {
		cerr << "Error: " << ex.code() << " - " << getErrorCodeMsg(ex.code().value());
		cerr << "Msg: " << ex.what() << endl;
	}
	catch(std::runtime_error& ex){
		cerr << "ErrorMsg: " << ex.what() << endl;
	}

	system("pause");
	return 0;
}

