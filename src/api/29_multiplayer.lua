-- CrabeLoader API - Multiplayer Module
-- Overrides the PC Gold Edition offline engine flags and exposes Crabe.Multiplayer

Crabe = Crabe or {}
Game = Game or {}

-- ---------------------------------------------------------------------------
-- 1. Engine Visibility & Privilege Overrides
-- ---------------------------------------------------------------------------

-- In Disney Infinity 3.0 Gold Edition, UI_IsTOGOOfflineGame() returned true,
-- which completely masked "Scn_Pause_OnlineMP" in pausemenu.lua:1019.
-- Forcing this to false unlocks the native Online Multiplayer menus!
_G.UI_IsTOGOOfflineGame = function()
    return false
end

-- Force account privileges & online status to true
_G.IsMultiplayerAllowed = function(playerNum)
    return true
end

_G.IsInviteAllowed = function(playerNum)
    return true
end

_G.IsOnline = function(playerNum)
    return true
end

_G.IsSignedIntoDisneyID = function(playerNum)
    return true
end

_G.IsSignedIntoPlatform = function(playerNum)
    return true
end

_G.IsOnlineContentAllowed = function()
    return true
end

-- Tell the session engine that Quazal Net-Z P2P is active
_G.NETZ = true

-- Additional gates unmasked via KnowledgeExplorer decompiled presentation scripts
_G.IsWaitingForSession = function(playerNum)
    return false
end

_G.GetSocialAuthState = function(service)
    return 2 -- 2 = Authenticated & Ready for friends and party services
end

_G.UI_IsLoggedIn = function()
    return true
end

_G.UI_IsCloudMigrationComplete = function()
    return true
end

_G.CanInviteFriendToGame = function(name)
    return true
end

_G.UGC_IsLegalParentalCheck = function(playerNum)
    return 1, ""
end

_G.UGC_IsLegalContentRequest = function(playerNum)
    return 1, ""
end

-- Track whether an active networked session is currently hosted
_G._mpHostingActive = false
local nativeHosting = _G.UI_HostingNetworkedGame
_G.UI_HostingNetworkedGame = function(playerNum)
    if _G._mpHostingActive then
        return true
    end
    if type(nativeHosting) == "function" then
        return nativeHosting(playerNum)
    end
    return false
end

-- ---------------------------------------------------------------------------
-- 1.1 Hook ClassFactory for OnlineMP_Options on PC Gold Edition
-- ---------------------------------------------------------------------------

local function installClassFactoryHook()
    local cf = (package and package.loaded and package.loaded["ClassFactory"]) or _G.ClassFactory
    if not cf or type(cf.CreateClass) ~= "function" then
        return false
    end
    if cf._mpHooked then
        return true
    end
    cf._mpHooked = true

    local origCreateClass = cf.CreateClass
    cf.CreateClass = function(className, baseClass, isBase)
        local newClass = origCreateClass(className, baseClass, isBase)
        if className == "OnlineMP_Options" then
            local origBuildList = newClass.BuildList
            newClass.BuildList = function(self)
                if origBuildList then
                    origBuildList(self)
                end
                if self.listData then
                    local hasFriends = false
                    local hasInvites = false
                    local hasLock = false
                    local hasViewPlayers = false
                    for _, item in ipairs(self.listData) do
                        if item.id == "Scn_View_Friends" then hasFriends = true end
                        if item.id == "ACT_Invites" then hasInvites = true end
                        if item.id == "Scn_Lock_Game" or item.id == "Scn_Unlock_Game" then hasLock = true end
                        if item.id == "Scn_ViewPlayers" then hasViewPlayers = true end
                    end
                    if not hasFriends then
                        self:AddListButton("Scn_View_Friends")
                    end
                    if not hasInvites then
                        self:AddListButton("ACT_Invites")
                    end
                    if not hasLock then
                        if type(_G.UI_GameIsLocked) == "function" and _G.UI_GameIsLocked() then
                            self:AddListButton("Scn_Unlock_Game")
                        else
                            self:AddListButton("Scn_Lock_Game")
                        end
                    end
                    local numPlayers = type(_G.Players_NumPlayers) == "function" and _G.Players_NumPlayers() or 1
                    local numLocal = type(_G.Players_NumLocalPlayers) == "function" and _G.Players_NumLocalPlayers() or 1
                    if numPlayers > 1 and numPlayers > numLocal and not hasViewPlayers then
                        self:AddListButton("Scn_ViewPlayers")
                    end
                    self:AddListButton("Scn_Pause_OnlineInvites")
                end
            end
        end
        return newClass
    end
    return true
