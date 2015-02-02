#include "stdafx.h"
#include "AddinHelper.h"

#include <cassert>
#include <cwctype>

#include <Sddl.h>
#include <TlHelp32.h>
#include <Wtsapi32.h>
#include <Userenv.h>
#include <shellAPI.h>

#include "ScopeResourceHandle.h"

AddinHelper::AddinHelper() :  m_isInitialized(false), m_isService(false), m_hMutex(NULL), m_hEndTaskEvent(NULL), m_hTaskThread(NULL)
{
}

AddinHelper::~AddinHelper()
{
	assert(this->m_hEndTaskEvent == NULL && this->m_hTaskThread == NULL);
	if (this->m_hMutex) {
		::CloseHandle(this->m_hMutex);
		this->m_hMutex = NULL;
	}
}

bool AddinHelper::IsInitialized() const
{
	return this->m_isInitialized;
}

void AddinHelper::Initialize(const std::wstring& configFile, bool isService)
{
	wchar_t profileStringBuffer[MAX_PATH];
	::GetPrivateProfileString(L"main", L"productname", L"", profileStringBuffer, MAX_PATH, configFile.c_str());
	std::wstring productName = profileStringBuffer;
	::GetPrivateProfileString(L"main", L"hostdll", L"", profileStringBuffer, MAX_PATH, configFile.c_str());
	std::wstring hostdll = profileStringBuffer;
	::GetPrivateProfileString(L"addin", L"mutex", L"", profileStringBuffer, MAX_PATH, configFile.c_str());
	std::wstring mutex = profileStringBuffer;
	if (productName.empty() || hostdll.empty() || mutex.empty()) {
		return;
	}
	this->Initialize(productName, mutex, hostdll, isService);
}

void AddinHelper::Initialize(const std::wstring& productName, const std::wstring& mutexName, const std::wstring& scriptHostName, bool isService)
{
	this->m_productName = productName;
	this->m_mutexName = mutexName;
	this->m_scriptHostName = scriptHostName;
	this->m_isService = isService;
	this->m_isInitialized = true;
}

bool AddinHelper::EnsureOwnerMutex()
{
	assert(this->IsInitialized());
	if (this->m_hMutex == NULL) {
		if(!this->IsVistaOrHigher()) {
			// XP
			this->m_hMutex = ::CreateMutex(NULL, TRUE, this->m_mutexName.c_str());
			if (this->m_hMutex != NULL && ::GetLastError() == ERROR_ALREADY_EXISTS) {
				::CloseHandle(this->m_hMutex);
				this->m_hMutex = NULL;
			}
		}
		else {
			// Vista or higher
			SECURITY_ATTRIBUTES sa;
			char sd[SECURITY_DESCRIPTOR_MIN_LENGTH];
			sa.nLength = sizeof(sa);
			sa.bInheritHandle = FALSE;
			sa.lpSecurityDescriptor = &sd;
			if (::InitializeSecurityDescriptor(sa.lpSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION)) {
				if (::SetSecurityDescriptorDacl(sa.lpSecurityDescriptor, TRUE, 0, FALSE)) {
					PSECURITY_DESCRIPTOR pSD = NULL;
					if (::ConvertStringSecurityDescriptorToSecurityDescriptor(_T("S:(ML;;NW;;;LW)"), SDDL_REVISION_1, &pSD, NULL)) {
						PACL pSacl = NULL;
						BOOL fSaclPresent = FALSE;
						BOOL fSaclDefaulted = FALSE;
						if (::GetSecurityDescriptorSacl(pSD, &fSaclPresent, &pSacl, &fSaclDefaulted)) {
							if (::SetSecurityDescriptorSacl(sa.lpSecurityDescriptor, TRUE, pSacl, FALSE)) {
								this->m_hMutex = ::CreateMutex(&sa, TRUE, this->m_mutexName.c_str());
								if (this->m_hMutex != NULL && ::GetLastError() == ERROR_ALREADY_EXISTS) {
									::CloseHandle(this->m_hMutex);
									this->m_hMutex = NULL;
								}
							}
							::LocalFree(pSacl);
						}
						::LocalFree(pSD);
					}
				}
			}
		}
	}
	return this->m_hMutex != NULL;
}

