local Helper = XLGetGlobal("Helper")

--data格式即为tPictures格式:{
-- {"szPath"=, "szExt"=, "utcLastWriteTime"=, "uFileSize"=, "uWidth"=, "uHeight"=, "szType"=, "xlhBitmap"=},

function SetData(self, data, index)--返回值为bool，代表是否成功设定 image，设定失败，则需要请求
	local attr = self:GetAttribute()
	if not attr or not "table" == type(data) then
		return
	end
	
	attr.data = data
	-- attr.index = index
	local background = self:GetControlObject("Background")
	local imageObj = self:GetControlObject("Image")
	local gifObj = self:GetControlObject("Image.gif")
	local extTextObj = self:GetControlObject("ExtText")
	local fileNameObj = self:GetControlObject("FileName")
	
	background:SetVisible(true)
	background:SetChildrenVisible(true)
	
	extTextObj:SetText(data.szExt or "")
	if "gif" == data.szExt then
		gifObj:SetVisible(true)
		imageObj:SetVisible(false)
		local XGP_Factory = XLGetObject("Xunlei.XGP.Factory")
		local gif = XGP_Factory:LoadGifFromFile(data.szPath)
		gifObj:SetGif(gif)
		gifObj:Play()
		return true--gif不去申请了
	else
		gifObj:SetVisible(false)
		imageObj:SetVisible(true)
	end
	if data.szPath then
		--提取filename
		local fileName = string.match(data.szPath, ".+[\\/]([^?]+)")
		attr.fileName = fileName
		fileNameObj:SetText(fileName)
	end
	
	if data.xlhBitmap then
		imageObj:SetDrawMode(1)
		imageObj:SetBitmap(data.xlhBitmap)
		--调整宽高比
		if data.uWidth and data.uHeight then
			local imageL, imageT, imageR, imageB = imageObj:GetObjPos()
			local imageWidth = imageR - imageL
			local imageHeight = imageB - imageT
			if data.uWidth/data.uHeight > imageWidth/imageHeight then--图片是矮、宽型的
				imageHeight = math.round((imageWidth*data.uHeight)/data.uWidth)
				imageObj:SetObjPos2(imageL, imageT, imageWidth, imageHeight)
			elseif data.uWidth/data.uHeight < imageWidth/imageHeight then--图片是高、瘦型的
				imageWidth = math.round((imageHeight*data.uWidth)/data.uHeight)
				imageObj:SetObjPos2(imageL, imageT, imageWidth, imageHeight)
			end
		end
		return true
	else
		if data.szExt and "" ~= data.szExt then
			imageObj:SetDrawMode(0)
			imageObj:SetResID("default_icon."..data.szExt)
		end
	end
	return false
end

function GetIndex(self)
	local attr = self:GetAttribute()
	return attr.index
end

function SetIndex(self, index)
	local attr = self:GetAttribute()
	local container = self:GetOwnerControl()
	local containerAttr = container:GetAttribute()
	local lineCount, columnCount, pageCount, picWidth, picHeight = container:GetPageLayout(self)  
	
	
	if attr.index and attr.lineCount and attr.columnCount then
		if attr.index == index and attr.lineCount == lineCount and attr.columnCount == columnCount then
			-- 无需调整位置
			return
		end
	end
	
	attr.index = index
	attr.lineCount = lineCount
	attr.columnCount = columnCount
	
	local left = ((index-1)%columnCount) * (containerAttr.SpaceH + picWidth)
	local top  = (math.floor((index-1)/columnCount)) * (containerAttr.SpaceV + picHeight)
	
	self:SetObjPos2(left, top, picWidth, picHeight)
	LOG("Thumbnail object SetLayout, id: ",self:GetID(), " left: ", left, " top: ", top, " width: ", picWidth, " height: ", picHeight)
end

function GetData(self)
	local attr = self:GetAttribute()
	return attr.data
end

function Clear(self)
	local attr = self:GetAttribute()
	attr.data = nil
	attr.index = nil
	--清除数据之后，整个Thumbnail变为不可见
	local background = self:GetControlObject("Background")
	background:SetVisible(false)
	background:SetChildrenVisible(false)
end

function SetImage(self, tImgInfo)
	local imageObj = self:GetControlObject("Image")
	local backgroundObj = self:GetControlObject("Background")
	local attr = self:GetAttribute()
	attr.data.xlhBitmap = tImgInfo.xlhBitmap
	attr.data.uWidth = tImgInfo.uWidth
	attr.data.uHeight = tImgInfo.uHeight
	attr.data.szType = tImgInfo.szType
	
	imageObj:SetDrawMode(1)
	imageObj:SetBitmap(tImgInfo.xlhBitmap)
	--调整宽高比
	if tImgInfo.uWidth and tImgInfo.uHeight then
		--这里不能用imageObj:GetObjPos,可能imageObj的pos曾经被下面的代码改过
		local imageL, imageT, imageR, imageB = backgroundObj:GetObjPos()
		local imageWidth = imageR - imageL - 4
		local imageHeight = imageB - imageT - 21
		if tImgInfo.uWidth/tImgInfo.uHeight > imageWidth/imageHeight then--图片是矮、宽型的
			local newHeight = math.round((imageWidth*tImgInfo.uHeight)/tImgInfo.uWidth)
			--计算居中的高度
			local newTop    = math.round((imageHeight - newHeight)/2)
			
			LOG("SetImage ", attr.data.szPath, " newHeight: ", newHeight, " newTop: ", newTop)
			imageObj:SetObjPos2(imageL, newTop, imageWidth, newHeight)
		elseif tImgInfo.uWidth/tImgInfo.uHeight < imageWidth/imageHeight then--图片是高、瘦型的
			local newWidth = math.round((imageHeight*tImgInfo.uWidth)/tImgInfo.uHeight)
			local newLeft  = math.round((imageWidth - newWidth)/2)
			imageObj:SetObjPos2(newLeft, imageT, newWidth, imageHeight)
			LOG("SetImage  ",  attr.data.szPath, " newWidth: ", newWidth, " newLeft: ", newLeft)
		end
	end
end

function Select(self, bSelect)
	local attr = self:GetAttribute()
	attr.bSelect = bSelect 
	local bkg = self:GetControlObject("Background")
	if bSelect then
		bkg:SetSrcColor("RGBA(79,196,246,255)")
	else
		bkg:SetSrcColor("RGBA(57,66,100,255)")
	end
end

function OnLButtonUp(self)
	local ownerCtrl = self:GetOwnerControl()
	local bkg = ownerCtrl:GetControlObject("Background")
	local attr = ownerCtrl:GetAttribute()
	if attr.bSelect then
		attr.bSelect = false
		bkg:SetSrcColor("RGBA(57,66,100,255)")
	else
		attr.bSelect = true
		bkg:SetSrcColor("RGBA(79,196,246,255)")
	end
	
	ownerCtrl:FireExtEvent("OnSelect", attr.bSelect)
end

function OnLButtonDbClick(self)
	local ownerCtrl = self:GetOwnerControl()
	local attr = ownerCtrl:GetAttribute()
	local tree = self:GetOwner()
	local thumbnailContainer = tree:GetUIObject("ThumbnailContainerObj")
	local thumbnailContainerAttr = thumbnailContainer:GetAttribute()
	local userData = {}
	userData.tPictures = thumbnailContainerAttr.tPictures
	userData.sPath = thumbnailContainerAttr.sPath  --文件夹
	userData.index = attr.index
	userData.fileName = attr.fileName
	
	Helper:CreateModelessWnd("ImageWnd","ImageWndTree", nil, userData)
end

function OnInitControl(self)
	local attr = self:GetAttribute()
	attr.bSelect = false
	
	--没有绑定数据之前，整个Thumbnail是不可见的
	local background = self:GetControlObject("Background")
	background:SetVisible(false)
	background:SetChildrenVisible(false)
end
