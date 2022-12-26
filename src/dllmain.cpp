﻿#include "pch.h"
#include "pystring/pystring.h"
#include "EasyCEFHooks.h"
#include <Windows.h>
#include "App.h"
#include "resource.h"
#include "utils.h"


#pragma comment(linker, "/EXPORT:vSetDdrawflag=_AheadLib_vSetDdrawflag,@1")
#pragma comment(linker, "/EXPORT:AlphaBlend=_AheadLib_AlphaBlend,@2") 
#pragma comment(linker, "/EXPORT:DllInitialize=_AheadLib_DllInitialize,@3")
#pragma comment(linker, "/EXPORT:GradientFill=_AheadLib_GradientFill,@4")
#pragma comment(linker, "/EXPORT:TransparentBlt=_AheadLib_TransparentBlt,@5") 

#define EXTERNC extern "C"
#define NAKED __declspec(naked)
#define EXPORT __declspec(dllexport)

#define ALCPP EXPORT NAKED 
#define ALSTD EXTERNC EXPORT NAKED void __stdcall
#define ALCFAST EXTERNC EXPORT NAKED void __fastcall
#define ALCDECL EXTERNC NAKED void __cdecl

string script;


void message(string title, string text) {
	MessageBox(NULL, s2ws(text).c_str(), s2ws(title).c_str(), 0);
}


namespace AheadLib
{
	HMODULE m_hModule = NULL;
	DWORD m_dwReturn[5] = { 0 };
	App* app;

	inline BOOL WINAPI Load()
	{
		TCHAR tzPath[MAX_PATH];
		TCHAR tzTemp[MAX_PATH * 2];

		GetSystemDirectory(tzPath, MAX_PATH);
		lstrcat(tzPath, TEXT("\\msimg32"));
		m_hModule = LoadLibrary(tzPath);
		if (m_hModule == NULL)
		{
			wsprintf(tzTemp, TEXT("�޷����� %s�������޷��������С�"), tzPath);
			MessageBox(NULL, tzTemp, TEXT("AheadLib"), MB_ICONSTOP);
		}

		return (m_hModule != NULL);
	}

	inline VOID WINAPI Free()
	{
		if (m_hModule)
		{
			FreeLibrary(m_hModule);
		}
	}

	FARPROC WINAPI GetAddress(PCSTR pszProcName)
	{
		FARPROC fpAddress;
		CHAR szProcName[16];
		TCHAR tzTemp[MAX_PATH];

		fpAddress = GetProcAddress(m_hModule, pszProcName);
		if (fpAddress == NULL)
		{
			if (HIWORD(pszProcName) == 0)
			{
				pszProcName = szProcName;
			}

			wsprintf(tzTemp, TEXT("�޷��ҵ����� %hs�������޷��������С�"), pszProcName);
			MessageBox(NULL, tzTemp, TEXT("AheadLib"), MB_ICONSTOP);
			ExitProcess(-2);
		}

		return fpAddress;
	}
}
using namespace AheadLib;
#pragma runtime_checks( "", off )
ALCDECL AheadLib_vSetDdrawflag(void)
{
	__asm PUSH m_dwReturn[0 * TYPE long];
	__asm CALL DWORD PTR[TlsSetValue];

	GetAddress("vSetDdrawflag")();

	__asm PUSH EAX;
	__asm PUSH m_dwReturn[0 * TYPE long];
	__asm CALL DWORD PTR[TlsGetValue];
	__asm XCHG EAX, [ESP];
	__asm RET;
}

ALCDECL AheadLib_AlphaBlend(void)
{
	__asm PUSH m_dwReturn[1 * TYPE long];
	__asm CALL DWORD PTR[TlsSetValue];

	GetAddress("AlphaBlend")();

	__asm PUSH EAX;
	__asm PUSH m_dwReturn[1 * TYPE long];
	__asm CALL DWORD PTR[TlsGetValue];
	__asm XCHG EAX, [ESP];
	__asm RET;
}