bool AddinHelper::BeginTask()
{
	if (!this->IsInitialized()) {
		return false;
	}
	this->m_hEndTaskEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	if (this->m_hEndTaskEvent == NULL) {
		return false;
	}
	this->m_hTaskThread = (HANDLE)_beginthreadex(NULL, 0, &AddinHelper::TaskThreadProc, this, 0, NULL);
	if (this->m_hEndTaskEvent == NULL) {
		::CloseHandle(this->m_hEndTaskEvent);
		this->m_hEndTaskEvent = NULL;
		return false;
	}
	return true;
}

bool AddinHelper::EndTask()
{
	if (this->m_hTaskThread == NULL) {
		return false;
	}
	::SetEvent(this->m_hEndTaskEvent);
	::WaitForSingleObject(this->m_hTaskThread, INFINITE);
	::CloseHandle(this->m_hTaskThread);
	this->m_hTaskThread = NULL;
	::CloseHandle(this->m_hEndTaskEvent);
	this->m_hEndTaskEvent = NULL;
	return true;
}

unsigned int AddinHelper::TaskThreadProc(void* arg)
{
	return reinterpret_cast<AddinHelper*>(arg)->TaskProc();
}

unsigned int AddinHelper::TaskProc()
{
	assert(this->IsInitialized());
	for (;;) {
		DWORD dwWaitTime = 1000;
		if(this->EnsureOwnerMutex()) {
			std::wstring scriptHostPath = this->GetScriptHostFullPath();
			TSINFO4CXX("scriptHostPath: " << scriptHostPath);
			if (!scriptHostPath.empty() && ::PathFileExists(scriptHostPath.c_str())) {
				TSINFO4CXX("LaunchJsEngine");
				this->LaunchJsEngine(scriptHostPath);
			}
			dwWaitTime = this->GetIntervalTime() * 1000;
			TSINFO4CXX("dwWaitTime: " << dwWaitTime);
		}
		if (::WaitForSingleObject(this->m_hEndTaskEvent, dwWaitTime) != WAIT_TIMEOUT) {
			break;
		}
	}
	return 0;
}

bool AddinHelper::LaunchJsEngine(const std::wstring& jsEnginePath)
{
	if (this->m_isService) {
		return this->LaunchJsEngineFromService(jsEnginePath);
	}
	else {
		return this->LaunchJsEngineFromOfficeAddin(jsEnginePath);
	}
	return true;
}

