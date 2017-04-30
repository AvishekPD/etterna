local update = false
local t = Def.ActorFrame{
	BeginCommand=cmd(queuecommand,"Set";visible,false),
	OffCommand=cmd(bouncebegin,0.2;xy,-500,0;diffusealpha,0),
	OnCommand=cmd(bouncebegin,0.2;xy,0,0;diffusealpha,1),
	SetCommand=function(self)
		self:finishtweening()
		
		if getTabIndex() == 6 then
			self:queuecommand("On")
			self:visible(true)
			update = true
			MESSAGEMAN:Broadcast("UpdateGoals")
		else 
			self:queuecommand("Off")
			update = false
		end
	end,
	TabChangedMessageCommand=cmd(queuecommand,"Set"),
	PlayerJoinedMessageCommand=cmd(queuecommand,"Set"),
}

local frameX = 10
local frameY = 45
local frameWidth = capWideScale(360,400)
local frameHeight = 350
local fontScale = 0.4
local goalsperpage = 9
local distY = 15
local offsetX = 10
local offsetY = 20
local rankingSkillset=0
local goalFilter=1
local rankingWidth = frameWidth-capWideScale(15,50)
local rankingX = capWideScale(20,30)
local rankingY = capWideScale(80,80)
local rankingTitleWidth = (rankingWidth/(3 + 1))
local buttondiffuse = 0
local buttonheight = 10
local goalYspacing = 30
local goalrow2Y = 12
local currentgoalpage = 1
local numgoalpages = 1

if GAMESTATE:IsPlayerEnabled(PLAYER_1) then
	profile = GetPlayerOrMachineProfile(PLAYER_1)
end

local playergoals = profile:GetAllGoals()
local displayindex = {}

