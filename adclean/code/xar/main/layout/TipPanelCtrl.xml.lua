local tFunHelper = XLGetGlobal("Project.FunctionHelper")
local tipUtil = tFunHelper.tipUtil

----方法----
function SetTipData(self, infoTab) 
	CreateFilterListener(self)
	self:UpdateMainWndBkg()
	return true
end


function UpdateMainWndBkg(self)
	local bFilterOpen = tFunHelper.GetFilterState() 
	local objCloseBkg = self:GetControlObject("MainWnd.Up.Bkg.CloseFilter")
	local objOpenBkg = self:GetControlObject("MainWnd.Up.Bkg.OpenFilter")
		
	if bFilterOpen then
		objCloseBkg:SetVisible(false)
		objOpenBkg:SetVisible(true)
		objOpenBkg:Play()
	else
		objCloseBkg:SetVisible(true)
		objOpenBkg:SetVisible(false)
		objOpenBkg:Stop()
	end	
end



---事件--
function OnClickCloseBtn(self)
	HideWndToTray(self)
end

function OnClickMinBtn(self)
	local objTree = self:GetOwner()
	if nil == objTree then
		return
	end
	
	local objHostWnd = objTree:GetBindHostWnd()
	if nil == objHostWnd then
		return
	end
	
	objHostWnd:Min()	
end



--监听事件
function CreateFilterListener(objRootCtrl)
	local objFactory = XLGetObject("GSListen.Factory")
	if not objFactory then
		tFunHelper.TipLog("[CreateFilterListener] not support GSListen.Factory")
		return
	end
	
	local objListen = objFactory:CreateInstance()	
	objListen:AttachListener(
		function(key,...)	

			tFunHelper.TipLog("[CreateFilterListener] key: " .. tostring(key))
			
			local tParam = {...}	
			if tostring(key) == "OnFilterResult" then
				OnFilterResult(tParam)
			elseif tostring(key) == "OnCommandLine" then
				OnCommandLine(tParam)
			end
			
			return
		end)
end

-------------
function ShowHostWnd()
	local objHostWnd = tFunHelper.GetMainWndInst()
	if objHostWnd then
		objHostWnd:Show(5)
		objHostWnd:BringWindowToTop(true)
	end
end

function HideWndToTray(objUIElement)
	local objTree = objUIElement:GetOwner()
	local objHostWnd = objTree:GetBindHostWnd()
	objHostWnd:Show(0)
end


function OnCommandLine(tParam)
	ShowHostWnd()
end

function OnFilterResult(tParam)
	local bFilterSucc = tParam[1]
	if not bFilterSucc then
		return
	end 
		
	local objAdvCount = tFunHelper.GetMainCtrlChildObj("MainWnd.Low.AdvCount")
	if objAdvCount == nil then
		tFunHelper.TipLog("[OnFilterResult] get ChildCtrl_AdvCount failed")
		return
	end
	
	objAdvCount:AddAdvCount()
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

function IsNilString(AString)
	if AString == nil or AString == "" then
		return true
	end
	return false
end


function IsRealString(AString)
    return type(AString) == "string" and AString ~= ""
end