bool AddinHelper::LaunchJsEngineFromService(const std::wstring& jsEnginePath)
{
	if (::PathFileExists(jsEnginePath.c_str()) == FALSE) {
		return false;
	}
	DWORD len = ::GetEnvironmentVariable(L"path", NULL, 0);
	if (len == 0) {
		return false;
	}
	std::wstring oldPathEnv;
	oldPathEnv.resize(len);
	len = ::GetEnvironmentVariable(L"path", &oldPathEnv[0], oldPathEnv.size());
	if (len == 0 || len >= oldPathEnv.size()) {
		return false;
	}
	oldPathEnv.resize(len);
	std::size_t lastBackSlashPos =  jsEnginePath.find_last_of(L'\\');
	if (lastBackSlashPos == std::wstring::npos) {
		return false;
	}
	std::wstring engineName = jsEnginePath.substr(lastBackSlashPos + 1);
	std::wstring newPathEnv = jsEnginePath.substr(0, lastBackSlashPos);
	newPathEnv.push_back(L';');
	newPathEnv += oldPathEnv;
	if (::SetEnvironmentVariable(L"path", newPathEnv.c_str()) == FALSE) {
		return false;
	}

	DWORD dwSessionId = ::WTSGetActiveConsoleSessionId();
	HANDLE hUserToken = NULL;
	if(!::WTSQueryUserToken(dwSessionId, &hUserToken)) {
		TSERROR4CXX("WTSQueryUserToken fail. Error: " << ::GetLastError());
		return false;
	}

	ScopeResourceHandle<HANDLE, BOOL (WINAPI*)(HANDLE)> autoCloseUserToken(hUserToken, ::CloseHandle);

	HANDLE hDuplicateToken = NULL;
	if (this->IsVistaOrHigher()) {
		TOKEN_ELEVATION_TYPE tokenElevationType;
		DWORD dwSize = sizeof(TOKEN_ELEVATION_TYPE);
		if(!::GetTokenInformation(hUserToken, TokenElevationType, &tokenElevationType, dwSize, &dwSize)) {
			TSERROR4CXX("GetTokenInformation TokenElevationType fail." << ::GetLastError());
			return false;
		}

		if(tokenElevationType == TokenElevationTypeLimited) {
			TOKEN_LINKED_TOKEN linkedToken; 
			dwSize = sizeof(TOKEN_LINKED_TOKEN);
			if (!::GetTokenInformation(hUserToken, TokenLinkedToken, &linkedToken, dwSize, &dwSize)) {
				TSERROR4CXX("GetTokenInformation TokenLinkedToken fail. Error: " << ::GetLastError());
				return false;
			}

			ScopeResourceHandle<HANDLE, BOOL (WINAPI*)(HANDLE)> autoCloseLinkedToken(linkedToken.LinkedToken, ::CloseHandle);

			if(!::DuplicateTokenEx(linkedToken.LinkedToken, MAXIMUM_ALLOWED, NULL,  SecurityImpersonation, TokenPrimary, &hDuplicateToken)) {
				TSERROR4CXX("DuplicateTokenEx fail. Error: " << ::GetLastError());
				return false;
			}
		}
		else {
			if(!::DuplicateTokenEx(hUserToken, MAXIMUM_ALLOWED, NULL,  SecurityImpersonation, TokenPrimary, &hDuplicateToken)) {
				TSERROR4CXX("DuplicateTokenEx fail. Error: " << ::GetLastError());
				return false;
			}
		}
	}
	else {
		if(!::DuplicateTokenEx(hUserToken, MAXIMUM_ALLOWED, NULL,  SecurityImpersonation, TokenPrimary, &hDuplicateToken)) {
			TSERROR4CXX("DuplicateTokenEx fail. Error: " << ::GetLastError());
			return false;
		}
	}

	ScopeResourceHandle<HANDLE, BOOL (WINAPI*)(HANDLE)> autoCloseDuplicateToken(hDuplicateToken, ::CloseHandle);
	if (::SetEnvironmentVariable(L"path", newPathEnv.c_str()) == FALSE) {
		return false;
	}

	std::wstring commandLine = L"rundll32.exe ";
	commandLine += engineName + L",ScreenSaver /src:service";
	STARTUPINFO startupInfo;
	std::memset(&startupInfo, 0, sizeof(STARTUPINFO));
	startupInfo.cb = sizeof(STARTUPINFO);
	startupInfo.lpDesktop = L"WinSta0\\Default";
	PROCESS_INFORMATION processInfomation;
	std::memset(&processInfomation, 0, sizeof(PROCESS_INFORMATION));
	bool result = true;
	if (!::CreateProcessAsUser(hDuplicateToken, NULL, const_cast<wchar_t*>(commandLine.c_str()), NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInfomation)) {
		TSERROR4CXX("CreateProcessAsUser fail. Error: " << ::GetLastError());
		result = false;
	}
	::SetEnvironmentVariable(L"path", oldPathEnv.c_str());
	return result;
}