end

if not installClassFactoryHook() then
    local origRequire = _G.require
    if type(origRequire) == "function" then
        _G.require = function(modName)
            local res = origRequire(modName)
            if modName == "ClassFactory" then
                installClassFactoryHook()
            end
            return res
        end
    end
end

-- ---------------------------------------------------------------------------
-- 2. Ergonomic Crabe.Multiplayer API
-- ---------------------------------------------------------------------------

Crabe.Multiplayer = Crabe.Multiplayer or {}

function Crabe.Multiplayer.getStatus()
    local rawFn = Crabe._mpGetStatus
    if type(rawFn) ~= "function" then
        return {
            patchesActive = false,
            patchedCount = 0,
            redirectorActive = false,
            host = "127.0.0.1",
            port = 3000
        }
    end

    local patchesActive, patchedCount, redirectorActive, host, port = rawFn()
    return {
        patchesActive = (patchesActive == 1),
        patchedCount = tonumber(patchedCount) or 0,
        redirectorActive = (redirectorActive == 1),
        host = tostring(host or "127.0.0.1"),
        port = tonumber(port) or 3000
    }
end

function Crabe.Multiplayer.setTargetHost(host, port)
    if type(host) ~= "string" or host == "" then
        error("Crabe.Multiplayer.setTargetHost: host must be a non-empty string", 2)
    end
    port = tonumber(port) or 3000

    local rawFn = Crabe._mpSetTarget
    if type(rawFn) == "function" then
        rawFn(host, port)
        return true
    end
    return false
end

function Crabe.Multiplayer.applyPatches()
    local rawFn = Crabe._mpApplyPatches
    if type(rawFn) == "function" then
        return rawFn() == 1
    end
    return false
end

-- Session management helpers (wrapping Game.* natives)
function Crabe.Multiplayer.isSessionLocked()
    if type(Game.IsSessionLocked) == "function" then
        return Game.IsSessionLocked()
    end
    return false
end

function Crabe.Multiplayer.setSessionLocked(locked)
    if type(Game.SetSessionLocked) == "function" then
        return Game.SetSessionLocked(locked)
    end
    return false
end

function Crabe.Multiplayer.getPlayerCounts()
    if type(Game.PlayerCounts) == "function" then
        return Game.PlayerCounts()
    end
    return { total = 1, localPlayers = 1, max = 4 }
end

function Crabe.Multiplayer.kickPlayer(playerNum)
    if type(Game.KickPlayer) == "function" then
        Game.KickPlayer(playerNum)
        return true
    end
    return false
end

function Crabe.Multiplayer.getNatInfo()
    local rawFn = Crabe._mpGetNatInfo
    if type(rawFn) ~= "function" then
        return {
            upnpAvailable = false,
            portForwarded = false,
            externalIp = "",
            externalPort = 3074,
            localIp = "127.0.0.1",
            internalPort = 3074,
            statusMessage = "UPnP native binding unavailable"
        }
    end

    local upnpAvail, portFwd, extIp, extPort, locIp, intPort, statusMsg = rawFn()
    return {
        upnpAvailable = (upnpAvail == 1),
        portForwarded = (portFwd == 1),
        externalIp = tostring(extIp or ""),
        externalPort = tonumber(extPort) or 3074,
        localIp = tostring(locIp or "127.0.0.1"),
        internalPort = tonumber(intPort) or 3074,
        statusMessage = tostring(statusMsg or "")
    }
end

function Crabe.Multiplayer.triggerPortForward(port, proto)
    port = tonumber(port) or 3074
    proto = tostring(proto or "UDP")
    local rawFn = Crabe._mpTriggerPortForward
    if type(rawFn) == "function" then
        return rawFn(port, proto) == 1
    end
    return false
end


