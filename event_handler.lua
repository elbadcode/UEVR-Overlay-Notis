
local api = uevr.api
local duration = 5
local function show_osd_text(text, duration)
    api:dispatch_custom_event("OSD_text", text)
    api:dispatch_custom_event("OSD_show", "")
end



uevr.sdk.callbacks.on_lua_event(
    function(event_name, event_data)
        show_osd_text(event_name)
        show_osd_text(event_data)
        if event_name == "Cool_Event" then
            uevr.api:dispatch_custom_event("Lua_Reply", "Hello_From_Lua")
            show_osd_text(event_data)
            -- do whatever other things you want to do now
        end
    end
)
cfg={
    text_color = "1.0f, 1.0f, 1.0f, 1.0f", -- color is parsed as a string of comma separate values
    bg_color = "1.0f, 1.0f, 1.0f", -- color is parsed as a string of comma separate values
    show_bg = false, -- background alpha is managed separately 
    offset_x = 32,
    offset_y = 32 -- position of osd
}


local function save_cfg()
    json.dump_file("osd.json", cfg, 4)
end

local function load_cfg()
    cfg = json.load_file("cfg.json")
end

local text = ""
uevr.lua.add_script_panel("OSD Settings", function()
    imgui.text("OSD Display Settings")
    if imgui.button("load config") then
        load_cfg()
        api:dispatch_custom_event("OSD_offset_x", tostring(cfg['offset_x']))
        api:dispatch_custom_event("OSD_offset_y", tostring(cfg['offset_y']))
        end
    if imgui.button("save config") then save_cfg() end

    local changed, value, _, _ = imgui.input_text("osdtext", text, 256)
    if changed then text = value end
    print(text)
    if imgui.button("send text") then show_osd_text(text, 5.0) end

end)



show_osd_text("Initializing hello_world.lua", 2.0)

UEVR_UObjectHook.activate()


local uobjects = uevr.types.FUObjectArray.get()



local once = true
local last_world = nil
local last_level = nil

uevr.sdk.callbacks.on_post_engine_tick(function(engine, delta)

end)

local spawn_once = true

uevr.sdk.callbacks.on_pre_engine_tick(function(engine, delta)
    local game_engine_class = api:find_uobject("Class /Script/Engine.GameEngine")
    local game_engine = UEVR_UObjectHook.get_first_object_by_class(game_engine_class)

    local viewport = game_engine.GameViewport
    if viewport == nil then      
        return
    end
    local world = viewport.World

    if world == nil then
        return
    end

    if world ~= last_world then
        

    end

    last_world = world

    local level = world.PersistentLevel

    if level == nil then
      show_osd_text("Level is nil", 2.0)
        return
    end

    if level ~= last_level then
        show_osd_text("Level changed", 1.0)
        show_osd_text("Level name: " .. level:get_full_name(), 1.0)

        local game_instance = game_engine.GameInstance

        if game_instance == nil then
            show_osd_text("GameInstance is nil", 1.0)
            return
        end

        local local_players = game_instance.LocalPlayers

        for i in ipairs(local_players) do
            local player = local_players[i]
            local player_controller = player.PlayerController
            local pawn = player_controller.Pawn
    
            if pawn ~= nil then
                show_osd_text("Pawn: " .. pawn:get_full_name(), 1.0)
                --pawn.BaseEyeHeight = 0.0
                --pawn.bActorEnableCollision = not pawn.bActorEnableCollision

                local actor_component_c = api:find_uobject("Class /Script/Engine.ActorComponent");
                show_osd_text("actor_component_c class: " .. tostring(actor_component_c), 1.0)
                local test_component = pawn:GetComponentByClass(actor_component_c)

                show_osd_text("TestComponent: " .. tostring(test_component), 1.0)

                local controller = pawn.Controller

                if controller ~= nil then
                    show_osd_text("Controller: " .. controller:get_full_name(), 1.0)

                    local velocity = controller:GetVelocity()
                    show_osd_text("Velocity: " .. tostring(velocity.x) .. ", " .. tostring(velocity.y) .. ", " .. tostring(velocity.z), 1.0)

                    local test = Vector3d.new(1.337, 1.0, 1.0)
                    show_osd_text("Test: " .. tostring(test.x) .. ", " .. tostring(test.y) .. ", " .. tostring(test.z), 1.0)

                    controller:SetActorScale3D(Vector3d.new(1.337, 1.0, 1.0))

                    local actor_scale_3d = controller:GetActorScale3D()
                    show_osd_text("ActorScale3D: " .. tostring(actor_scale_3d.x) .. ", " .. tostring(actor_scale_3d.y) .. ", " .. tostring(actor_scale_3d.z), 1.0)

                    
                    local control_rotation = controller:GetControlRotation()

                    show_osd_text("ControlRotation: " .. tostring(control_rotation.Pitch) .. ", " .. tostring(control_rotation.Yaw) .. ", " .. tostring(control_rotation.Roll), 1.0)
                    control_rotation.Pitch = 1.337

                    controller:SetControlRotation(control_rotation)
                    control_rotation = controller:GetControlRotation()

                    show_osd_text("New ControlRotation: " .. tostring(control_rotation.Pitch) .. ", " .. tostring(control_rotation.Yaw) .. ", " .. tostring(control_rotation.Roll), 1.0)
                end

                local primary_actor_tick = pawn.PrimaryActorTick

                if primary_actor_tick ~= nil then
                    show_osd_text("PrimaryActorTick: " .. tostring(primary_actor_tick), 1.0)

                    -- show_osd_text various properties, this is testing of StructProperty as PrimaryActorTick is a struct
                    local tick_interval = primary_actor_tick.TickInterval
                    show_osd_text("TickInterval: " .. tostring(tick_interval), 1.0)

                    show_osd_text("bAllowTickOnDedicatedServer: " .. tostring(primary_actor_tick.bAllowTickOnDedicatedServer), 1.0)
                    show_osd_text("bCanEverTick: " .. tostring(primary_actor_tick.bCanEverTick), 1.0)
                    show_osd_text("bStartWithTickEnabled: " .. tostring(primary_actor_tick.bStartWithTickEnabled), 1.0)
                    show_osd_text("bTickEvenWhenPaused: " .. tostring(primary_actor_tick.bTickEvenWhenPaused), 1.0)
                else
                    show_osd_text("PrimaryActorTick is nil", 1.0)
                end

            end
    
        end

        show_osd_text("Local players: " .. tostring(local_players), 1.0)
    end



    if once then

    end
end)

uevr.sdk.callbacks.on_script_reset(function()
    show_osd_text("Resetting hello_world.lua", 1.0)
    api:dispatch_custom_event("OSD_text_color", "0.0f, 1.0f, 1.0f, 1.0f")

end)

