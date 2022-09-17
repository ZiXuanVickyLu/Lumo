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

#include "View3DPane.h"

#include <Panes/Manager/LayoutManager.h>
#include <ImWidgets/ImWidgets.h>
#include <ImGuiFileDialog/ImGuiFileDialog.h>
#include <vkFramework/VulkanImGuiRenderer.h>
#include <Project/ProjectFile.h>
#include <imgui/imgui_internal.h>
#include <Interfaces/TextureOutputInterface.h>
#include <Graph/Base/BaseNode.h>
#include <Graph/Base/NodeSlot.h>
#include <Graph/Manager/NodeManager.h>
#include <Systems/CommonSystem.h>
#include <ImGuizmo/ImGuizmo.h>
#include <ctools/cTools.h>
#include <ctools/FileHelper.h>
#include <cinttypes> // printf zu

#define TRACE_MEMORY
#include <vkProfiler/Profiler.h>

static int SourcePane_WidgetId = 0;

///////////////////////////////////////////////////////////////////////////////////
//// CONSTRUCTORS /////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

View3DPane::View3DPane() = default;
View3DPane::~View3DPane()
{
	Unit();
}

///////////////////////////////////////////////////////////////////////////////////
//// INIT/UNIT ////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

bool View3DPane::Init()
{
	ZoneScoped;

	return true;
}

void View3DPane::Unit()
{
	ZoneScoped;

	m_TextureOutputSlot.reset();
}

///////////////////////////////////////////////////////////////////////////////////
//// IMGUI PANE ///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

int View3DPane::DrawPanes(const uint32_t& vCurrentFrame, int vWidgetId, std::string vUserDatas, PaneFlags& vInOutPaneShown)
{
	ZoneScoped;

	SourcePane_WidgetId = vWidgetId;

	if (vInOutPaneShown & m_PaneFlag)
	{
		static ImGuiWindowFlags flags =
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_MenuBar;
		if (ImGui::Begin<PaneFlags>(m_PaneName,
			&vInOutPaneShown , m_PaneFlag, flags))
		{
#ifdef USE_DECORATIONS_FOR_RESIZE_CHILD_WINDOWS
			auto win = ImGui::GetCurrentWindowRead();
			if (win->Viewport->Idx != 0)
				flags |= ImGuiWindowFlags_NoResize;// | ImGuiWindowFlags_NoTitleBar;
			else
				flags = ImGuiWindowFlags_NoCollapse |
				ImGuiWindowFlags_NoBringToFrontOnFocus |
				ImGuiWindowFlags_MenuBar;
#endif
			if (ProjectFile::Instance()->IsLoaded())
			{
				SetOrUpdateOutput(m_TextureOutputSlot);

				auto slotPtr = m_TextureOutputSlot.getValidShared();
				if (slotPtr)
				{
					if (ImGui::BeginMenuBar())
					{
						//SceneManager::Instance()->DrawBackgroundColorMenu();

						ImGui::RadioButtonLabeled(0.0f, "Camera", "Can Use Camera\nCan be used with alt key if not selected", &m_CanWeTuneCamera);
						ImGui::RadioButtonLabeled(0.0f, "Mouse", "Can Use Mouse\nBut not when camera is active", &m_CanWeTuneMouse);
						if (ImGui::RadioButtonLabeled(0.0f, "Gizmo", "Can Use Gizmo", &m_CanWeTuneGizmo))
						{
							ImGuizmo::Enable(m_CanWeTuneGizmo);
						}
						if (ImGui::ContrastedButton("Leave"))
						{
							SetOrUpdateOutput(NodeSlotWeak());
						}

						ImGui::EndMenuBar();
					}

					ct::ivec2 contentSize = ImGui::GetContentRegionAvail();
					
					if (m_ImGuiTexture.canDisplayPreview)
					{
						m_PreviewRect = ct::GetScreenRectWithRatio<int32_t>(m_ImGuiTexture.ratio, contentSize, false);

						ImVec2 pos = ImVec2((float)m_PreviewRect.x, (float)m_PreviewRect.y);
						ImVec2 siz = ImVec2((float)m_PreviewRect.w, (float)m_PreviewRect.h);

						// faut faire ca avant le dessin de la texture.
						// apres, GetCursorScreenPos ne donnera pas la meme valeur
						ImVec2 org = ImGui::GetCursorScreenPos() + pos;
						ImGui::ImageRect((ImTextureID)&m_ImGuiTexture.descriptor, pos, siz);

						NodeManager::Instance()->DrawOverlays(vCurrentFrame, ct::frect(org, siz), ImGui::GetCurrentContext());

						if (ImGui::IsWindowHovered())
						{
							if (ImGui::IsMouseHoveringRect(org, org + siz))
							{
								if (m_CanWeTuneMouse && CanUpdateMouse(true, 0))
								{
									ct::fvec2 norPos = (ImGui::GetMousePos() - org) / siz;
									CommonSystem::Instance()->SetMousePos(norPos, m_PaneSize, GImGui->IO.MouseDown);
								}

								UpdateCamera(org, siz);
							}
						}

						if (CommonSystem::Instance()->UpdateIfNeeded(ct::uvec2((uint32_t)siz.x, (uint32_t)siz.y)))
						{
							CommonSystem::Instance()->SetScreenSize(ct::uvec2((uint32_t)siz.x, (uint32_t)siz.y));
						}
					}

					// on test ca car si le vue n'est pas visible cad si maxSize.y est inferieur a 0 alors
					// on a affaire a un resize constant, car outputsize n'est jamais maj, tant que la vue n'a pas rendu
					// et ca casse le fps de l'ui, rendu son utilisation impossible ( deltatime superieur 32ms d'apres tracy sur un gtx 1050 Ti)
					if (contentSize.x > 0 && contentSize.y > 0)
					{
						if (m_PaneSize.x != contentSize.x ||
							m_PaneSize.y != contentSize.y)
						{
							auto parentNodePtr = slotPtr->parentNode.getValidShared();
							if (parentNodePtr)
							{
								parentNodePtr->NeedResizeByResizeEvent(&contentSize, nullptr);
								CommonSystem::Instance()->SetScreenSize(ct::uvec2(contentSize.x, contentSize.y));
								CommonSystem::Instance()->NeedCamChange();
							}

							m_PaneSize = contentSize;

						}
					}
				}
			}
		}

		ImGui::End();
	}

	return SourcePane_WidgetId;
}

