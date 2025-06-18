I've used macros to setup paths, ensuring this should work regardless of where you save it
Output has been set to build debug versions directly in your UnrealVRMod/UEVR/Plugins global plugin dir for rapid testing
Release will build to a folder named after the project within the same directory 
Of course be mindful to not leave old WIP plugins that may crash your game

The ImGui overlay included here is completely separate both from the UEVR C++ overlay and the lua API
There's not really a way to directly integrate them without changes to UEVR itself 
However you can make your overlay interact with lua overlays indirectly 
You'll notice a couple changes to Plugin.hpp, notably a more direct function to dispatch lua which has an overload available 
Taking strings instead of const char* arrays. You can easily overload it with anything else useful if you want more
specific behavior. Dllmain also now gives you the handle to our own plugin so you dont need to call getmodulehandle
if you need the handle for a startup thread, (e.g. you could access the filesystem before UEVR starts up), 
and you also get an on_detach event although you already could do similar with on_device_reset
And the last addition in this iteration are the widen and narrow string helpers, although you can generally just use wstring
Or you could pass responsibility to lua and let praydog's api take care of string handling

Interacting with Lua can be cool but it may be hard to know when to use one or the other. Generally I think if 
you're comfy writing in C++ you may as well do so. Personally I don't know many lua methods 
so even though its easy I feel hamstringed. Remember that lua was originally a configuration language
designed for non-coders so if you have any logic you want to expose to users and other modders you can do it in lua
You may also come to enjoy (or not) lua's table api. You could for example use lua for some text processing 
Or use lua in lieu of a json library. Examples below show how lua handles merging multiple json tables automatically (credit dabinn)
and further below there is an example of sending data from c++ to lua and receiving some back


