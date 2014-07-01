#pragma once

class AsyncIOHandle
{
	std::shared_ptr<OVERLAPPED> _overlapped;

public:
	AsyncIOHandle();	
	~AsyncIOHandle() {};

	bool isCompleted();

	void await();
	bool awaitTimeout(long timeoutMs);

	OVERLAPPED* getOerlapped() { return _overlapped.get(); }
};

/////////////////////////////////////////////////////

class USBInterface
{
	WINUSB_INTERFACE_HANDLE _winUsbHandle;
public:
	USBInterface(HANDLE deviceHandle);
	~USBInterface();

	int controlTransfer(
		uint8_t request_type, uint8_t bRequest,
		uint16_t wValue, uint16_t wIndex,
		uint8_t* data, uint16_t wLength);

	int bulkTransfer(
		unsigned char endpoint, unsigned char *data,
		unsigned long length, unsigned long *actual_length);

	
	AsyncIOHandle bulkTransferAsync(
		unsigned char endpoint, unsigned char *data,
		unsigned long length, unsigned long *actual_length);
};