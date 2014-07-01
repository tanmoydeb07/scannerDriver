// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include <windows.h>
#include <cstdio>
#include <tchar.h>

// Include WinUSB headers
#include <winusb.h>
#include <Setupapi.h>

#include <vector>
#include <iostream>
#include <string>
#include <exception>
#include <functional>
#include <stdint.h>

//TMP
#include <fstream>
#include <system_error>

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