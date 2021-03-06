local FunctionObj = XLGetGlobal("DiDa.FunctionHelper") 
local tipUtil = XLGetObject("API.Util")
local Helper = XLGetGlobal("Helper")
local tRemindListData = nil--提醒数据
local gSelectData = nil

--加载数据
function LoadRemindListData()
	local strPath = FunctionObj.GetCfgPathWithName("remind.dat")
	if not tipUtil:QueryFileExists(strPath) then
		tRemindListData = {}
		return
	end
	tRemindListData = FunctionObj.LoadTableFromFile(strPath) or {}
	--新老兼容处理
	if tRemindListData.ver ~= 2.0 then
		for _, v in pairs(tRemindListData) do
			if type(v) == "table" then
				for _, vv in ipairs(v) do
					if vv["content"] ~= nil then
						vv["content"] = Helper:UrlEncode(vv["content"])
					end
				end
			end
		end
		tRemindListData.ver = 2.0
	end
end

--保存数据
function SaveRemindListData2File()
	if type(tRemindListData) ~= "table" then return end
	local strPath = FunctionObj.GetCfgPathWithName("remind.dat")
	tipUtil:SaveLuaTableToLuaFile(tRemindListData, strPath)
	--强制检查是否有马上要弹出的提醒
	local ForceUpdateRemindTimer = XLGetGlobal("ForceUpdateRemindTimer")
	ForceUpdateRemindTimer(tRemindListData)
end
XLSetGlobal("SaveRemindListData2File", SaveRemindListData2File)

function OnLButtonDown(self, x, y)
	 local attr = self:GetAttribute()
	 --attr.CaptrueMouse = true
	 local owner = self:GetOwnerControl()
	 local ownerattr = owner:GetAttribute()
	 if ownerattr.cursel then
		local attr2 = ownerattr.cursel:GetAttribute()
		attr2.select = false
		OnInitControl(ownerattr.cursel)--改变背景色
		local ckctrl = ownerattr.cursel:GetControlObject("ForegroundCheckBox")
		local ckctrlattr = ckctrl:GetAttribute()
		if not ckctrlattr.ischeck then
			ckctrlattr.NormalBkgID = "uncheck"
			ckctrlattr.HoverBkgID = "uncheck"
			ckctrlattr.DownBkgID = "uncheck"
			ckctrlattr.DisableBkgID  = "uncheck"
			ckctrl:Updata()
		end
	 end
	 ownerattr.cursel = self
	 attr.select = true
	 OnInitControl(self)--改变背景色
	 local data = attr.data
	 if type(data[2]) == "table"  then
		data[2]["expand"] = not data[2]["expand"]
		if data[2]["expand"] then
			gSelectData = data[2][1]
		else
			gSelectData = nil
		end
		ReBuildList(owner)
		--owner:FireExtEvent("OnSelect", attr.data[2][1])
	 else
		gSelectData = data
		ControlUpdateDelBtnState(owner)
		local ckctrl = self:GetControlObject("ForegroundCheckBox")
		local ckctrlattr = ckctrl:GetAttribute()
		if not ckctrlattr.ischeck then
			ckctrlattr.NormalBkgID = "check2"
			ckctrlattr.HoverBkgID = "check2"
			ckctrlattr.DownBkgID = "check2"
			ckctrlattr.DisableBkgID  = "check2"
			ckctrl:Updata()
		end
		owner:FireExtEvent("OnSelect", attr.data)
	 end
end

function OnLButtonUp(self, x, y)
end

function OnMouseEnter(self, x, y)
	local bkg = self:GetControlObject("Background")
	if not bkg then
		return
	end
	bkg:SetSrcColor("EBF1F9")
	bkg:SetDestColor("EBF1F9")
end

function OnMouseLeave(self, x, y)
	local bkg = self:GetControlObject("Background")
	if not bkg then
		return
	end
	local l, t, r, b = self:GetObjPos()
	local w, h = r-l, b-t
	if x > 0 and y > 0 and x < w and y < h then return end
	local attr = self:GetAttribute()
	if attr.select then return end
	bkg:SetSrcColor("system.white")
	bkg:SetDestColor("system.white")
end

