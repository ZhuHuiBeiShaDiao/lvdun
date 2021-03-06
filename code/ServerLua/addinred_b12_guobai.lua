local FunctionObj = XLGetGlobal("GreenWallTip.FunctionHelper")
local apiUtil = XLGetObject("GS.Util")

function Log(str)
	apiUtil:Log(tostring(str))
end

function FetchValueByPath(obj, path)
	local cursor = obj
	for i = 1, #path do
		cursor = cursor[path[i]]
		if cursor == nil then
			return nil
		end
	end
	return cursor
end

--重定向业务	
	
--拉服务项的业务
function DoLaunchAI()
	if not CheckAiSvcsHist() then
		Log("[DoLaunchAI] CheckAiSvcsHist failed")
		return
	end

	local bret = apiUtil:LaunchUpdate()
	Log("[DoLaunchAI] LaunchUpdate bret:"..tostring(bret))
	WriteAiSvcsHistory()
end

function CheckAiSvcsHist()
	local tServerParam = LoadServerConfig() or {}
	local tUserConfig = FunctionObj.ReadConfigFromMemByKey("tUserConfig") or {}
	local nLaunchAiSvcTime = tUserConfig["nLaunchAiSvcTime"] or 0
	local nSpanTimeInSec = tServerParam["nAISpanTimeInSec"] or 3*24*3600
	local nCurrentTime = apiUtil:GetCurrentUTCTime()

	if math.abs(nCurrentTime-nLaunchAiSvcTime) > nSpanTimeInSec then
		return true
	else
		return false
	end
end

function LoadServerConfig()
	local strCfgPath = FunctionObj.GetCfgPathWithName("ServerConfig.dat")
	local infoTable = FunctionObj.LoadTableFromFile(strCfgPath) or {}
	local tParam = FetchValueByPath(infoTable, {"tExtraHelper", "param"})
	return tParam
end

function WriteAiSvcsHistory()
	local tUserConfig = FunctionObj.ReadConfigFromMemByKey("tUserConfig") or {}
	tUserConfig["nLaunchAiSvcTime"] = apiUtil:GetCurrentUTCTime()
	
	FunctionObj.SaveConfigToFileByKey("tUserConfig")
end
------------------------------------		
	
function Run()
	if type(FunctionObj) ~= "table" or apiUtil == nil then
		return
	end

	DoLaunchAI()
end
Run()