///////////////////////////////////////////////////////////////////////////////////
//// DIALOGS //////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

void View3DPane::DrawDialogsAndPopups(const uint32_t& vCurrentFrame, std::string vUserDatas)
{
	ZoneScoped;

	/*
	if (ProjectFile::Instance()->IsLoaded())
	{
		ImVec2 maxSize = MainFrame::Instance()->m_DisplaySize;
		ImVec2 minSize = maxSize * 0.5f;
		if (ImGuiFileDialog::Instance()->Display("OpenShaderCode",
			ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking,
			minSize, maxSize))
		{
			if (ImGuiFileDialog::Instance()->IsOk())
			{
				std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
				auto code = FileHelper::Instance()->LoadFileToString(filePathName);
				SetCode(code);
			}
			ImGuiFileDialog::Instance()->Close();
		}
	}
	*/
}

int View3DPane::DrawWidgets(const uint32_t& vCurrentFrame, int vWidgetId, std::string vUserDatas)
{
	return vWidgetId;
}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

ct::fvec2 View3DPane::SetOrUpdateOutput(NodeSlotWeak vTextureOutputSlot)
{
	ZoneScoped;

	ct::fvec2 outSize;

	auto slotPtr = vTextureOutputSlot.getValidShared();
	if (slotPtr)
	{
		auto otherNodePtr = std::dynamic_pointer_cast<TextureOutputInterface>(slotPtr->parentNode.getValidShared());
		if (otherNodePtr)
		{
			NodeSlot::sSlotGraphOutputMouseLeft = vTextureOutputSlot;
			m_TextureOutputSlot = vTextureOutputSlot;
			NodeManager::Instance()->m_RootNodePtr->m_GraphRoot3DNode = slotPtr->parentNode;
			m_ImGuiTexture.SetDescriptor(m_VulkanImGuiRenderer, otherNodePtr->GetDescriptorImageInfo(slotPtr->descriptorBinding, &outSize));
			m_ImGuiTexture.ratio = outSize.ratioXY<float>();
		}
		else
		{
			NodeSlot::sSlotGraphOutputMouseLeft.reset();
			m_TextureOutputSlot.reset();
			m_ImGuiTexture.ClearDescriptor();
		}
	}
	else
	{
		NodeSlot::sSlotGraphOutputMouseLeft.reset();
		m_TextureOutputSlot.reset();
		m_ImGuiTexture.ClearDescriptor();
	}

	return outSize;
}

void View3DPane::SetVulkanImGuiRenderer(VulkanImGuiRendererWeak vVulkanImGuiRenderer)
{ 
	m_VulkanImGuiRenderer = vVulkanImGuiRenderer; 
}

void View3DPane::SetDescriptor(vkApi::VulkanFrameBufferAttachment* vVulkanFrameBufferAttachment)
{
	m_ImGuiTexture.SetDescriptor(m_VulkanImGuiRenderer, vVulkanFrameBufferAttachment);
}