function OnInitControl(self)
	local attr = self:GetAttribute()
	local bkg = self:GetControlObject("Background")
	if attr.select then
		if bkg then
			bkg:SetSrcColor("EBF1F9")
			bkg:SetDestColor("EBF1F9")
		end
	else
		if bkg then
			bkg:SetSrcColor("system.white")
			bkg:SetDestColor("system.white")
		end
	end
end

function GetTwoLenString(str)
	if string.len(str) == 1 then
		str = "0"..str
	end
	return str
end

function GetWeekDayString(idx)
	local tDayWeekMap = {[2]="周一", [3]="周二", [4]="周三", [5]="周四", [6]="周五", [7]="周六", [1]="周日"}
	return tDayWeekMap[tonumber(idx)]
end

function SetDataHead(self, data)
	local attr = self:GetAttribute() 
	attr.data = data
	local LYear, LMonth, LDay, LHour, LMinute, LSecond, LWeek = tipUtil:Seconds2DateTime(data[1])
	local dayctrl = self:GetControlObject("HeadDay")
	local strDay = GetTwoLenString(tostring(LDay))
	dayctrl:SetText(strDay)
	local strDateMain = tostring(LYear).."年"..GetTwoLenString(tostring(LMonth)).."月  "..GetWeekDayString(LWeek)
	local datectrl = self:GetControlObject("DateAndWeek")
	datectrl:SetText(strDateMain)
end

function SetDataChild(self, data)
	local attr = self:GetAttribute() 
	attr.data = data
	local LYear, LMonth, LDay, LHour, LMinute, LSecond, LWeek = tipUtil:FormatCrtTime(data["createtime"])
	--local HourSecond = self:GetControlObject("HourSecond")
	--HourSecond:SetText(GetTwoLenString(tostring(LHour))..":"..GetTwoLenString(tostring(LMinute)))
	local titlectrl = self:GetControlObject("TitleText")
	titlectrl:SetText(data["title"])
	local timertext = self:GetControlObject("BottomTimerText")
	timertext:SetText(data["type"])
	if data.ischeck then
		local ckctrl = self:GetControlObject("ForegroundCheckBox")
		local ckctrlattr = ckctrl:GetAttribute()
		ckctrlattr.ischeck = true
		ckctrlattr.NormalBkgID = "check2"
		ckctrlattr.HoverBkgID = "check2"
		ckctrlattr.DownBkgID = "check2"
		ckctrlattr.DisableBkgID  = "check2"
		ckctrl:Updata()
	end
	--如果是选中的，界面CHECKBOX需要置为选中状态
	if gSelectData == data then
		local ckctrl = self:GetControlObject("ForegroundCheckBox")
		local ckctrlattr = ckctrl:GetAttribute()
		--ckctrlattr.ischeck = true
		ckctrlattr.NormalBkgID = "check2"
		ckctrlattr.HoverBkgID = "check2"
		ckctrlattr.DownBkgID = "check2"
		ckctrlattr.DisableBkgID  = "check2"
		ckctrl:Updata()
	end
end

function OnClickCheckBox(self)
	local attr = self:GetAttribute() 
	local normal, hover, down, disable = "uncheck", "uncheck", "uncheck", "uncheck"
	local owner = self:GetOwnerControl()
	local owattr = owner:GetAttribute()
	if attr.ischeck then
		attr.ischeck = false
		owattr.data.ischeck = false
		if gSelectData == owattr.data then
			gSelectData = nil
		end
	elseif gSelectData == owattr.data then
		gSelectData = nil
	else
		attr.ischeck = true
		owattr.data.ischeck = true
		normal, hover, down, disable = "check2", "check2", "check2", "check2"
	end
	attr.NormalBkgID = normal
	attr.HoverBkgID = hover
	attr.DownBkgID = down
	attr.DisableBkgID  = disable
	self:Updata()
	CheckIsAllUnCheck(self)
end

