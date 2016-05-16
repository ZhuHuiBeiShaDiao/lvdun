local Helper = XLGetGlobal("Helper")

function OnCreate(self)
	local objtree = self:GetBindUIObjectTree()
	local objRootLayout = objtree:GetUIObject("root")
    local nLayoutL, nLayoutT, nLayoutR, nLayoutB = objRootLayout:GetObjPos()
	local nLayoutWidth = nLayoutR - nLayoutL
	local nLayoutHeight = nLayoutB - nLayoutT
	
	local workleft, worktop, workright, workbottom = Helper.tipUtil:GetWorkArea()
	local workWidth = workright - workleft
	local workHeigth = workbottom - worktop
	self:Move( math.floor((workWidth - nLayoutWidth) / 2), math.floor((workHeigth - nLayoutHeight) / 2), nLayoutWidth, nLayoutHeight)

	-- local imageCtrl = objtree:GetUIObject("FrameWnd.ImageCtrl")
	-- imageCtrl:SetImagePath("C:\\Users\\mjt\\Pictures\\Pictures\\test2.png")
	-- local titleCtrl = objtree:GetUIObject("FrameWnd.TitleCtrl")
	-- titleCtrl:SetTitleTextContent("test2.png")
	
	-- local imageCtrl = objtree:GetUIObject("FrameWnd.ImageCtrl")
	-- imageCtrl:AttachListener("OnImageSizeChange", false, OnImageSizeChange)
	
	-- Helper:AddDropTarget(self)
	-- local function OnDrop(file)
		-- imageCtrl:SetImagePath(file)
	-- end
	-- Helper:AddListener("OnDrop", function(_, _, file) OnDrop(file) end)
end

local isLButtonDown = false
function OnLButtonDown_ResizeLine(self, x, y)
	self:SetCaptureMouse(true)
	isLButtonDown = true
end

function OnMouseMove_ResizeLine(self, x, y)
	self:SetCursorID("IDC_SIZEWE")
	if not isLButtonDown then
		return
	end
	
	local owner = self:GetOwner()
	local wnd = owner:GetBindHostWnd()
	local x, y = wnd:ScreenPtToHostWndPt(Helper.tipUtil:GetCursorPos())
	x, y = wnd:HostWndPtToTreePt(x, y)
	
	--最大、最小限制
	if x > 400 or x < 100 then
		-- Helper:Assert(false, "x: "..tostring(x))
		return
	end
	local leftTreeContainer = owner:GetUIObject("MainWnd.LeftTreeContainer")
	local thumbnailContainer = owner:GetUIObject("MainWnd.ThumbnailContainer")
	
	leftTreeContainer:SetObjPos2("0", "30", x, "father.height - 30 - 30")
	self:SetObjPos2(x, "30", "2", "father.height - 30 - 30")
	thumbnailContainer:SetObjPos2(x, "30", "father.width - "..x, "father.height - 30 - 30")
end

function OnLButtonUp_ResizeLine(self, x, y)
	self:SetCursorID("IDC_SIZEWE")
	self:SetCaptureMouse(false)
	isLButtonDown = false
end


function OnShowWindow(self, bVisible)
	
end