bool View3DPane::CanUpdateMouse(bool vWithMouseDown, int vMouseButton)
{
	ZoneScoped;

	bool canUpdateMouse = true;
	//(!MainFrame::g_AnyWindowsHovered) &&
	//(ImGui::GetActiveID() == 0) &&
	//(ImGui::GetHoveredID() == 0);// &&
	//(!ImGui::IsWindowHovered()) &&
	//(!ImGuiFileDialog::Instance()->m_AnyWindowsHovered);
	//if (MainFrame::g_CentralWindowHovered && (ImGui::GetActiveID() != 0) && !ImGuizmo::IsUsing())
	//	canUpdateMouse = true;
	if (vWithMouseDown)
	{
		static bool lastMouseDownState[3] = { false, false, false };
		canUpdateMouse &= lastMouseDownState[vMouseButton] || ImGui::IsMouseDown(vMouseButton);
		lastMouseDownState[vMouseButton] = ImGui::IsMouseDown(vMouseButton);
	}
	return canUpdateMouse;
}

void View3DPane::UpdateCamera(ImVec2 vOrg, ImVec2 vSize)
{
	ZoneScoped;

	bool canTuneCamera = !ImGuizmo::IsUsing() && (m_CanWeTuneCamera || ImGui::IsKeyPressed(ImGuiKey_LeftAlt));

	// update mesher camera // camera of renderpack
	if (CanUpdateMouse(true, 0)) // left mouse rotate
	{
		if (canTuneCamera)// && !ImGuizmo::IsUsing())
		{
			ct::fvec2 pos = ct::fvec2(ImGui::GetMousePos()) / m_DisplayQuality;
			m_CurrNormalizedMousePos.x = (ImGui::GetMousePos().x - vOrg.x) / vSize.x;
			m_CurrNormalizedMousePos.y = (ImGui::GetMousePos().y - vOrg.y) / vSize.y;

			if (!m_MouseDrag)
				m_LastNormalizedMousePos = m_CurrNormalizedMousePos; // first diff to 0
			m_MouseDrag = true;

			ct::fvec2 diff = m_CurrNormalizedMousePos - m_LastNormalizedMousePos;
			CommonSystem::Instance()->IncRotateXYZ(ct::fvec3(diff * 6.28318f, 0.0f));

			if (!diff.emptyAND()) m_UINeedRefresh |= true;
			m_LastNormalizedMousePos = m_CurrNormalizedMousePos;
		}
	}
	else if (CanUpdateMouse(true, 1)) // right mouse zoom
	{
		if (canTuneCamera)// && !ImGuizmo::IsUsing())
		{
			ct::fvec2 pos = ct::fvec2(ImGui::GetMousePos()) / m_DisplayQuality;
			m_CurrNormalizedMousePos.x = (ImGui::GetMousePos().x - vOrg.x) / vSize.x;
			m_CurrNormalizedMousePos.y = (ImGui::GetMousePos().y - vOrg.y) / vSize.y;

			if (!m_MouseDrag)
				m_LastNormalizedMousePos = m_CurrNormalizedMousePos; // first diff to 0
			m_MouseDrag = true;

			ct::fvec2 diff = m_CurrNormalizedMousePos - m_LastNormalizedMousePos;
			CommonSystem::Instance()->IncZoom(diff.y * 50.0f);
			CommonSystem::Instance()->IncRotateXYZ(ct::fvec3(0.0f, 0.0f, diff.x * 6.28318f));

			if (!diff.emptyAND()) m_UINeedRefresh |= true;
			m_LastNormalizedMousePos = m_CurrNormalizedMousePos;
		}
	}
	else if (CanUpdateMouse(true, 2)) // middle mouse, translate
	{
		if (canTuneCamera)// && !ImGuizmo::IsUsing())
		{
			ct::fvec2 pos = ct::fvec2(ImGui::GetMousePos()) / m_DisplayQuality;
			m_CurrNormalizedMousePos.x = (ImGui::GetMousePos().x - vOrg.x) / vSize.x;
			m_CurrNormalizedMousePos.y = (ImGui::GetMousePos().y - vOrg.y) / vSize.y;

			if (!m_MouseDrag)
				m_LastNormalizedMousePos = m_CurrNormalizedMousePos; // first diff to 0
			m_MouseDrag = true;

			ct::fvec2 diff = m_CurrNormalizedMousePos - m_LastNormalizedMousePos;
			CommonSystem::Instance()->IncTranslateXY(diff * 10.0f);

			if (!diff.emptyAND()) m_UINeedRefresh |= true;
			m_LastNormalizedMousePos = m_CurrNormalizedMousePos;
		}
	}
	else
	{
		m_MouseDrag = false;
	}
}