function CheckIsAllUnCheck(self)
	local owner = self:GetOwnerControl()
	local parent = owner:GetParent()
	local rootctrl = owner:GetOwnerControl()
	local delctrl = rootctrl:GetControlObject("HeadDeleteBtn")
	local attr = delctrl:GetAttribute()
	if IsAllUnCheck(parent) then
		attr.NormalBkgID = "del.hover"
		attr.HoverBkgID = "del.hover2"
		attr.DownBkgID = "del.hover2"
		attr.DisableBkgID  = "del.hover"
	else
		attr.NormalBkgID = "del.normal"
		attr.HoverBkgID = "del.normal2"
		attr.DownBkgID = "del.normal2"
		attr.DisableBkgID  = "del.normal"
	end
	delctrl:Updata()
end

function ControlUpdateDelBtnState(self)
	local layout = self:GetControlObject("BottomListLayout")
	local delctrl = self:GetControlObject("HeadDeleteBtn")
	local attr = delctrl:GetAttribute() 
	if IsAllUnCheck(layout) then
		attr.NormalBkgID = "del.hover"
		attr.HoverBkgID = "del.hover2"
		attr.DownBkgID = "del.hover2"
		attr.DisableBkgID  = "del.hover"
	else
		attr.NormalBkgID = "del.normal"
		attr.HoverBkgID = "del.normal2"
		attr.DownBkgID = "del.normal2"
		attr.DisableBkgID  = "del.normal"
	end
	delctrl:Updata()
end

function IsAllUnCheck(layout)
	if gSelectData ~= nil then return false end
	local nCount = layout:GetChildCount()
	FunctionObj.TipLog("IsAllUnCheck, nCount = "..tostring(nCount))
	local obj
	local bRet = true
	for i=1, nCount do
		obj = layout:GetChildByIndex(i-1)
		FunctionObj.TipLog("IsAllUnCheck, obj:GetClass() = "..tostring(obj:GetClass()))
		if obj:GetClass() == "TimeLineListItemChildEx" then
			local checkobj = obj:GetControlObject("ForegroundCheckBox")
			local attr = checkobj:GetAttribute()
			if attr.ischeck then
				bRet = false
				break
			end
		end
	end
	return bRet
end

function OnClickDel(self)
	local rootctrl = self:GetOwnerControl()
	local parent = rootctrl:GetControlObject("BottomListLayout") 
	FunctionObj.TipLog("OnClickDel, type(parent) = "..tostring(type(parent)))
	if not IsAllUnCheck(parent) then--有选中的时候
		FunctionObj.TipLog("OnClickDel, IsAllUnCheck return false")
		local SetLoseFocusNoHideFlag = XLGetGlobal("SetLoseFocusNoHideFlag") 
		FunctionObj.TipLog("OnClickDel, type of SetLoseFocusNoHideFlag is "..type(SetLoseFocusNoHideFlag))
		SetLoseFocusNoHideFlag(true)
		FunctionObj.ShowDeleteNotepadRemindWnd("remind", function()
			DeleteData()
			ReBuildList(rootctrl)
			--删完恢复显示状态
			--[[local attr = self:GetAttribute()
			attr.NormalBkgID = "del.normal2"
			attr.HoverBkgID = "del.hover2"
			attr.DownBkgID = "del.normal2"
			attr.DisableBkgID  = "del.normal2"
			self:Updata()]]--
		end)
	end
end

function ReportData()
	local tStatInfo = {}
	
	local bRet, strSource = FunctionObj.GetCommandStrValue("/sstartfrom")
	tStatInfo.strEL = strSource or ""
	tStatInfo.strEC = "addremind"  --增加提醒项
	tStatInfo.strEA = FunctionObj.GetMinorVer() or ""
	tStatInfo.strEV = 1
	FunctionObj.TipConvStatistic(tStatInfo)
end

--父节点的+号按钮
function OnClickAddBtn(self)
	local owner = self:GetOwnerControl()
	local oattr = owner:GetAttribute()
	local data = oattr.data
	table.insert(data[2], 1, {["title"] = "新建提醒", ["createtime"] = tipUtil:GetCurrentUTCTime(), ["type"]="准时提醒",["ntype"]=1,["bopen"]=false})
	data[2]["expand"] = false
	SaveRemindListData2File()
	OnLButtonDown(owner)
	ReportData()
end

