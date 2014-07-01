#include <iostream>
#include <string>
#include <libusb.h>
#include <functional>
#include <cstdio>
#include <iomanip>
#include <sstream>
#include "Scanner.h"
#include <EasyBMP.h>

#include <fstream>

using namespace std;

class onScopeExit
{
	function<void(void)> _func;
public:
	onScopeExit(function<void(void)> func): _func(func) {}
	~onScopeExit() { _func(); }
};

int main()
{
	int r = libusb_init(NULL);
	if (r < 0) { return r; }
	onScopeExit ex1( []{libusb_exit(NULL);} );

	libusb_set_debug(NULL,3);

/*	libusb_device** deviceArray = NULL;
	int listSize = libusb_get_device_list(NULL,&deviceArray);
	for(int i = 0; i < listSize; ++i)
	{
		libusb_device* dev = deviceArray[i];
		libusb_device_descriptor desc;
		libusb_config_descriptor* cDesc = NULL;
		libusb_get_device_descriptor(dev,&desc);
		int r = libusb_get_config_descriptor(dev,1,&cDesc);
		cout << hex << desc.idVendor << " " << hex << desc.idProduct<< endl; 
		libusb_free_config_descriptor(cDesc);
	}
	libusb_free_device_list(deviceArray,true);*/

	libusb_device_handle* handle = NULL;
	handle = libusb_open_device_with_vid_pid(NULL,Scanner::vendorID,Scanner::productID);
	if (handle == NULL) 
	{
		cerr << "No scanner found." << endl;
		return 1;
	}	
	onScopeExit ex2( [handle]{libusb_close(handle);	} );


	// Configuration
	int conf;
	r = libusb_get_configuration(handle, &conf);
	if(r < 0)
	{
		cerr << "Error getting configuration: " << libusb_error_name(r) << endl;
		return 1;
	}

	cout << "Config: " << conf << endl;

	r = libusb_claim_interface(handle,0);
	if(r < 0)
	{
		cerr << "Error claiming interface: " << libusb_error_name(r) << endl;
		return 1;
	}
	onScopeExit ex3( [handle] { libusb_release_interface(handle,0);} );

	// Descriptor
	unsigned char descriptor[257];
	descriptor[256] = '\0';
	r = libusb_get_string_descriptor_ascii(handle,1,descriptor,256);
	if(r < 0)
	{
		cerr << "Error getting descriptor ascii: " << libusb_error_name(r) << endl;
		return 1;
	}
	cout << descriptor << endl;


	libusb_config_descriptor* pDesc = NULL;
	r = libusb_get_active_config_descriptor(libusb_get_device(handle),&pDesc);
	if(r < 0)
	{
		cerr << "Error getting descriptor: " << libusb_error_name(r) << endl;
		return 1;
	}

	libusb_free_config_descriptor(pDesc);

	r = libusb_set_configuration(handle,1);
	if(r < 0)
	{
		cerr << " Error setting configuration: " << libusb_error_name(r) << endl;
		return 1;
	}

	
	Scanner scanner(handle);
	if(scanner.init() < 0) {return 1;}

///////////////////////////////////////////////////////////////

	ScanInfo scan;
	scan.color = COLOR_TEST;
	scan.DPI = 600;
	scan.startY = 0x00b4;
	//scan.height = 0x1b68;
	scan.height = 512;
	scan.startX = 0x0018;
	scan.width  = 0x1400;

	scan.DPI = 150;
	/*for(int i = 0; i < 64; ++i)
	{
		uint8_t v = i;
		ostringstream ostr;
	//	ostr << setfill('0') << hex;
		ostr << "green/out_0x17_" <<  int(v) << ".bmp"; 
		if( scanner.scanBMP(scan,ostr.str().c_str(),0x17,v ) < 0) { return 1; } 
	}*/
	scan.DPI = 300;
	if( scanner.scanBMP(scan,"out300.bmp",0x17,0x10 ) < 0) {
		return 1;
	} 
	/*scan.DPI = 150;
	if( scanner.scanBMP(scan,"out150.bmp",0x17,0x10 ) < 0) {
		return 1;
	} */
	/*scan.DPI = 75;
	if( scanner.scanBMP(scan,"out75.bmp",0x17,0x10 ) < 0) {
		return 1;
	} */

	return 0;
}