ALCDECL AheadLib_DllInitialize(void)
{
	__asm PUSH m_dwReturn[2 * TYPE long];
	__asm CALL DWORD PTR[TlsSetValue];

	GetAddress("DllInitialize")();

	__asm PUSH EAX;
	__asm PUSH m_dwReturn[2 * TYPE long];
	__asm CALL DWORD PTR[TlsGetValue];
	__asm XCHG EAX, [ESP];
	__asm RET;
}

ALCDECL AheadLib_GradientFill(void)
{
	__asm PUSH m_dwReturn[3 * TYPE long];
	__asm CALL DWORD PTR[TlsSetValue];

	GetAddress("GradientFill")();

	__asm PUSH EAX;
	__asm PUSH m_dwReturn[3 * TYPE long];
	__asm CALL DWORD PTR[TlsGetValue];
	__asm XCHG EAX, [ESP];
	__asm RET;
}

ALCDECL AheadLib_TransparentBlt(void)
{
	__asm PUSH m_dwReturn[4 * TYPE long];
	__asm CALL DWORD PTR[TlsSetValue];

	GetAddress("TransparentBlt")();

	__asm PUSH EAX;
	__asm PUSH m_dwReturn[4 * TYPE long];
	__asm CALL DWORD PTR[TlsGetValue];
	__asm XCHG EAX, [ESP];
	__asm RET;
}
#pragma runtime_checks( "", restore )
HMODULE  g_hModule = nullptr;

extern BNString datapath;

BOOL WINAPI DllMain(HMODULE hModule, DWORD dwReason, PVOID pvReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		g_hModule = hModule;
		if (pystring::find(get_command_line(), "--type") == -1) {
			AllocConsole();
			freopen("CONOUT$", "w", stdout);
#ifndef _DEBUG
			ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif


			namespace fs = std::filesystem;

			// Pick data folder
			if (getenv("BETTERNCM_PROFILE")) {
				datapath = getEnvironment("BETTERNCM_PROFILE");
			}
			else {
				if ((int)fs::status(getEnvironment("USERPROFILE") + L"\\betterncm").permissions() & (int)std::filesystem::perms::owner_write) {
					datapath = getEnvironment("USERPROFILE") + L"\\betterncm";
				}
				else {
					datapath = "C:\\betterncm";
				}
			}

			std::wcout << L"Data folder picked: " << datapath << "\n";

			if ((int)fs::status((wstring)datapath).permissions() & (int)std::filesystem::perms::owner_write) {
				// Create data folder
				fs::create_directories(datapath + L"/plugins");

				// PluginMarket
				HRSRC myResource = ::FindResource(hModule, MAKEINTRESOURCE(IDR_RCDATA1), RT_RCDATA);
				unsigned int myResourceSize = ::SizeofResource(hModule, myResource);
				HGLOBAL myResourceData = ::LoadResource(hModule, myResource);
				void* pMyBinaryData = ::LockResource(myResourceData);
				std::ofstream f(datapath + L"/plugins/PluginMarket.plugin", std::ios::out | std::ios::binary);
				f.write((char*)pMyBinaryData, myResourceSize);
				f.close();

				// Inject NCM
				app = new App();
			}
			else {
				alert(L"BetterNCM访问数据目录失败！可能需要以管理员身份运行或更改数据目录。\n\nBetterNCM将不会运行");
			}
		}







		DisableThreadLibraryCalls(hModule);

		for (INT i = 0; i < sizeof(m_dwReturn) / sizeof(DWORD); i++)
		{
			m_dwReturn[i] = TlsAlloc();
		}

		return Load();
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		delete app;
		for (INT i = 0; i < sizeof(m_dwReturn) / sizeof(DWORD); i++)
		{
			TlsFree(m_dwReturn[i]);
		}

		Free();
	}
	return TRUE;
}