--顶部大+号按钮
function OnClickHeadAddBtn(self)
	local nCurUTC = tipUtil:GetCurrentUTCTime()
	local LYear, LMonth, LDay = tipUtil:FormatCrtTime(nCurUTC)
	local y, m, d
	local data = nil
	for utc, v in pairs(tRemindListData) do
		y, m, d = tipUtil:FormatCrtTime(utc)
		if y == LYear and LMonth == m and LDay == d then
			data = v
			break
		end
	end
	
	if data == nil then
		local t = {}
		t.expand = true
		t[1] = {}
		t[1].title="新建提醒"
		t[1].createtime = tipUtil:GetCurrentUTCTime()
		t[1].type = "准时提醒"
		t[1].ntype=1
		t[1].bopen = false
		gSelectData = t[1]
		tRemindListData[tostring(nCurUTC)] = t
	else
		data["expand"] = true
		local t = {}
		t.title="新建提醒"
		t.createtime = tipUtil:GetCurrentUTCTime()
		t.type = "准时提醒"
		t.ntype=1
		t.bopen = false
		table.insert(data, 1, t)
		gSelectData = t
	end
	SaveRemindListData2File()
	local owner = self:GetOwnerControl()
	ReportData()
	ReBuildList(owner)
end

	
function OnInitTimeLineListCtrl(self)
	LoadRemindListData()
	ReBuildList(self)
end

