#include "stdafx.h"
#include "USBInterface.h"

#include <iostream>

using namespace std;

AsyncIOHandle::AsyncIOHandle()
{
	_overlapped = shared_ptr<OVERLAPPED>(new OVERLAPPED, 
		[](OVERLAPPED* p){
			if(p->hEvent){ ::CloseHandle(p->hEvent); }
		});

	_overlapped->hEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
}


bool AsyncIOHandle::isCompleted()
{
	return HasOverlappedIoCompleted(_overlapped.get());
}

void AsyncIOHandle::await()
{
	if(! isCompleted()){
		::WaitForSingleObject(_overlapped->hEvent, INFINITE);
	}
}

bool AsyncIOHandle::awaitTimeout(long timeoutMs)
{
	if(! isCompleted()){
		DWORD r = ::WaitForSingleObject(_overlapped->hEvent, timeoutMs);
		return r == WAIT_OBJECT_0;
	}
	return true;
}

//-------------------------------------------------------------

USBInterface::USBInterface(HANDLE deviceHandle):
	_winUsbHandle(NULL)
{
	if(! WinUsb_Initialize(deviceHandle, &_winUsbHandle) )
		{ throw  std::system_error( GetLastError(), std::system_category(), "WinUsb_Initialize Error"); }
}

USBInterface::~USBInterface()
{
	WinUsb_Free(_winUsbHandle);
}

int USBInterface::controlTransfer(
	uint8_t request_type, uint8_t bRequest,
	uint16_t wValue, uint16_t wIndex,
	uint8_t* data, uint16_t wLength)
{

	WINUSB_SETUP_PACKET setupPacket;
	setupPacket.RequestType = request_type;
	setupPacket.Request = bRequest;
	setupPacket.Value = wValue;
	setupPacket.Index = wIndex;
	setupPacket.Length = wLength;

	ULONG transferred = 0;
	BOOL res = WinUsb_ControlTransfer(
		_winUsbHandle,
		setupPacket,
		data,
		wLength,
		&transferred,
		NULL);

	if(res == FALSE)
	{
		DWORD err = GetLastError();
		std::cerr << "Control transfer error" << getErrorCodeMsg(err);
	}

	return (res == FALSE) ? -1 : 0;
}

int USBInterface::bulkTransfer(
	unsigned char endpoint, unsigned char *data,
	unsigned long length, unsigned long *actual_length)
{
	bool read = endpoint & (1 << 7);

	BOOL res = TRUE;
	if(read)
	{
		res = WinUsb_ReadPipe(
			_winUsbHandle,
			endpoint,
			data,
			length,
			actual_length,
			NULL);
	}
	else
	{
		res = WinUsb_WritePipe(
			_winUsbHandle,
			endpoint,
			data,
			length,
			actual_length,
			NULL);
	}

	return (res == FALSE)? -1 : 0;
}


AsyncIOHandle USBInterface::bulkTransferAsync(
	unsigned char endpoint, unsigned char *data,
	unsigned long length, unsigned long *actual_length)
{
	AsyncIOHandle asyncHandle;

	bool read = endpoint & (1 << 7);

	BOOL res = TRUE;
	if(read)
	{
		res = WinUsb_ReadPipe(
			_winUsbHandle,
			endpoint,
			data,
			length,
			actual_length,
			asyncHandle.getOerlapped());
	}
	else
	{
		res = WinUsb_WritePipe(
			_winUsbHandle,
			endpoint,
			data,
			length,
			actual_length,
			asyncHandle.getOerlapped());
	}

	//return (res == FALSE)? -1 : 0;	
	return asyncHandle;
}