-- ds-uevr-universal-freecam config 
https://github.com/dabinn/UEVR-Universal-Free-Camera/releases
```
local cfg={}

-- Available button codes:
-- A, B, X, Y 
-- LB, RB, L3, R3 (LT, RT are not implemented yet)
-- DPadUp, DPadDown, DPadLeft, DPadRight
-- Back, Start
-- To specify a button combination, use the "+" symbol. For example: "Select+Y"
-- To specify a special event (pressed, held, released, doubleclick), use these words separated by `_` with the button name.
-- For example: `L3_held`, `Select_pressed` 
-- Default event is "released" if not specified.
cfg.buttons = {
    active = "L3_held", -- Activate free camera
    deactive = "L3", -- Deactivate free camera
    resetCam = "R3", -- Reset the camera
    resetAll = "R3_held", -- Reset both the camera and the custom view
    speedIncrease = "RB", -- Increase movement speed
    speedDecrease = "LB", -- Decrease movement speed
    levelFlight = "X", -- Toggle level flight / omni-directional flight mode
    omniFlightWithSpaceControl = "X_held", -- Enable omni-directional flight mode with space control scheme
    followOn = "Y", -- Enable follow mode
    followPositionOnly = "Y_doubleclick", -- Enable follow position only mode
    followOff = "Y_held", -- Disable follow mode (Hold the camera)
    viewCycle = "Back", -- Cycle through saved views
    viewSave = "Back_held", -- Save the current view
    autoGameMenuToggle = "Start", -- Hide the game menu automatically when free camera is enabled
    disable = "B", -- for debug only
}

-- uevr.sdk.callbacks.on_draw_ui(function()
--     if imgui.collapsing_header("Freecam") then

-- end)
-- Speed Settings
cfg.spd={}
cfg.spd[1] = {
    speedTotalStep = 10,
    move_speed_max = 50000, -- cm per second
    move_speed_min = 50,
    rotate_speed_max = 270, -- degrees per second
    rotate_speed_min = 150, -- degrees per second
    currMoveStep = 4,
    currRotStep = 4
}


-- Freecam Parameters
cfg.opt={
    uevrAttachCameraCompatible = false, -- Compatible with UEVR's attached camera feature, affecting the camera offset value in the UEVR interface.
    autoGameMenuToggle = false, -- Disable game GUI when free camera is enabled
    freecamInvertPitch = false, -- Invert the pitch of the free camera
    levelFlight = true, -- The vertical orientation of the camera does not affect the flight altitude.
    recenterVROnCameraReset = true, -- Reset the camera and recenter VR at the same time
}

local cfgmerge = {
    ["buttons"] =cfg.buttons,
    ["spd"] = cfg.spd,
    ["opt"] = cfg.opt
}
print(cfgmerge["spd"][1])

local function save_cfg()
    json.dump_file("freecam.json", cfgmerge, 4)
end

local function load_cfg()
    cfgmerge = json.load_file("freecam.json")
    print(lib.printTable(cfgmerge, 4))
    if cfgmerge["buttons"] ~= cfg.buttons then
        cfg.buttons = cfgmerge["buttons"]
    end
    if cfgmerge["spd"] ~= cfg.buttons then
        cfg.buttons = cfgmerge["spd"]
    end
    if cfgmerge["opt"] ~= cfg.buttons then
        cfg.buttons = cfgmerge["opt"]
    end

end



.json output
{
    "buttons": {
        "active": "L3_held",
        "autoGameMenuToggle": "Start",
        "deactive": "L3",
        "disable": "B",
        "followOff": "Y_held",
        "followOn": "Y",
        "followPositionOnly": "Y_doubleclick",
        "levelFlight": "X",
        "omniFlightWithSpaceControl": "X_held",
        "resetAll": "R3_held",
        "resetCam": "R3",
        "speedDecrease": "LB",
        "speedIncrease": "RB",
        "viewCycle": "Back",
        "viewSave": "Back_held"
    },
    "opt": {
        "autoGameMenuToggle": false,
        "freecamInvertPitch": false,
        "levelFlight": true,
        "recenterVROnCameraReset": true,
        "uevrAttachCameraCompatible": true
    },
    "spd": [
        {
            "currMoveStep": 4,
            "currRotStep": 4,
            "move_speed_max": 50000,
            "move_speed_min": 50,
            "rotate_speed_max": 270,
            "rotate_speed_min": 150,
            "speedTotalStep": 10
        }
    ]
}



    void SendStatesToLua()
    {
        if (playerManager.vehicleType != playerManager.previousVehicleType)
            API::get()->dispatch_lua_event("playerState", playerManager.VehicleTypeToString(playerManager.vehicleType));
        if (cameraController.previousOnFootCameraMode != cameraController.currentOnFootCameraMode)
            API::get()->dispatch_lua_event("onFootCameraMode", cameraController.VehicleCameraModeToString(cameraController.currentVehicleCameraMode));
        if (cameraController.previousVehicleCameraMode != cameraController.currentVehicleCameraMode)
            API::get()->dispatch_lua_event("vehicleCameraMode", cameraController.VehicleCameraModeToString(cameraController.currentVehicleCameraMode));
    }

https://github.com/Holydh/UEVR_GTASADE/blob/main/LUA/GTASADE_LuaEvents.lua
uevr.sdk.callbacks.on_lua_event(function(event_name, event_string)
    if (event_name == "playerState") then
        playerState = event_string
        print("Player State : " .. event_string)
    end
        if (event_name == "onFootCameraMode") then
        vehicleCameraMode = event_string
        print("On Foot Camera Mode : " .. event_string)
    end
    if (event_name == "vehicleCameraMode") then
        vehicleCameraMode = event_string
        print("Vehicle Camera Mode : " .. event_string)
    end
    if (event_name == "playerIsLeftHanded") then
        local value = tonumber(event_string)
        if value and value == math.floor(value) then
            leftHandedMode = value
        end
        print("Left handed mode : " .. tostring(leftHandedMode))
    end
    if (event_name == "leftHandedOnlyWhileOnFoot") then
        if (event_string == "true") then
            leftHandedOnlyWhileOnFoot = true
        else
            leftHandedOnlyWhileOnFoot = false
        end
        print("Left handed only while on foot : " .. tostring(leftHandedOnlyWhileOnFoot))
    end
end)
```