function DeleteData()
	local tNeedDel = {}
	for k, v in pairs(tRemindListData) do
		if #v == 0 then
			tNeedDel[#tNeedDel+1] = k
		else
			local i = 1
			while true do
				if not v[i] then break end
				if v[i].ischeck then
					if gSelectData == v[i] then--删除了当前选中的data
						gSelectData = nil 
					end
					table.remove(v, i)
				elseif gSelectData == v[i] then--当前选择的必定要被删除
					gSelectData = nil
					table.remove(v, i)
				else
					i = i + 1
				end
			end
			if #v == 0 then
				tNeedDel[#tNeedDel+1] = k
			end
		end
	end
	for i, v in ipairs(tNeedDel) do
		tRemindListData[v] = nil
	end
	
	SaveRemindListData2File()
end

function GetSortKey()
	local tSort = {}
	for createtime, _ in pairs(tRemindListData) do
		tSort[#tSort+1] = tonumber(createtime)
	end
	table.sort(tSort, function(a, b) return a > b end)
	return tSort
end

function ReBuildList(self, fdata)
	local attr = self:GetAttribute()
	attr.prevobj = nil
	attr.cursel = nil
	if fdata then
		gSelectData = fdata
	end
	local layout = self:GetControlObject("BottomListLayout")
	layout:RemoveAllChild()
	local tKeys = GetSortKey()
	for i=1, #tKeys do
		 CreateNode(self, {tostring(tKeys[i]), tRemindListData[tostring(tKeys[i])]})
	end
	if attr.cursel then
		local selattr = attr.cursel:GetAttribute()
		local datasel = selattr.data
		if type(datasel[2]) == "table" then
			self:FireExtEvent("OnSelect", datasel[2][1])
		else
			self:FireExtEvent("OnSelect", datasel)
		end
	else
		self:FireExtEvent("OnSelect", nil)
	end
	ControlUpdateDelBtnState(self)
	SaveRemindListData2File()
	ResetScrollBar(self)
end
XLSetGlobal("ReBuildRemindList", ReBuildList)

function CreateNode(self, data)
	local objFactory = XLGetObject("Xunlei.UIEngine.ObjectFactory")
	local layout = self:GetControlObject("BottomListLayout")
	local attr = self:GetAttribute()
	local t = 0
	if attr.prevobj then
		local _,_,_,b0 = (attr.prevobj):GetObjPos()
		t = b0+1
	end
	local newNode
	if type(data[2]) ~= "table" then
		newNode = objFactory:CreateUIObject("S"..data["createtime"]..data["idx"], "TimeLineListItemChildEx")
	else
		newNode = objFactory:CreateUIObject("S"..data[1], "TimeLineListItemHeadEx")
	end
	--默认选择第一个非父节点
	if attr.cursel == nil and gSelectData == nil and not data[2] then
		attr.cursel = newNode
		local attr2 = newNode:GetAttribute()
		attr2.select = true
		gSelectData =  data
	end
	--是否选择
	if gSelectData ~= nil and (gSelectData == data) then
		attr.cursel = newNode
		local attr2 = newNode:GetAttribute()
		attr2.select = true
	end
	newNode:SetData(data)
	layout:AddChild(newNode)
	newNode:SetObjPos(0, t, "father.width", t+44)
	attr.prevobj = newNode
	AddLine(self)
	if type(data[2]) == "table" and data[2]["expand"] then
		for i, v in ipairs(data[2]) do
			v["createtime"] = v["createtime"] or data[1]
			v["idx"] = i
			CreateNode(self, v)
		end
	end
end

function AddLine(self)
	local objFactory = XLGetObject("Xunlei.UIEngine.ObjectFactory")
	local layout = self:GetControlObject("BottomListLayout")
	local attr = self:GetAttribute()
	if not attr.prevobj then return end
	local _,_,_,b0 = (attr.prevobj):GetObjPos()
	local t = b0
	local lineobj = objFactory:CreateUIObject("", "TextureObject")
	lineobj:SetTextureID("line2")
	lineobj:SetObjPos(0, t, "father.width", t+1)
	layout:AddChild(lineobj)
end

function ResetScrollBar(objRootCtrl)
	if objRootCtrl == nil then
		return false
	end
	local objScrollBar = objRootCtrl:GetControlObject("listbox.vscroll")
	if objScrollBar == nil then
		return false
	end
	
	local fatherctrl = 	objRootCtrl:GetControlObject("BottomPanle")
	if fatherctrl == nil then
		return false
	end
	local attr = objRootCtrl:GetAttribute()
	if not attr.prevobj then
		objScrollBar:SetVisible(false)
		objScrollBar:SetChildrenVisible(false)
		return false
	end
	local _,_,_,lastbottom = attr.prevobj:GetObjPos()
	local l, t, r, b = fatherctrl:GetObjPos()
	if lastbottom > b-t then
		objScrollBar:SetScrollRange( 0, lastbottom-b+t, true )
		objScrollBar:SetPageSize(b-t, true)
		objScrollBar:SetVisible(true)
		objScrollBar:SetChildrenVisible(true)
		objScrollBar:Show(true)
		OnScrollMousePosEvent(objScrollBar)
	else
		objScrollBar:SetScrollPos(0, true)	
		objScrollBar:SetVisible(false)
		objScrollBar:SetChildrenVisible(false)
		MoveItemListPanel(objRootCtrl, 0)
		return true
	end
	return true
end

function MoveItemListPanel(objRootCtrl, nScrollPos)
	if not objRootCtrl then
		return
	end
	
	local objContainer = objRootCtrl:GetControlObject("BottomListLayout")
	if not objContainer then
		return
	end
	
	local nL, nT, nR, nB = objContainer:GetObjPos()
	local nHeight = nB-nT
	local nNewT = 0-nScrollPos
	
	objContainer:SetObjPos(nL, nNewT, nR, nNewT+nHeight)
end

function OnScrollBarMouseWheel(self, name, x, y, distance)
	local objRootCtrl = self:GetOwnerControl()
	local nScrollPos = self:GetScrollPos()	
    if distance > 0 then
		self:SetScrollPos( nScrollPos - 44, true )
    else		
		self:SetScrollPos( nScrollPos + 44, true )
    end

	local nNewScrollPos = self:GetScrollPos()
	MoveItemListPanel(objRootCtrl, nNewScrollPos)
	return true	
end

function OnScrollMousePosEvent(self)
	local objRootCtrl = self:GetOwnerControl()
	local nScrollPos = self:GetScrollPos()
	
	MoveItemListPanel(objRootCtrl, nScrollPos)
end

function OnVScroll(self, fun, _type, pos)
	local objRootCtrl = self:GetOwnerControl()
	local nScrollPos = self:GetScrollPos()	
	--点击向上按钮或上方空白
    if _type ==1 then
        self:SetScrollPos( nScrollPos - 44, true )
	--点击向下按钮或下方空白
    elseif _type==2 then
		self:SetScrollPos( nScrollPos + 44, true )
    end

	local nNewScrollPos = self:GetScrollPos()
	MoveItemListPanel(objRootCtrl, nNewScrollPos)
	return true
end