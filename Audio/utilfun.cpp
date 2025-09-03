#include"utilfun.h"
#include<memory>
#include<cassert>
#include "Util/logger.h"
#include "Network/Socket.h"



std::string wchar_t2uft8(const wchar_t* a_szSrc, int len)
{
	int size = WideCharToMultiByte(CP_UTF8, 0, a_szSrc, -1, NULL, 0, NULL, NULL);
	std::unique_ptr<char[]>ptr = std::make_unique<char[]>(size);
	int szLen = WideCharToMultiByte(CP_UTF8, 0, a_szSrc, len, ptr.get(), size, NULL, NULL);
	assert(szLen > 0);
	std::string szuft8(ptr.get(), size > 0 ? size -1: size);
	return szuft8;
}

std::wstring utf82wchar_t(const char* a_szSrc, int len)
{
	int size = MultiByteToWideChar(CP_UTF8, 0, a_szSrc, -1, NULL, 0);
	std::unique_ptr<wchar_t[]>ptr = std::make_unique<wchar_t[]>(size);
	int szLen = MultiByteToWideChar(CP_UTF8, 0, a_szSrc, len, ptr.get(), size);
	assert(szLen > 0);
	std::wstring szWchar(ptr.get(), size);
	return szWchar;
}

std::wstring HRESULT2wchar_t(HRESULT hr)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER
		| FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		hr,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR)&lpMsgBuf,
		0,
		NULL
	);
	return std::wstring((wchar_t*)lpMsgBuf);
}

std::string HRESULT2utf8(HRESULT hr)
{
	std::wstring wstr = HRESULT2wchar_t(hr);
	std::string str;
	if(!wstr.empty())
		str = wchar_t2uft8(wstr.c_str(),-1);
	return str;
}


void initLog() 
{
	if (!bInitLog)
	{
		toolkit::Logger::Instance().add(std::make_shared<toolkit::ConsoleChannel>());
		toolkit::Logger::Instance().add(std::make_shared<toolkit::FileChannel>());
		toolkit::Logger::Instance().setWriter(std::make_shared<toolkit::AsyncLogWriter>());
		bInitLog = true;
	}
}

bool initCom()
{
	if (!bInitCom)
	{
		HRESULT hr = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
		if (FAILED(hr))
		{
			PrintE("CoInitializeEx error:%s", HRESULT2utf8(hr));
			return false;
		}
		bInitCom = true;
		//return true;
	}
	return true;
}

void uninitCom() 
{
	if (bInitCom)
	{
		CoUninitialize();
		bInitCom = false;
	}
}


std::string GetExePath()
{
	wchar_t szFilePath[MAX_PATH + 1] = { 0 };
	std::string strPath2 = "";
	GetModuleFileNameW(NULL, szFilePath, MAX_PATH);
	auto iter = std::find(std::rbegin(szFilePath), std::rend(szFilePath), L'\\');
	int len = lstrlenW(szFilePath);
	if (iter != std::rend(szFilePath))
	{
		len = iter.base() - std::begin(szFilePath);
	}
	std::wstring wpath(szFilePath, len);
	std::string strPath = wchar_t2uft8(wpath.c_str());// .substr(0, 24);
	return strPath;
}