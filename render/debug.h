#pragma once

#include <Windows.h>
#include <stdio.h>

void _TRACE(TCHAR *pszFormat, ...)
{
	if (lstrlen(pszFormat) > 512)
	{
		return;
	}

	TCHAR buffer[1024] = { 0 };
	va_list argptr;
	va_start(argptr, pszFormat);
	wvsprintf(buffer, pszFormat, argptr);
	va_end(argptr);
	OutputDebugString(buffer);
}

void _TRACEA(const char *pszFormat, ...)
{
	if (strlen(pszFormat) > 512)
	{
		return;
	}

	char buffer[1024] = { 0 };
	va_list argptr;
	va_start(argptr, pszFormat);
	vsprintf_s(buffer, pszFormat, argptr);
	va_end(argptr);
	OutputDebugStringA(buffer);
}

#ifdef _DEBUG
#define TRACE _TRACE
#define TRACEA _TRACEA
#else
#define TRACE
#define TRACEA
#endif
