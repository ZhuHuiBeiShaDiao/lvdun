﻿#include "stdafx.h"
#include "CDDMsgWnd.h"
#include "LuaListenPre.h"
#include "LuaPrefactory.h"

#include <XLLuaRuntime.h>



CDDMsgWindow* LuaListenPreFactory::CreateInstance()
{
	return  CDDMsgWindow::Instance();
}

int LuaListenPreFactory::CreateInstance(lua_State* luaState)
{
	CDDMsgWindow* pResult =  CDDMsgWindow::Instance();
	XLLRT_PushXLObject(luaState,APILISTEN_LUA_CLASS,pResult);
	return 1;
}

LuaListenPreFactory* __stdcall LuaListenPreFactory::Instance(void*)
{
	static LuaListenPreFactory* pTheOne = NULL;
	if(pTheOne == NULL)
	{
		pTheOne = new LuaListenPreFactory();
	}
	return pTheOne;
}

static XLLRTGlobalAPI LuaXLYYPreFactoryMemberFunctions[] = 
{
	{"CreateInstance",LuaListenPreFactory::CreateInstance},
	{NULL,NULL}
};

void LuaListenPreFactory::RegisterObj(XL_LRT_ENV_HANDLE hEnv)
{
	if(hEnv == NULL)
		return ;

	XLLRTObject theObject;
	theObject.ClassName = APIL_FACTORY_LUA_CLASS;
	theObject.MemberFunctions = LuaXLYYPreFactoryMemberFunctions;
	theObject.ObjName = APIL_FACTORY_LUA_OBJ;
	theObject.userData = NULL;
	theObject.pfnGetObject = (fnGetObject)LuaListenPreFactory::Instance;

	XLLRT_RegisterGlobalObj(hEnv,theObject); 
}