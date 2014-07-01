// WIAMicrodriver.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

extern "C" {
#include <wiamicro.h>
}

#include "Scanner.h"

namespace ScannerDriver
{

	inline HRESULT init(PVAL pValue)
	{
		ZeroMemory(pValue->pScanInfo, sizeof(SCANINFO));

		SCANINFO& info = *(pValue->pScanInfo);
		// global scanner specs
		info.OpticalXResolution = 600;
		info.OpticalYResolution = 600;

		// thousands of an inch
		info.BedWidth = 8530;
		info.BedHeight = 12000;

		info.IntensityRange.lMin = 0;
		info.IntensityRange.lMax = 100;
		info.IntensityRange.lStep = 1;

		info.ContrastRange.lMin = 0;
		info.ContrastRange.lMax = 100;
		info.ContrastRange.lStep = 1;

		info.SupportedDataTypes = SUPPORT_GRAYSCALE | SUPPORT_COLOR;

		info.RawDataFormat = WIA_PACKED_PIXEL;
		info.RawPixelOrder = WIA_ORDER_RGB;
		info.bNeedDataAlignment = TRUE;

		// current scanner settings
		info.Intensity = 50;
		info.Contrast = 50;
		info.Xresolution = 600;
		info.Yresolution = 600;

		info.Window.xPos = 0;
		info.Window.yPos = 0;
		info.Window.xExtent = (info.Xresolution * info.BedWidth) / 1000;
		info.Window.yExtent = (info.Yresolution * info.BedHeight) / 1000;

		// img settings

		info.DataType = WIA_DATA_GRAYSCALE;
		switch(info.DataType)
		{
		case WIA_DATA_THRESHOLD:
			info.PixelBits = 1; break;
		case WIA_DATA_GRAYSCALE:
			info.PixelBits = 8; break;
		case WIA_DATA_COLOR:
			info.PixelBits = 24; break;
		};

		info.WidthPixels = info.Window.xExtent - info.Window.xPos;
		info.WidthBytes = (info.WidthPixels * info.PixelBits) / 8;
		info.Lines = info.Window.yExtent;

		// use info.DeviceIOHandles[0] to communicate with the device

		info.pMicroDriverContext = new Scanner(info.DeviceIOHandles[0]);

		return S_OK;
	}

	inline HRESULT unInit(PVAL pValue)
	{
		Scanner* scanner = static_cast<Scanner*>(pValue->pScanInfo->pMicroDriverContext);
		pValue->pScanInfo->pMicroDriverContext = NULL;
		delete scanner;

		return S_OK;
	}

	inline HRESULT getCapabilities(PVAL pValue)
	{
		// No buttons
		pValue->lVal = 0;
		pValue->pGuid = NULL;
		pValue->ppButtonNames = NULL;
		return S_OK;
	}

	inline HRESULT resetScanner(PVAL pValue)
	{
		// unimplamented
		return S_OK;
	}

	inline HRESULT setDataType(PVAL pValue)
	{
		pValue->pScanInfo->DataType = pValue->lVal;
		return S_OK;
	}

	inline HRESULT setContrast(PVAL pValue)
	{
		long val = pValue->lVal;
		auto range = pValue->pScanInfo->ContrastRange;
		val = min( max(val, range.lMin), range.lMax );
		pValue->pScanInfo->Contrast = val;

		return S_OK;
	}

	inline HRESULT setIntensity(PVAL pValue)
	{
		pValue->pScanInfo->Intensity = pValue->lVal;
		return S_OK;
	}

	inline HRESULT setXResolution(PVAL pValue)
	{
		long val = pValue->lVal;


		pValue->pScanInfo->Xresolution = pValue->lVal;
		return S_OK;
	}

	inline HRESULT setYResolution(PVAL pValue)
	{
		pValue->pScanInfo->Yresolution = pValue->lVal;
		return S_OK;
	}

	inline HRESULT STIDeviceReset(PVAL pValue)
	{
		return S_OK;
	}

