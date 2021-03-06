#include <string>
#include <vector>
// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the GSNETFILTER_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// GSNETFILTER_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef GSNETFILTER_EXPORTS
#define GSNETFILTER_API __declspec(dllexport)
#else
#define GSNETFILTER_API __declspec(dllimport)
#endif

// This class is exported from the GsNetFilter.dll
//class GSNETFILTER_API CGsNetFilter {
//public:
//	CGsNetFilter(void);
	// TODO: add your methods here.
//};
//
//extern GSNETFILTER_API int nGsNetFilter;
//
//GSNETFILTER_API int fnGsNetFilter(void);

GSNETFILTER_API BOOL GsEnable(BOOL bEnable, USHORT listen_port);
GSNETFILTER_API HANDLE GsStartProxy(USHORT* listen_port);
GSNETFILTER_API BOOL GsSetHook(const std::wstring& dllPath);
GSNETFILTER_API VOID GsEnableRedirect(BOOL bEnable);

GSNETFILTER_API bool GsUpdateConfigVideoHost(const std::string& url,int istate = 0);
GSNETFILTER_API bool GsUpdateConfigWhiteHost(const std::string& url,bool bEnable);
GSNETFILTER_API bool GsGetWebRules(const std::wstring& filename);
GSNETFILTER_API bool GsGetVideoRules(const std::wstring& filename);
GSNETFILTER_API bool GsGetUsersRules(const std::wstring& filename);
GSNETFILTER_API bool GsGetRedirectRules(const std::wstring& filename);