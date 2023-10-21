/*
Copyright 2022-2022 Stephane Cuillerdier (aka aiekick)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <Panes/CodePane.h>
#include <Gaia/gaia.h>
#include <ctools/cTools.h>
#include <ctools/FileHelper.h>
#include <LumoBackend/Utils/ZepImGuiEditor.h>
#include <LumoBackend/Systems/CommonSystem.h>
#include <LumoBackend/Graph/Base/BaseNode.h>

#ifdef PROFILER_INCLUDE
#include PROFILER_INCLUDE
#endif
#ifndef ZoneScoped
#define ZoneScoped
#endif
#ifndef FrameMark
#define FrameMark
#endif

#include <cinttypes> // printf zu

CodePane::CodePane() = default;
CodePane::~CodePane() = default;

bool CodePane::Init()
{
	// Zep Editor
	// Called once the fonts/device is guaranteed setup
	zep_init(Zep::NVec2f(1.0f, 1.0f));
	zep_load("GrayScott.vk_frag", u8R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1 ) in;

layout(binding = 0, rgba32f) uniform image2D colorBuffer;

layout(std140, binding = 2) uniform UBO_Comp
{
	float mouse_radius;
	float mouse_inversion;
	float reset_substances;
	float grayscott_diffusion_u;
	float grayscott_diffusion_v;
	float grayscott_feed;
	float grayscott_kill;
	ivec2 image_size;
};

layout(binding = 3) uniform sampler2D input_mask;

vec4 getPixel(ivec2 g, int x, int y)
{
    ivec2 v = (g + ivec2(x,y)) % image_size;
	return imageLoad(colorBuffer, v);
}

/* laplacian corner ratio */	#define lc .2
/* laplacian side ratio */ 		#define ls .8

vec4 grayScott(ivec2 g, vec4 mo)
{
    vec4 l 	= getPixel(g, -1,  0);
	vec4 lt = getPixel(g, -1,  1);
	vec4 t 	= getPixel(g,  0,  1);
	vec4 rt = getPixel(g,  1,  1);
	vec4 r 	= getPixel(g,  1,  0);
	vec4 rb = getPixel(g,  1, -1);
	vec4 b 	= getPixel(g,  0, -1);
	vec4 lb = getPixel(g, -1, -1);
	vec4 c 	= getPixel(g,  0,  0);
	vec4 lap = (l+t+r+b)/4.*ls + (lt+rt+rb+lb)/4.*lc - c; // laplacian

	float re = c.x * c.y * c.y; // reaction
    c.xy += vec2(grayscott_diffusion_u, grayscott_diffusion_v) * lap.xy + 
		vec2(grayscott_feed * (1. - c.x) - re, re - (grayscott_feed + grayscott_kill) * c.y); // grayscott formula
	
	/*if (length(vec2(g - image_size / 2)) < mouse_radius) 
	{
		c = vec4(0,1,0,1);
	}*/

	if (mo.z > 0.) 
	{
		if (length(vec2(g) - mo.xy * vec2(image_size)) < mouse_radius) 
		{
			if (mouse_inversion > 0.5)
			{
				c = vec4(1,0,0,1);
			}
			else
			{
				c = vec4(0,1,0,1);
			}
		}
	}

	if (reset_substances > 0.5)
	{
		c = vec4(1,0,0,1);	
	}

	return vec4(clamp(c.xy, -1e1, 1e1), 0, 1);
}

void main()
{
	const ivec2 coords = ivec2(gl_GlobalInvocationID.xy);

	vec4 color = grayScott(coords, left_mouse);

	imageStore(colorBuffer, coords, color); 
}
)");

	return true;
}

void CodePane::Unit()
{
	m_NodeToDebug.reset();

	zep_destroy();
}

///////////////////////////////////////////////////////////////////////////////////
//// IMGUI PANE ///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

bool CodePane::DrawPanes(const uint32_t& vCurrentFrame,
    PaneFlags& vInOutPaneShown,
    ImGuiContext* vContextPtr,
    const std::string& vUserDatas) {
    ZoneScoped;

    bool change = false;

    if (vInOutPaneShown & paneFlag) {
        static ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                        ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollbar;
        if (ImGui::Begin<PaneFlags>(paneName.c_str(), &vInOutPaneShown, paneFlag, flags)) {
#ifdef USE_DECORATIONS_FOR_RESIZE_CHILD_WINDOWS
            auto win = ImGui::GetCurrentWindowRead();
            if (win->Viewport->Idx != 0)
                flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_MenuBar |
                         ImGuiWindowFlags_NoScrollbar;
            else
                flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus |
                        ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollbar;
#endif
            // Required for CTRL+P and flashing cursor.
            zep_update();

            // Just show it
            auto size = ImGui::GetContentRegionAvail();
            zep_show(Zep::NVec2i((int)size.x, (int)size.y));
        }

        ImGui::End();
    }

    return change;
}

///////////////////////////////////////////////////////////////////////////////////
//// DIALOGS //////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

bool CodePane::DrawDialogsAndPopups(
    const uint32_t& vCurrentFrame, const ImVec2& vMaxSize, ImGuiContext* vContextPtr, const std::string& vUserDatas) {
    ZoneScoped;

    return false;
}

bool CodePane::DrawWidgets(const uint32_t& vCurrentFrame, ImGuiContext* vContextPtr, const std::string& vUserDatas) {
    bool change = false;

    change |= CommonSystem::Instance()->DrawImGui();

    auto ptr = GetParentNode().lock();
    if (ptr) {
        change |= ptr->DrawWidgets(vCurrentFrame, ImGui::GetCurrentContext(), vUserDatas);
    }

    return change;
}

bool CodePane::DrawOverlays(
    const uint32_t& vCurrentFrame, const ImRect& vRect, ImGuiContext* vContextPtr, const std::string& vUserDatas) {
    ZoneScoped;
    UNUSED(vCurrentFrame);
    UNUSED(vRect);
    ImGui::SetCurrentContext(vContextPtr);
    UNUSED(vUserDatas);
    return false;
}

///////////////////////////////////////////////////////////////////////////////////
//// SELECTOR /////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

void CodePane::Select(BaseNodeWeak vObjet) { SetParentNode(vObjet); }