	inline HRESULT STIDiagnostic(PVAL pValue)
	{
		return S_OK;
	}

//-----------------------------------------------------

	inline HRESULT scanFirst(PSCANINFO pScanInfo, PBYTE pBuffer, LONG lLength ,LONG *plReceived)
	{
		Scanner* scanner = static_cast<Scanner*>(pScanInfo->pMicroDriverContext);

		ScanInfo info;
		if( scanner->initScan(info) < 0 ) { return E_FAIL; }
		if( scanner->rcvData(lLength, pBuffer, plReceived) < 0 ) { return E_FAIL; };

		return S_OK;
	}

	inline HRESULT scanNext(PSCANINFO pScanInfo, PBYTE pBuffer, LONG lLength, LONG *plReceived)
	{
		Scanner* scanner = static_cast<Scanner*>(pScanInfo->pMicroDriverContext);
		if( scanner->rcvData(lLength, pBuffer, plReceived) < 0 ) { return E_FAIL; };
		return S_OK;
	}

	inline HRESULT scanFinish(PSCANINFO pScanInfo)
	{
		Scanner* scanner = static_cast<Scanner*>(pScanInfo->pMicroDriverContext);
		if(scanner->finishScan() < 0) { return E_FAIL; }
		return S_OK;
	}

}

//------------------------------------------------------------------------------------

extern "C" WIAMICRO_API HRESULT MicroEntry(
	LONG lCommand,
	_Inout_  PVAL pValue)
{
	switch(lCommand)
	{
	case CMD_INITIALIZE:		return ScannerDriver::init(pValue);
	case CMD_UNINITIALIZE:		return ScannerDriver::unInit(pValue);		
	case CMD_GETCAPABILITIES:	return ScannerDriver::getCapabilities(pValue);
	case CMD_RESETSCANNER:		return ScannerDriver::resetScanner(pValue);
	case CMD_SETDATATYPE:		return ScannerDriver::setDataType(pValue);
	case CMD_SETCONTRAST:		return ScannerDriver::setContrast(pValue);
	case CMD_SETINTENSITY:		return ScannerDriver::setIntensity(pValue);
	case CMD_SETXRESOLUTION:	return ScannerDriver::setXResolution(pValue);
	case CMD_SETYRESOLUTION:	return ScannerDriver::setYResolution(pValue);
	case CMD_STI_DEVICERESET:	return ScannerDriver::STIDeviceReset(pValue);
	case CMD_STI_DIAGNOSTIC:	return ScannerDriver::STIDiagnostic(pValue);
	};

	return E_NOTIMPL;
}

extern "C" WIAMICRO_API HRESULT Scan(
	__inout PSCANINFO pScanInfo, 
	LONG lPhase, 
	__out_bcount(lLength) PBYTE pBuffer, 
	LONG lLength, 
	__out LONG *plReceived)
{
	switch(lPhase)
	{
	case SCAN_FIRST:	return ScannerDriver::scanFirst(pScanInfo, pBuffer, lLength, plReceived);
	case SCAN_NEXT:		return ScannerDriver::scanNext(pScanInfo, pBuffer, lLength, plReceived);
	case SCAN_FINISHED:	return ScannerDriver::scanFinish(pScanInfo);
	};

	return E_FAIL;
}

extern "C" WIAMICRO_API HRESULT SetPixelWindow(
	__inout PSCANINFO pScanInfo,
	LONG x, LONG y, 
	LONG xExtent, LONG yExtent)
{
	// TODO

	pScanInfo->Window.xPos = x;
	pScanInfo->Window.yPos = y;
	pScanInfo->Window.xExtent = xExtent;
	pScanInfo->Window.yExtent = yExtent;

	return S_OK;
}




BOOL APIENTRY DllMain( HMODULE hModule,
					   DWORD  ul_reason_for_call,
					   LPVOID lpReserved
					 )
{
	return TRUE;
}