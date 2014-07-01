// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

// Include WinUSB headers
#include <winusb.h>
#include <Setupapi.h>

//#include "mywia.h"
#include <stdint.h>

#include <cstdio>
#include <tchar.h>



#include <functional>
#include <system_error>
#include <vector>
#include <string>
#include <cassert>
#include <memory>


///////////////////////////////////////////////////////

class ScopeGuard
{
	std::function<void()> _func;
public:
	ScopeGuard(const std::function<void()>& func):
		_func(func) {}
	~ScopeGuard()
		{ _func(); }
	void drop()
		{ _func = std::function<void()>(); }
};

inline std::string getErrorCodeMsg(DWORD errorCode)
{
	char* buffer = NULL;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		errorCode,
		0,
		(LPTSTR) &buffer,
		0, NULL );

	std::string msg(buffer);
	LocalFree(buffer);

	return msg;
}