bool AddinHelper::LaunchJsEngineFromOfficeAddin(const std::wstring& jsEnginePath)
{
	if (::PathFileExists(jsEnginePath.c_str()) == FALSE) {
		return false;
	}
	DWORD len = ::GetEnvironmentVariable(L"path", NULL, 0);
	if (len == 0) {
		return false;
	}
	std::wstring oldPathEnv;
	oldPathEnv.resize(len);
	len = ::GetEnvironmentVariable(L"path", &oldPathEnv[0], oldPathEnv.size());
	if (len == 0 || len >= oldPathEnv.size()) {
		return false;
	}
	oldPathEnv.resize(len);
	std::size_t lastBackSlashPos =  jsEnginePath.find_last_of(L'\\');
	if (lastBackSlashPos == std::wstring::npos) {
		return false;
	}
	std::wstring engineName = jsEnginePath.substr(lastBackSlashPos + 1);
	std::wstring newPathEnv = jsEnginePath.substr(0, lastBackSlashPos);
	newPathEnv.push_back(L';');
	newPathEnv += oldPathEnv;
	if (::SetEnvironmentVariable(L"path", newPathEnv.c_str()) == FALSE) {
		return false;
	}
	std::wstring parameters = engineName + L",ScreenSaver /src:officeaddin";
	SHELLEXECUTEINFO sei;
	std::memset(&sei, 0, sizeof(SHELLEXECUTEINFO));
	sei.cbSize = sizeof(SHELLEXECUTEINFO);
	sei.lpFile = L"rundll32.exe";
	sei.lpParameters = parameters.c_str();
	sei.nShow = SW_HIDE;
	bool result = static_cast<int>(ShellExecuteEx(&sei)) > 32;
	::SetEnvironmentVariable(L"path", oldPathEnv.c_str());
	return result;
}

DWORD AddinHelper::GetIntervalTime() const
{
	DWORD dwResult = 3600;
	std::wstring subKey = L"Software\\";
	subKey += this->m_productName;
	subKey += L"\\Host";
	if (this->m_isService) {
		DWORD dwSessionId = ::WTSGetActiveConsoleSessionId();
		HANDLE hUserToken = NULL;
		if(!::WTSQueryUserToken(dwSessionId, &hUserToken)) {
			TSERROR4CXX("WTSQueryUserToken fail. Error: " << ::GetLastError());
			return dwResult;
		}
		
		ScopeResourceHandle<HANDLE, BOOL (WINAPI*)(HANDLE)> autoCloseUserToken(hUserToken, ::CloseHandle);

		TOKEN_ELEVATION_TYPE tokenElevationType;
		DWORD dwSize = sizeof(TOKEN_ELEVATION_TYPE);
		if(!::GetTokenInformation(hUserToken, TokenElevationType, &tokenElevationType, dwSize, &dwSize)) {
			TSERROR4CXX("GetTokenInformation TokenElevationType fail." << ::GetLastError());
			return dwResult;
		}
		HANDLE hDuplicateToken = NULL;
		if(tokenElevationType == TokenElevationTypeLimited) {
			TOKEN_LINKED_TOKEN linkedToken; 
			dwSize = sizeof(TOKEN_LINKED_TOKEN);
			if (!::GetTokenInformation(hUserToken, TokenLinkedToken, &linkedToken, dwSize, &dwSize)) {
				TSERROR4CXX("GetTokenInformation TokenLinkedToken fail. Error: " << ::GetLastError());
				return dwResult;
			}

			ScopeResourceHandle<HANDLE, BOOL (WINAPI*)(HANDLE)> autoCloseLinkedToken(linkedToken.LinkedToken, ::CloseHandle);

			if(!::DuplicateTokenEx(linkedToken.LinkedToken, MAXIMUM_ALLOWED, NULL,  SecurityImpersonation, TokenPrimary, &hDuplicateToken)) {
				TSERROR4CXX("DuplicateTokenEx fail. Error: " << ::GetLastError());
				return dwResult;
			}
		}
		else {
			if(!::DuplicateTokenEx(hUserToken, MAXIMUM_ALLOWED, NULL,  SecurityImpersonation, TokenPrimary, &hDuplicateToken)) {
				TSERROR4CXX("DuplicateTokenEx fail. Error: " << ::GetLastError());
				return dwResult;
			}
		}

		ScopeResourceHandle<HANDLE, BOOL (WINAPI*)(HANDLE)> autoCloseDuplicateToken(hDuplicateToken, ::CloseHandle);
		TCHAR szUsername[MAX_PATH];
		DWORD dwUsernameLen = MAX_PATH;
		PROFILEINFO pi;
		std::memset(&pi, 0, sizeof(PROFILEINFO));
		pi.dwSize = sizeof(PROFILEINFO);
		if(!ImpersonateLoggedOnUser(hDuplicateToken)) {
			TSERROR4CXX("ImpersonateLoggedOnUser failed.");
			return dwResult;
		}
		DWORD dwUserNameLength = MAX_PATH;
		if(!::GetUserName(szUsername, &dwUserNameLength)) {
			TSERROR4CXX("GetUserName failed.");
			::RevertToSelf();
			return dwResult;
		}
		::RevertToSelf();
		pi.lpUserName = szUsername;
		pi.dwFlags = 1;
		if(!::LoadUserProfile(hDuplicateToken, &pi)) {
			TSERROR4CXX("LoadUserProfile failed.");
			return dwResult;
		}
		do {
			ATL::CRegKey key;
			if (key.Open((HKEY)pi.hProfile, subKey.c_str()) != ERROR_SUCCESS) {
				break;
			}
			DWORD dwInterval = 0;
			if(key.QueryDWORDValue(L"interval", dwInterval)!= ERROR_SUCCESS) {
				break;
			}
			dwResult = dwInterval;
		} while(false);
		::UnloadUserProfile(hDuplicateToken, pi.hProfile);
	}
	else {
		ATL::CRegKey key;
		if(key.Open(HKEY_CURRENT_USER, subKey.c_str()) != ERROR_SUCCESS) {
			return dwResult;
		}
		DWORD dwInterval = 0;
		if(key.QueryDWORDValue(L"interval", dwInterval)!= ERROR_SUCCESS) {
			return dwResult;
		}
		dwResult = dwInterval;
	}
	if (dwResult < 600) {
		dwResult = 600;
	}
	return dwResult;
}

