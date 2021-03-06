function OnPopupMenu(self)
	SetTitleText(self)	
end

function OnEndMenu(self)
	-- _G["gMenu"] = nil
	-- local tree = self:GetBindUIObjectTree()
	-- if tree then
		-- local objMainLayout = tree:GetUIObject("TrayMenu.Main")
		-- local menu = objMainLayout:GetObject("context_menu")
		-- menu:AnimateHide()
	-- end
end

function OnBindObjectTree(self, menuTree)
	local objMainLayout = menuTree:GetUIObject("TrayMenu.Main")
	local context = objMainLayout:GetObject("TrayMenu.Context")
	context:OnInitControl()	
end

------------------
function SetTitleText(self)
	local menuTree = self:GetBindUIObjectTree()
	local objMainLayout = menuTree:GetUIObject("TrayMenu.Main")
	local objTitleText = objMainLayout:GetObject("TrayMenu.Title.Text")
	if objTitleText == nil then
		return
	end
	
	local tFunctionHelper = XLGetGlobal("Project.FunctionHelper")
	if type(tFunctionHelper.ReadConfigFromMemByKey) ~= "function" then
		return
	end

	local tUserConfig = tFunctionHelper.ReadConfigFromMemByKey("tUserConfig") or {}
	local nFilterCount = tonumber(tUserConfig["nFilterCountOneDay"]) or 0
	local strText = "当天累计拦截广告"..tostring(nFilterCount).."次"
	objTitleText:SetText(strText)	
end