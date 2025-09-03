#pragma once
#include<string>
#include<winerror.h>

static bool bInitCom = false;
static bool bInitLog = false;


std::string wchar_t2uft8(const wchar_t* a_szSrc, int len = -1);
std::wstring utf82wchar_t(const char* a_szSrc, int len = -1);
std::wstring HRESULT2wchar_t(HRESULT hr);
std::string HRESULT2utf8(HRESULT hr);
void initLog();
bool initCom();
void uninitCom();

std::string GetExePath();
