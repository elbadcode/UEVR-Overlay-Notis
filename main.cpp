/*
This file (Plugin.cpp) is licensed under the MIT license and is separate from the rest of the UEVR codebase.

Copyright (c) 2023 praydog

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include <sstream>
#include <mutex>
#include <memory>
#include <locale>
#include <codecvt>

#include <Windows.h>

// only really necessary if you want to render to the screen
#include "renderlib/uevr_imgui/imgui_impl_dx11.h"
#include "renderlib/uevr_imgui/imgui_impl_dx12.h"
#include "renderlib/uevr_imgui/imgui_impl_win32.h"

#include "renderlib/rendering/d3d11.hpp"
#include "renderlib/rendering/d3d12.hpp"

#include "uevr/Plugin.hpp"
#include "renderlib/imgui/imgui.h"

using namespace uevr;


#define PLUGIN_LOG_ONCE(...) \
    static bool _logged_ = false; \
    if (!_logged_) { \
        _logged_ = true; \
        API::get()->log_info(__VA_ARGS__); \
    }

class OSDTemplate : public uevr::Plugin {
public:
    OSDTemplate() = default;

    void on_dllmain(HMODULE hmod) override {}

    void on_initialize() override {
        ImGui::CreateContext();
    }
    
    void on_present() override {
        std::scoped_lock _{ m_imgui_mutex };

        if (!m_initialized) {
            if (!initialize_imgui()) {
                API::get()->log_info("Failed to initialize imgui");
                return;
            }
            else {
                API::get()->log_info("Initialized imgui");
            }
        }

        const auto renderer_data = API::get()->param()->renderer;

        if (!API::get()->param()->vr->is_hmd_active()) {
            if (!m_was_rendering_desktop) {
                m_was_rendering_desktop = true;
                on_device_reset();
                return;
            }

            m_was_rendering_desktop = true;

            if (renderer_data->renderer_type == UEVR_RENDERER_D3D11) {
                ImGui_ImplDX11_NewFrame();
                g_d3d11.render_imgui();
            }
            else if (renderer_data->renderer_type == UEVR_RENDERER_D3D12) {
                auto command_queue = (ID3D12CommandQueue*)renderer_data->command_queue;

                if (command_queue == nullptr) {
                    return;
                }

                ImGui_ImplDX12_NewFrame();
                g_d3d12.render_imgui();
            }
        }
    }

    void reset_height() {
        auto& api = API::get();
        auto vr = api->param()->vr;
        UEVR_Vector3f origin{};
        vr->get_standing_origin(&origin);

        UEVR_Vector3f hmd_pos{};
        UEVR_Quaternionf hmd_rot{};
        vr->get_pose(vr->get_hmd_index(), &hmd_pos, &hmd_rot);

        origin.y = hmd_pos.y;

        vr->set_standing_origin(&origin);
    }

    void on_device_reset() override {
        PLUGIN_LOG_ONCE("Example Device Reset");

        std::scoped_lock _{ m_imgui_mutex };

        const auto renderer_data = API::get()->param()->renderer;

        if (renderer_data->renderer_type == UEVR_RENDERER_D3D11) {
            ImGui_ImplDX11_Shutdown();
            g_d3d11 = {};
        }

        if (renderer_data->renderer_type == UEVR_RENDERER_D3D12) {
            g_d3d12.reset();
            ImGui_ImplDX12_Shutdown();
            g_d3d12 = {};
        }

        m_initialized = false;
    }

    void on_post_render_vr_framework_dx11(ID3D11DeviceContext* context, ID3D11Texture2D* texture, ID3D11RenderTargetView* rtv) override {
        PLUGIN_LOG_ONCE("Post Render VR Framework DX11");

        const auto vr_active = API::get()->param()->vr->is_hmd_active();

        if (!m_initialized || !vr_active) {
            return;
        }

        if (m_was_rendering_desktop) {
            m_was_rendering_desktop = false;
            on_device_reset();
            return;
        }

        std::scoped_lock _{ m_imgui_mutex };

        ImGui_ImplDX11_NewFrame();
        g_d3d11.render_imgui_vr(context, rtv);
    }

    void on_post_render_vr_framework_dx12(ID3D12GraphicsCommandList* command_list, ID3D12Resource* rt, D3D12_CPU_DESCRIPTOR_HANDLE* rtv) override {
        PLUGIN_LOG_ONCE("Post Render VR Framework DX12");

        const auto vr_active = API::get()->param()->vr->is_hmd_active();

        if (!m_initialized || !vr_active) {
            return;
        }

        if (m_was_rendering_desktop) {
            m_was_rendering_desktop = false;
            on_device_reset();
            return;
        }

        std::scoped_lock _{ m_imgui_mutex };

        ImGui_ImplDX12_NewFrame();
        g_d3d12.render_imgui_vr(command_list, rtv);
    }

    bool on_message(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) override {
        ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam);

        return !ImGui::GetIO().WantCaptureMouse && !ImGui::GetIO().WantCaptureKeyboard;
    }

    ImVec4 parse_color(std::string colorstr) 
        {
        std::string segment;
        std::vector<std::string> parts;
        std::stringstream ss(colorstr);

        while (std::getline(ss, segment, ',')) {
            parts.push_back(segment); 
        }
        ImVec4 outcolor = ImVec4(0.0f,0.0f,0.0f,0.0f);
        // Print the split parts
		outcolor.x = std::stof(parts[0]);
		outcolor.y = std::stof(parts[1]);
		outcolor.z = std::stof(parts[2]);
		outcolor.w = std::stof(parts[3]);
        return outcolor;
        }

    void on_custom_event(const char* event_name, const char* event_data) override {
        API::get()->log_info(event_name);
        API::get()->log_info(event_data);
        if (event_name == "OSD_text_color")
			text_color = parse_color(std::string(event_data));      
        if (event_name == "OSD_bg_color") 
            bg_color = parse_color(std::string(event_data));
		if (event_name == "OSD_show_bg")
			show_bg = (std::string(event_data) == "true");
        if (event_name == "OSD_text")
			osd_text = event_data;
		if (event_name == "OSD_offset_x")
			osd_offset[0] = std::stof(event_data);
        if (event_name == "OSD_offset_y")
            osd_offset[1] = std::stof(event_data);
        if (event_name == "OSD")
        {
            if(strcmp(event_data,"show") == 0)
            {
                show_osd = true;
                lua_overrides = true;
            }
            else if (strcmp(event_data, "hide") == 0)
            {
                show_osd = false;
                lua_overrides = false;
            }
        }              
    }
    
    void dispatch_lua_event(const char* event_name, const char* event_data) override {
		API::get()->dispatch_lua_event(event_name, event_data);
    }
    void send_custom_event(const std::string event_name, std::string event_data) {
		API::get()->dispatch_custom_event(event_name.c_str(), event_data.c_str());
    }

 void on_pre_engine_tick(API::UGameEngine* engine, float delta) override {
        PLUGIN_LOG_ONCE("Pre Engine Tick: %f", delta);

        if (m_initialized) {
            std::scoped_lock _{ m_imgui_mutex };

            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();
            if (GetAsyncKeyState(VK_F3)) {
                show_menu = !show_menu;
            }
            internal_frame();

            ImGui::EndFrame();
            ImGui::Render();
        }
    }

    void on_post_engine_tick(API::UGameEngine* engine, float delta) override {
        PLUGIN_LOG_ONCE("Post Engine Tick: %f", delta);
    }

    void on_pre_slate_draw_window(UEVR_FSlateRHIRendererHandle renderer, UEVR_FViewportInfoHandle viewport_info) override {
        PLUGIN_LOG_ONCE("Pre Slate Draw Window");
    }

    void on_post_slate_draw_window(UEVR_FSlateRHIRendererHandle renderer, UEVR_FViewportInfoHandle viewport_info) override {
        PLUGIN_LOG_ONCE("Post Slate Draw Window");
    }

    void on_pre_calculate_stereo_view_offset(UEVR_StereoRenderingDeviceHandle, int view_index, float world_to_meters,
        UEVR_Vector3f* position, UEVR_Rotatorf* rotation, bool is_double) override
    {
        PLUGIN_LOG_ONCE("Pre Calculate Stereo View Offset");
    }

    void on_post_calculate_stereo_view_offset(UEVR_StereoRenderingDeviceHandle, int view_index, float world_to_meters,
        UEVR_Vector3f* position, UEVR_Rotatorf* rotation, bool is_double)
    {
        PLUGIN_LOG_ONCE("Post Calculate Stereo View Offset");
    }

    void on_pre_viewport_client_draw(UEVR_UGameViewportClientHandle viewport_client, UEVR_FViewportHandle viewport, UEVR_FCanvasHandle canvas) {
        PLUGIN_LOG_ONCE("Pre Viewport Client Draw");
    }

    void on_post_viewport_client_draw(UEVR_UGameViewportClientHandle viewport_client, UEVR_FViewportHandle viewport, UEVR_FCanvasHandle canvas) {
        PLUGIN_LOG_ONCE("Post Viewport Client Draw");
    }

private:
    bool initialize_imgui() {
        if (m_initialized) {
            return true;
        }

        std::scoped_lock _{ m_imgui_mutex };

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        static const auto imgui_ini = API::get()->get_persistent_dir(L"imgui_template_plugin.ini").string();
        ImGui::GetIO().IniFilename = imgui_ini.c_str();

        const auto renderer_data = API::get()->param()->renderer;

        DXGI_SWAP_CHAIN_DESC swap_desc{};
        auto swapchain = (IDXGISwapChain*)renderer_data->swapchain;
        swapchain->GetDesc(&swap_desc);

        m_wnd = swap_desc.OutputWindow;

        if (!ImGui_ImplWin32_Init(m_wnd)) {
            return false;
        }

        if (renderer_data->renderer_type == UEVR_RENDERER_D3D11) {
            if (!g_d3d11.initialize()) {
                return false;
            }
        }
        else if (renderer_data->renderer_type == UEVR_RENDERER_D3D12) {
            if (!g_d3d12.initialize()) {
                return false;
            }
        }

        m_initialized = true;
        return true;
    }



    void osd(std::string text, ImVec2 offset = ImVec2(0, 0), bool show_background = false, ImVec4 textcolor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f), ImVec4 bgcolor = ImVec4(0.11f, 0.11f, 0.11f, 0.7f) ) {
		const auto imgui_io = ImGui::GetIO(); 
		const auto imgui_ctx = ImGui::GetCurrentContext();
        ImGui::SetNextWindowPos(ImVec2(offset[0]+ 4 , offset[1]+4));
        ImGui::SetNextWindowSize(ImVec2(imgui_io.DisplaySize.x - 20.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, textcolor);
		bgcolor.z = show_background ? bgcolor.z : 0.0f;
        ImGui::PushStyleColor(ImGuiCol_WindowBg, bgcolor);
        ImGui::Begin("OSD", nullptr,
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoNav |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoInputs |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoFocusOnAppearing);
        ImGui::TextUnformatted(text.c_str());
        ImGui::End();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar();
    }

    void osd()
    {
		osd(osd_text, osd_offset, show_bg, text_color, bg_color);
    }
    void osd(std::string text)
    {
        osd(text, osd_offset, show_bg, text_color, bg_color);
    }
    void internal_frame() {
        if (show_menu)
        {
            if (ImGui::Begin("Very Cool Template Plugin")) {
                ImGui::Text("Hello from the very cool plugin!");

                static char lua_event_name[256]{};
                ImGui::Text("Send Event to Lua");
                ImGui::InputText("LuaEvent", lua_event_name, sizeof(lua_event_name));
                
                static char lua_event_data[256]{};
                ImGui::InputText("LuaData", lua_event_data, sizeof(lua_event_data));

                if (ImGui::Button("Send Lua Event")) {
                    if (lua_event_name[0] != '\0') {
                        dispatch_lua_event(lua_event_name, lua_event_data);
                    }
                }
                      
                
                static char custom_event_name[256]{};
                ImGui::Text("Send Event");
                ImGui::InputText("customEvent", custom_event_name, sizeof(custom_event_name));
                
                static char custom_event_data[256]{};
                ImGui::InputText("customData", custom_event_data, sizeof(custom_event_data));

                if (ImGui::Button("Send custom Event")) {
                       send_custom_event(custom_event_name, custom_event_data);
                    }
                


                static char osd_event_name[256]{};
                ImGui::Text("Set osd text");
                ImGui::InputText("OSDtext", osd_event_name, sizeof(osd_event_name));
				ImVec4 osd_text_color = text_color;
				if (ImGui::ColorEdit4("TextColor", (float*)&osd_text_color))
                    text_color = osd_text_color;
                ImVec4 osd_bg_color = bg_color;
                if (ImGui::ColorEdit4("Background Color", (float*)&osd_bg_color))
                    bg_color = osd_bg_color;
                ImVec2 osd_temp_offset = ImVec2(0,0);
                if (ImGui::Button("Show OSD Text"))
                    show_osd = !show_osd;
                if (show_osd) osd(osd_event_name, ImVec2(osd_temp_offset), false, osd_text_color, bg_color);
                static char input[256]{};
                std::string mod_value = API::VR::get_mod_value<std::string>(input);
                ImGui::Text("Mod value: %s", mod_value.c_str());
                if (ImGui::CollapsingHeader("values")) {
                    ImGui::Text(show_osd ? "show osd: true" : "show osd : false");
                    ImGui::Text(osd_text.c_str());
                }
            }
            ImGui::End();
        }
        
        if (show_osd && !osd_text.empty()) {
            osd(osd_text, osd_offset, true, text_color, bg_color);
		}
    }

private:
    HWND m_wnd{};
    bool m_initialized{ false };
    bool m_was_rendering_desktop{ false };
    bool show_menu = false;
    bool show_osd = false;
	std::string osd_text = "";
    std::recursive_mutex m_imgui_mutex{};
    ImVec4 text_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    ImVec4 bg_color = ImVec4(0.11f, 0.11f, 0.11f, 0.7f);
    bool show_bg = false;
    ImVec2 osd_offset = ImVec2(0,0);
    bool lua_overrides = false;
};

// Actually creates the plugin. Very important that this global is created.
// The fact that it's using std::unique_ptr is not important, as long as the constructor is called in some way.
std::unique_ptr<OSDTemplate> g_plugin{ new OSDTemplate() };