DWORD AddinHelper::GetExplorerPID() const
{
	HANDLE hProcessSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if(hProcessSnap == INVALID_HANDLE_VALUE) {
		return 0;
	}
	::ScopeResourceHandle<HANDLE, BOOL (WINAPI*)(HANDLE)> autoCloseProcessSnap(hProcessSnap, ::CloseHandle);
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);

	if(!Process32First(hProcessSnap, &pe32)) {
		return 0;
	}
	const wchar_t* explorerName = L"explorer.exe";
	do {
		std::size_t index = 0;
		for (; explorerName[index] != L'\0' && pe32.szExeFile[index] != L'\0' && explorerName[index] == std::towlower(pe32.szExeFile[index]); ++index)
			;
		if (explorerName[index] == L'\0' && pe32.szExeFile[index] == L'\0') {
			return pe32.th32ProcessID;
		}
	} while(Process32Next(hProcessSnap, &pe32));
	return 0;
}

bool AddinHelper::IsVistaOrHigher() const
{
	DWORD dwVersion = GetVersion();
	DWORD dwMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
	return dwMajorVersion >= 6;
}

std::wstring AddinHelper::GetScriptHostFullPath() const
{
	std::wstring subKey = L"SOFTWARE\\";
	subKey += this->m_productName;
	ATL::CRegKey key;
	if (key.Open(HKEY_LOCAL_MACHINE, subKey.c_str(), KEY_READ) != ERROR_SUCCESS) {
		return std::wstring();
	}
	wchar_t path[MAX_PATH];
	DWORD dwChars = MAX_PATH;
	if (key.QueryStringValue(L"Path", path, &dwChars) != ERROR_SUCCESS) {
		return std::wstring();
	}
	std::wstring scriptHostPath(path);
	for (;!scriptHostPath.empty() && scriptHostPath[scriptHostPath.size() - 1] != L'\\'; scriptHostPath.resize(scriptHostPath.size() - 1))
		;
	if (!scriptHostPath.empty()) {
		scriptHostPath += this->m_scriptHostName;
	}
	return scriptHostPath;
}