local hbus = require("tasks.hal.core.hbus"):instance()
local log = require("log").logger("cs.netservicestarter")
local mfgData = require("tasks.mfg.core.mfgdata")
local prefMgr = require("tasks.harmonywebservices.core.preferencemanager")
local session = require("tasks.harmonywebservices.core.session")
local system = require("system")

MSG_NETSERVICE_NEW_ADDRESS = "cs.netservicestarter_new_address"

local ipaddr
local discovery
local ssdpDiscovery
local halConnect
local hbusHttpServerConnect
local ltcpServerConnector

local function logWifiEvent(name, id)
  local usageLog = require("tasks.crashlog.apihandler.usagelog")
  local t = {
    category = "hub connectivity",
    name = name
  }
  if id then
    t.id = id
  end
  usageLog.postAnalyticEvent(t)
end

local function handleEvent(event)
  if event.family ~= "inet" or event.label == "br-lan" then
    return
  end

  if event.msgtype == "newaddr" then
    log.notice(event.label .. ":newaddr", event.address)
    if event.label ~= "lo" then
      session.setIp(event.address)
    end

    local uuid = system.uniqueId()
    local usageLog = require("tasks.crashlog.apihandler.usagelog")
    usageLog.setUniqueId("wifi", uuid)
    logWifiEvent("connect wifi", uuid)

    if ipaddr ~= event.address and event.label ~= "lo" then
      ipaddr = event.address
    end

    if not discovery then
      log.notice("discovery: starting task")
      discovery = system.loadTask("tasks/connectserver/transport/discovery.lua")
    end
    if not ssdpDiscovery and event.label ~= "lo" then
      log.notice("ssdpDiscovery: starting task")
      ssdpDiscovery = system.loadTask("tasks/connectserver/transport/ssdpdiscovery.lua")
    end
    if not halConnect then
      log.notice("starting HAL task")
      halConnect = system.loadTask("tasks/connectserver/transport/halhttpserverconnector.lua")
    end
    if not hbusHttpServerConnect then
      log.notice("starting HBUS over HTTP server task")
      hbusHttpServerConnect = system.loadTask("tasks/connectserver/transport/hbushttpserverconnector.lua")
    end
    if not ltcpServerConnector then
      log.notice("starting LTCP Server task")
      ltcpServerConnector = system.loadTask("tasks/connectserver/transport/ltcpserverconnector.lua")
    end

    if event.label ~= "lo" then
      log.notice("codex cloud suppression active: cloudapi, pubnub, and packagemgr background tasks not started")
      system.broadcastMessageExceptMeNoWarning(MSG_NETSERVICE_NEW_ADDRESS, {
        label = event.label,
        ipaddr = event.address
      })
    end
  elseif event.msgtype == "deladdr" then
    log.notice(event.label .. ":deladdr", event.address)
    if event.label ~= "lo" then
      session.setIp(nil)
      local usageLog = require("tasks.crashlog.apihandler.usagelog")
      logWifiEvent("disconnect wifi", usageLog.getUniqueId("wifi"))
    end
  end
end

if mfgData.hasNetwork == true then
  while not system.isMessageRegistered("config_unload") or not system.isMessageRegistered("get_setup_account") do
    system.yield()
  end
  local netlink = system.netlinkOpen()
  while true do
    system.yieldSocketRecv(netlink)
    local events = netlink:receive()
    if events then
      local processEvent = false
      local newEvent
      for i, event in ipairs(events) do
        log.notice("netlink event", i, event)
        if event.msgtype == "newaddr" then
          newEvent = event
        end
        if event.family == "inet" and event.label ~= "lo" then
          handleEvent(event)
          processEvent = true
          break
        end
      end
      if newEvent and not processEvent then
        handleEvent(newEvent)
      end
    end
  end
  netlink:close()
else
  log.notice("starting HAL task")
  halConnect = system.loadTask("tasks/connectserver/transport/halhttpserverconnector.lua")
  log.notice("starting HBUS over HTTP server task")
  hbusHttpServerConnect = system.loadTask("tasks/connectserver/transport/hbushttpserverconnector.lua")
  log.notice("starting LTCP Server task")
  ltcpServerConnector = system.loadTask("tasks/connectserver/transport/ltcpserverconnector.lua")
end