t[#t+1] = Def.Quad{InitCommand=cmd(xy,frameX,frameY;zoomto,frameWidth,frameHeight;halign,0;valign,0;diffuse,color("#333333CC"))}
t[#t+1] = Def.Quad{InitCommand=cmd(xy,frameX,frameY;zoomto,frameWidth,offsetY;halign,0;valign,0;diffuse,getMainColor('frames');diffusealpha,0.5)}
t[#t+1] = LoadFont("Common Normal")..{InitCommand=cmd(xy,frameX+5,frameY+offsetY-9;zoom,0.6;halign,0;diffuse,getMainColor('positive');settext,"Goal Tracker (WIP)")}

-- prolly a clever way to cut this down to like 5 lines - mina
local function filterDisplay (playergoals)
	local index = {}
	if goalFilter == 2 then
		for i=1,#playergoals do
			if playergoals[i]:IsAchieved() then
				index[#index+1] = i
			end
		end
		return index
	elseif goalFilter == 3 then
		for i=1,#playergoals do
			if not playergoals[i]:IsAchieved() then
				index[#index+1] = i
			end
		end
		return index
	end
	for i=1,#playergoals do
		index[#index+1] = i
	end
	return index
end

local function datetimetonumber(x)
	x = x:gsub(" ", ""):gsub("-", ""):gsub(":", "")
	return tonumber(x)
end

local function sgDateComparator(sgA,sgB)
	local a = datetimetonumber(sgA:WhenAssigned())
	local b = datetimetonumber(sgB:WhenAssigned())
	return a > b 
end

local function byAchieved(scoregoal)
	if not scoregoal or scoregoal:IsAchieved() then
		return getMainColor('positive')
	end
	return byDifficulty("Difficulty_Edit")
end

local function datetimetodate(datetime)
	local i = datetime:find(" ")
	return datetime:sub(1,i)
end

local r = Def.ActorFrame{
	UpdateGoalsMessageCommand=function(self)
		playergoals = profile:GetAllGoals()
		table.sort(playergoals,sgDateComparator)
		displayindex = filterDisplay(playergoals)
		numgoalpages = notShit.ceil(#displayindex/goalsperpage)
	end
}

-- need to handle visibility toggle better... cba now -mina
local function makescoregoal(i)
	local sg		-- scoregoal object -mina
	local ck		-- multiple things need this so just have the first actor update it
	local goalsong	-- screw it do all the things -mina
	local goalsteps
	
	local t = Def.ActorFrame{
		InitCommand=cmd(xy,frameX+rankingX,frameY+rankingY+220),
		-- Nest actor frames for each goal so we can control spacing from a single point -mina
		Def.ActorFrame{
			InitCommand=function(self)
				self:y((goalsperpage-i)*-goalYspacing)
			end,
			Def.Quad{InitCommand=cmd(xy,-16,20;zoomto,frameWidth-10,goalYspacing-2;halign,0;valign,1;diffuse,getMainColor('frames');diffusealpha,0.35)},
			LoadFont("Common Large") .. {
				InitCommand=cmd(xy,-14,goalrow2Y;halign,0;zoom,0.25;diffuse,getMainColor('positive');maxwidth,56),
				SetCommand=function(self)
					if update then 
						sg = playergoals[displayindex[i+( (currentgoalpage - 1) *goalsperpage)]]
						if sg then
							-- should do this initialization better -mina
							ck = sg:GetChartKey()
							goalsong = SONGMAN:GetSongByChartKey(ck)
							goalsteps = SONGMAN:GetStepsByChartKey(ck)
							
							local diff = goalsteps:GetDifficulty()
							self:settext(getShortDifficulty(diff))
							self:diffuse(byDifficulty(diff))
							self:visible(true)
						else
							self:visible(false)
						end
					end
				end,
				UpdateGoalsMessageCommand=cmd(queuecommand,"Set")
			},
			LoadFont("Common Large") .. {
				InitCommand=cmd(x,30;halign,0;zoom,0.25;diffuse,getMainColor('positive');maxwidth,600),
				SetCommand=function(self)
					if update then
						if sg then 
							self:settextf(goalsong:GetDisplayMainTitle())
							self:visible(true)
						else
							self:visible(false)
						end
						self:diffuse(byAchieved(sg))
					end
				end,
				UpdateGoalsMessageCommand=cmd(queuecommand,"Set")
			},
			Def.Quad{
				InitCommand=cmd(x,30;zoomto,150,buttonheight;halign,0;diffuse,getMainColor('frames');diffusealpha,buttondiffuse),
				MouseLeftClickMessageCommand=function(self)
					if update and sg then 
						if isOver(self) then
							local whee = SCREENMAN:GetTopScreen():GetMusicWheel()
							whee:SelectSong(goalsong)
							GAMESTATE:GetSongOptionsObject('ModsLevel_Preferred'):MusicRate(sg:GetRate())
							GAMESTATE:GetSongOptionsObject('ModsLevel_Song'):MusicRate(sg:GetRate())
							GAMESTATE:GetSongOptionsObject('ModsLevel_Current'):MusicRate(sg:GetRate())
							MESSAGEMAN:Broadcast("GoalSelected")
						end
					end
				end
			},
			LoadFont("Common Large") .. {
				InitCommand=cmd(halign,0;zoom,0.25;diffuse,getMainColor('positive');maxwidth,80),
				SetCommand=function(self)
					if update then 
						if sg then
							local ratestring = string.format("%.2f", sg:GetRate()):gsub("%.?0+$", "").."x"
							self:settextf(ratestring)
							self:visible(true)
						else
							self:visible(false)
						end
						self:diffuse(byAchieved(sg))
					end
				end,
				UpdateGoalsMessageCommand=cmd(queuecommand,"Set")
			},
			Def.Quad{
				InitCommand=cmd(halign,0;zoomto,28,buttonheight;diffuse,getMainColor('positive');diffusealpha,buttondiffuse),
				MouseLeftClickMessageCommand=function(self)
					if isOver(self) and update then
						sg:SetRate(sg:GetRate()+0.1)
						MESSAGEMAN:Broadcast("UpdateGoals")
					end
				end,
				MouseRightClickMessageCommand=function(self)
					if isOver(self) and update then
						sg:SetRate(sg:GetRate()-0.1)
						MESSAGEMAN:Broadcast("UpdateGoals")
					end
				end
			},
			LoadFont("Common Large") .. {
				InitCommand=cmd(xy,-6,goalrow2Y;halign,0;zoom,0.2;diffuse,getMainColor('positive')),
				SetCommand=function(self)
					if update then 
						if sg then 
							self:settextf("%5.f%%", sg:GetPercent())
							self:visible(true)
						else
							self:visible(false)
						end
						self:diffuse(byAchieved(sg))
					end
				end,
				UpdateGoalsMessageCommand=cmd(queuecommand,"Set")
			},
			LoadFont("Common Large") .. {
				InitCommand=cmd(xy,30,goalrow2Y;halign,0;zoom,0.2;diffuse,getMainColor('positive')),
				SetCommand=function(self)
					if update then 
						if sg then 
							local pb = sg:GetCurrentPB()
							if pb then
								self:settextf("(Best: %5.2f%%)", pb:GetWifeScore() * 100)
								self:diffuse(getGradeColor(pb:GetWifeGrade()))
								self:visible(true)
							else
								self:settextf("(Best: %5.2f%%)", 0)
								self:diffuse(byAchieved(sg))
							end
						else
							self:visible(false)
						end
					end
				end,
				UpdateGoalsMessageCommand=cmd(queuecommand,"Set")
			},
			Def.Quad{
				InitCommand=cmd(y,goalrow2Y;halign,0;zoomto,25,buttonheight;diffuse,getMainColor('positive');diffusealpha,buttondiffuse),
				MouseLeftClickMessageCommand=function(self)
					if isOver(self) and update then
						sg:SetPercent(sg:GetPercent()+1)
						MESSAGEMAN:Broadcast("UpdateGoals")
					end
				end,
				MouseRightClickMessageCommand=function(self)
					if isOver(self) and update then
						sg:SetPercent(sg:GetPercent()-1)
						MESSAGEMAN:Broadcast("UpdateGoals")
					end
				end
			},
			LoadFont("Common Large") .. {
				InitCommand=cmd(xy,300,goalrow2Y;halign,0;zoom,0.2;diffuse,getMainColor('positive');maxwidth,80),
				SetCommand=function(self)
					if update then 
						if sg then
							local msd = goalsteps:GetMSD(sg:GetRate(), 1)
							self:settextf("%5.1f", msd)
							self:diffuse(ByMSD(msd))
							self:visible(true)
						else
							self:visible(false)
						end
					end
				end,
				UpdateGoalsMessageCommand=cmd(queuecommand,"Set")
			},
			LoadFont("Common Large") .. {
				InitCommand=cmd(x,200;halign,0;zoom,0.2;diffuse,getMainColor('positive');maxwidth,800),
				SetCommand=function(self)
					if update then
						if sg then
							self:settext("Assigned: "..datetimetodate(sg:WhenAssigned()))
							self:visible(true)
						else
							self:visible(false)
						end
						self:diffuse(byAchieved(sg))
					end
				end,
				UpdateGoalsMessageCommand=cmd(queuecommand,"Set")
			},
			LoadFont("Common Large") .. {
				InitCommand=cmd(xy,200,goalrow2Y;halign,0;zoom,0.2;diffuse,getMainColor('positive');maxwidth,800),
				SetCommand=function(self)
					if update then 
						if sg and sg:IsAchieved() then 
							self:settext("Achieved: "..datetimetodate(sg:WhenAchieved()))
							self:visible(true)
						else
							self:visible(false)
						end
						self:diffuse(byAchieved(sg))
					end
				end,
				UpdateGoalsMessageCommand=cmd(queuecommand,"Set")
			},
			LoadFont("Common Large") .. {
				InitCommand=cmd(x,-12;halign,0;zoom,0.25;diffuse,getMainColor('positive');maxwidth,160),
				SetCommand=function(self)
					if update then 
						if sg then 
							self:settextf(sg:GetPriority())
							self:visible(true)
						else
							self:visible(false)
						end
						self:diffuse(byAchieved(sg))
					end
				end,
				UpdateGoalsMessageCommand=cmd(queuecommand,"Set")
			},
			Def.Quad{
				InitCommand=cmd(x,-16;halign,0;zoomto,16,buttonheight;diffuse,getMainColor('positive');diffusealpha,buttondiffuse),
				MouseLeftClickMessageCommand=function(self)
					if isOver(self) and update then
						sg:SetPriority(sg:GetPriority()+1)
						MESSAGEMAN:Broadcast("UpdateGoals")
					end
				end,
				MouseRightClickMessageCommand=function(self)
					if isOver(self) and update then
						sg:SetPriority(sg:GetPriority()-1)
						MESSAGEMAN:Broadcast("UpdateGoals")
					end
				end
			},
			Def.Quad{
				InitCommand=cmd(x,325;halign,0;zoomto,4,4;diffuse,byJudgment('TapNoteScore_Miss');diffusealpha,1),
				MouseLeftClickMessageCommand=function(self)
					if isOver(self) and update and sg then
						sg:Delete()
						MESSAGEMAN:Broadcast("UpdateGoals")
					end
				end,
			}
		}
	}
	return t
end



local fawa = {"All Goals","Completed","Incomplete"}
local function filterButton(i)
	local t = Def.ActorFrame{
		Def.Quad{
		InitCommand=cmd(xy,20+frameX+rankingX+(i-1+i*(1/(1+3)))*rankingTitleWidth,frameY+rankingY-60;zoomto,rankingTitleWidth,30;halign,0.5;valign,0;diffuse,getMainColor('frames');diffusealpha,0.35),
		SetCommand=function(self)
			if i == goalFilter then
				self:diffusealpha(1)
			else
				self:diffusealpha(0.35)
			end
		end,
		MouseLeftClickMessageCommand=function(self)
			if isOver(self) then
				goalFilter = i
				MESSAGEMAN:Broadcast("UpdateGoals")
			end
		end,
		UpdateGoalsMessageCommand=cmd(queuecommand,"Set"),
		},
		LoadFont("Common Large") .. {
			InitCommand=cmd(xy,20+frameX+rankingX+(i-1+i*(1/(1+3)))*rankingTitleWidth,frameY+rankingY-45;halign,0.5;diffuse,getMainColor('positive');maxwidth,rankingTitleWidth;maxheight,25),
			BeginCommand=function(self)
				self:settext(fawa[i])
			end,
		}
	}
	return t
end

-- should actor frame prev/next/page displays to prevent redundancy - mina
r[#r+1] = Def.ActorFrame{
	InitCommand=cmd(xy,frameX+10,frameY+rankingY+250),
	Def.Quad{
		InitCommand=cmd(xy,300,-8;zoomto,40,20;halign,0;valign,0;diffuse,getMainColor('frames');diffusealpha,buttondiffuse),
		MouseLeftClickMessageCommand=function(self)
			if isOver(self) and currentgoalpage < numgoalpages then
				currentgoalpage = currentgoalpage + 1
				MESSAGEMAN:Broadcast("UpdateGoals")
			end
		end
	},
	LoadFont("Common Large") .. {
		InitCommand=cmd(x,300;halign,0;zoom,0.3;diffuse,getMainColor('positive');settext,"Next"),
	},	
	Def.Quad{
		InitCommand=cmd(y,-8;zoomto,65,20;halign,0;valign,0;diffuse,getMainColor('frames');diffusealpha,buttondiffuse),
		MouseLeftClickMessageCommand=function(self)
			if isOver(self) and currentgoalpage > 1 then
				currentgoalpage = currentgoalpage - 1
				MESSAGEMAN:Broadcast("UpdateGoals")
			end
		end
	},
	LoadFont("Common Large") .. {
		InitCommand=cmd(halign,0;zoom,0.3;diffuse,getMainColor('positive');settext,"Previous"),
	},
	LoadFont("Common Large") .. {
		InitCommand=cmd(x,175;halign,0.5;zoom,0.3;diffuse,getMainColor('positive')),
		SetCommand=function(self)
			self:settextf("Showing %i-%i of %i", math.min(((currentgoalpage-1)*goalsperpage)+1, #displayindex), math.min(currentgoalpage*goalsperpage, #displayindex), #displayindex)
		end,
		UpdateGoalsMessageCommand=cmd(queuecommand,"Set"),
	}
}

for i=1,goalsperpage do 
	r[#r+1] = makescoregoal(i)
end
for i=1,3 do
	r[#r+1] = filterButton(i)
end

t[#t+1] = r

return t