/********************************************************************
*
* =-----------------------------------------------------------------=
* =                                                                 =
* =             Copyright (c) Xunlei, LTD. 2003-2009                =
* =                                                                 =
* =-----------------------------------------------------------------=
* 
*   FileName    :   XLI18N.h
*   Author      :   xlue group(xlue@xunlei.com)
*   Create      :   9:12:2008   20:06
*   LastChange  :   2008/12/09
*   History     :	
*								
*   Description :   xlue�����Թ���ģ��   
*
********************************************************************/  

#ifndef __XLI18N_H__
#define __XLI18N_H__

#ifdef XLUE_EXPORTS
	#ifdef __cplusplus 
		#define XLI18N_API(x) extern "C" __declspec(dllexport) x __stdcall 
	#else
		#define XLI18N_API(x) __declspec(dllexport) x __stdcall 
	#endif //__cplusplus
#elif defined (XLUE_UNION)
	#ifdef __cplusplus 
		#define XLI18N_API(x) extern "C" x __stdcall 
	#else
		#define XLI18N_API(x) x __stdcall 
	#endif //__cplusplus
#else // not XLUE_EXPORTS
	#ifdef __cplusplus
		#define XLI18N_API(x) extern "C" __declspec(dllimport) x __stdcall
	#else
		#define XLI18N_API(x) __declspec(dllimport) x __stdcall
	#endif //__cplusplus
#endif //XLUE_EXPORTS


//�Զ�������
typedef void* XLI18N_LANGUAGE_PACKAGE_HANDLE;
typedef long (__stdcall *fnXLI18NLocationChangeEvent) (void* userData,const wchar_t* strOldLocationID,const wchar_t* strNewLocationID);
typedef long XLI18N_LOCATION_EVENT_HANDLE;

//������Ϣ����
//LPARAM,WPARAM��δ����
#define WM_XLI18N_LOCATION_CHANGE (WM_USER + 2129) 

//һЩ�������Ͷ���
#define XLI18N_TEXTID_ASSERT_DISABLE 0
#define XLI18N_TEXTID_ASSERT_ENABLE  1
#define XLI18N_TEXTID_ASSERT_DEFAULT 2

//���еĴ����붨��
#define XLI18N_RESULT_SUCCESS               0
#define XLI18N_RESULT_PACKAGE_NOT_FOUND     1
#define XLI18N_RESULT_PACKAGE_LOAD_FAILED   2
#define XLI18N_RESULT_PACKAGE_INVALID       3
#define XLI18N_RESULT_TEXTID_NOT_FOUND      4
#define XLI18N_RESULT_LOCATION_NOT_SUPPORT  5
#define XLI18N_RESULT_EVENT_INVALID         6
#define XLI18N_RESULT_HWND_EXIST            7
#define XLI18N_RESULT_HWND_NOT_REG          8
#define XLI18N_RESULT_NOT_INIT              9
#define XLI18N_RESULT_INIT_ERROR           10
#define XLI18N_RESULT_NOT_IMP              11
#define XLI18N_RESULT_ROOT_DIR_ERROR       12
#define XLI18N_RESULT_HAVE_INITED          13

//��׼��LocationID����
#define XLI18N_LOCATION_ZH_CN   TEXT("zh_CN")
#define XLI18N_LOCATION_ZH_TW   TEXT("zh_TW")
#define XLI18N_LOCATION_EN_US   TEXT("en_US")

//*** NOTICE: �������нӿں����������̵߳���
//----------------------------------------------------------------------------------------------------
//---��ʼ�����---
XLI18N_API(const wchar_t*)   XLI18N_GetRootDir();
XLI18N_API(long)     XLI18N_Init(const wchar_t* strBaseDirPath,const wchar_t* strLocationID);
XLI18N_API(size_t)   XLI18N_GetSupportLocationList(wchar_t* strOutBuffer,size_t bufferLen);
XLI18N_API(BOOL)     XLI18N_IsSupportLocation(const wchar_t* strLocationID);

XLI18N_API(void)	XLI18N_SetTextIDAssertMode(long modeType);
XLI18N_API(long)	XLI18N_GetTextIDAssertMode();

//---���԰�����---
XLI18N_API(long)     XLI18N_PreloadLanguagePackage(const wchar_t* strPackageName);
XLI18N_API(long)     XLI18N_UnloadLanguagePackage(const wchar_t* strPackageName);

//---��ȡ�ַ���---
XLI18N_API(long) XLI18N_GetLanguagePackage(const wchar_t* packageName,XLI18N_LANGUAGE_PACKAGE_HANDLE* pResult);

//֧���ޱ���ģʽ,���textID�޷��ҵ��ַ���,Ҳ���᷵��NULL,���Ƿ���textID����,���Enable��TextID assert,�����һ��assert�������
XLI18N_API(const wchar_t*) XLI18N_LoadTextFromPackage(XLI18N_LANGUAGE_PACKAGE_HANDLE hPackage,const wchar_t* textID,long* pResultCode);
XLI18N_API(const wchar_t*) XLI18N_LoadText(const wchar_t* packageName,const wchar_t* textID,long* pResultCode);

XLI18N_API(long) XLI18N_ReleaseLanguagePackage(XLI18N_LANGUAGE_PACKAGE_HANDLE hPackage);

//---�����л����¼�---
XLI18N_API(long) XLI18N_ActiveLocation(const wchar_t* strLocationID);
XLI18N_API(const wchar_t*) XLI18N_GetActiveLocationID();
XLI18N_API(XLI18N_LOCATION_EVENT_HANDLE) XLI18N_RegisterLocationChangeEvent(fnXLI18NLocationChangeEvent pfnEvent,
                                                                               void* userData);
XLI18N_API(long) XLI18N_RemoveLocationChangeEvent(XLI18N_LOCATION_EVENT_HANDLE hLocationEvent,void** ppUserData);

//����Lua��װ��ֱ�ӷ�����i18n��dll�����Ӧ�ý���һ��������Host���̵ġ�
//Ϊ�˱��������ĵ�����ʱֱ����void*���� XL_LRT_ENV_HANDLE
XLI18N_API(void) XLI18N_RegisterLuaClass(void* hEnv);

#endif // __XLI18N_H__