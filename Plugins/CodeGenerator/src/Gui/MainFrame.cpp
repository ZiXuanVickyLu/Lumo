﻿/*
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

#include <Headers/LumoCodeGeneratorBuild.h>

#include <Gui/MainFrame.h>
#include <ctools/cTools.h>
#include <ctools/FileHelper.h>
#include <cctype>
#include <GLFW/glfw3.h>
#include <ImGuiFileDialog/ImGuiFileDialog.h>
#include <ImWidgets/ImWidgets.h>
#include <FontIcons/CustomFont.h>
#include <FontIcons/CustomFont.h>
#include <Helper/ThemeHelper.h>
#include <Helper/Messaging.h>
#include <LumoBackend/Graph/GeneratorNode.h>
#include <imgui/imgui_internal.h>
#include <LumoBackend/Graph/Layout/GraphLayout.h>
#include <Project/ProjectFile.h>

MainFrame::MainFrame(GLFWwindow* vWin)
{
	m_Window = vWin;
}


MainFrame::~MainFrame()
{
	
}

bool MainFrame::Init(GaiApi::VulkanCorePtr vVulkanCorePtr)
{
	m_VulkanCorePtr = vVulkanCorePtr;

	using namespace std::placeholders;
	BaseNode::sSelectCallback = std::bind(&MainFrame::SelectNode, this, _1);
	BaseNode::sSelectSlotCallback = std::bind(&MainFrame::SelectSlot, this, _1, _2);
	BaseNode::sLoadNodeFromXMLCallback = std::bind(&MainFrame::LoadNodeFromXML, this, _1, _2, _3, _4, _5, _6, _7);
	NodeSlot::sSlotGraphOutputMouseLeftColor = ImVec4(0.2f, 0.9f, 0.2f, 1.0f);
	NodeSlot::sSlotGraphOutputMouseRightColor = ImVec4(0.2f, 0.9f, 0.2f, 1.0f);

	SetAppTitle();

	ThemeHelper::Instance(); // default
	LoadConfigFile("generatorConfig.xml");

	ThemeHelper::Instance()->ApplyStyle();

	NodeSlot::sGetSlotColors()->AddSlotColor("NONE", ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
	NodeSlot::sGetSlotColors()->AddSlotColor("MESH", ImVec4(0.5f, 0.5f, 0.9f, 1.0f));
	NodeSlot::sGetSlotColors()->AddSlotColor("MESH_GROUP", ImVec4(0.1f, 0.1f, 0.8f, 1.0f));
	NodeSlot::sGetSlotColors()->AddSlotColor("LIGHT_GROUP", ImVec4(0.9f, 0.9f, 0.1f, 1.0f));
	NodeSlot::sGetSlotColors()->AddSlotColor("ENVIRONMENT", ImVec4(0.1f, 0.9f, 0.1f, 1.0f));
	NodeSlot::sGetSlotColors()->AddSlotColor("MERGED", ImVec4(0.1f, 0.5f, 0.9f, 1.0f));
	NodeSlot::sGetSlotColors()->AddSlotColor("TEXTURE_2D", ImVec4(0.9f, 0.5f, 0.1f, 1.0f));
	NodeSlot::sGetSlotColors()->AddSlotColor("TEXTURE_2D_GROUP", ImVec4(0.2f, 0.9f, 0.2f, 1.0f));
	NodeSlot::sGetSlotColors()->AddSlotColor("TEXTURE_3D", ImVec4(0.9f, 0.8f, 0.3f, 1.0f));
	NodeSlot::sGetSlotColors()->AddSlotColor("MIXED", ImVec4(0.3f, 0.5f, 0.1f, 1.0f));
	NodeSlot::sGetSlotColors()->AddSlotColor("WIDGET_BOOLEAN", ImVec4(0.8f, 0.7f, 0.6f, 1.0f));
	NodeSlot::sGetSlotColors()->AddSlotColor("WIDGET_UINT", ImVec4(0.8f, 0.7f, 0.6f, 1.0f));
	NodeSlot::sGetSlotColors()->AddSlotColor("WIDGET_INT", ImVec4(0.8f, 0.7f, 0.6f, 1.0f));
	NodeSlot::sGetSlotColors()->AddSlotColor("WIDGET_FLOAT", ImVec4(0.8f, 0.7f, 0.6f, 1.0f));
	NodeSlot::sGetSlotColors()->AddSlotColor("DEPTH", ImVec4(0.2f, 0.7f, 0.6f, 1.0f));

	return false;
}

void MainFrame::Unit()
{
	ProjectFile::Instance()->Save();
	SaveConfigFile("generatorConfig.xml");
}

void MainFrame::PostRenderingActions()
{
	if (m_NeedToNewProject)
	{
		ProjectFile::Instance()->New(m_FilePathNameToLoad);
		SetAppTitle(m_FilePathNameToLoad);

		m_FilePathNameToLoad.clear();
		m_NeedToNewProject = false;
	}

	if (m_NeedToLoadProject)
	{
		if (ProjectFile::Instance()->LoadAs(m_FilePathNameToLoad))
		{
			SetAppTitle(m_FilePathNameToLoad);
			ProjectFile::Instance()->SetProjectChange(false);

			// on selectionne le 1er node du projet
			if (ProjectFile::Instance()->m_SelectedNode.expired())
			{
				if (!ProjectFile::Instance()->m_RootNodePtr->m_ChildNodes.empty())
				{
					SelectNode(ProjectFile::Instance()->m_RootNodePtr->m_ChildNodes.begin()->second);
				}
			}

			// on selectionne les 1ers slots du node selectionné 
			// a l'issue du chargement
			auto nodePtr = ProjectFile::Instance()->m_SelectedNode.lock();
			if (nodePtr)
			{
				// selection of the first slot
				if (!nodePtr->m_Inputs.empty())
				{
					SelectSlot(nodePtr->m_Inputs.begin()->second, ImGuiMouseButton_Left);
				}

				// selection of the first slot
				if (!nodePtr->m_Outputs.empty())
				{
					SelectSlot(nodePtr->m_Outputs.begin()->second, ImGuiMouseButton_Left);
				}
			}
		}
		else
		{
			Messaging::Instance()->AddError(true, nullptr, nullptr,
				"Failed to load project %s", m_FilePathNameToLoad.c_str());
		}

		m_FilePathNameToLoad.clear();
		m_NeedToLoadProject = false;
	}

	if (m_NeedToCloseProject)
	{
		ProjectFile::Instance()->Clear();
		m_NeedToCloseProject = false;
	}
}

//////////////////////////////////////////////////////////////////////////////
//// DRAW ////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

void MainFrame::Display(const uint32_t& vCurrentFrame, ct::ivec4 vViewport)
{
	if (m_NeedToApplyLayout && ProjectFile::Instance()->m_RootNodePtr)
	{
		m_NeedToApplyLayout = false;
		GraphLayout::Instance()->ApplyLayout(ProjectFile::Instance()->m_RootNodePtr);
	}

	m_DisplayPos = ImVec2((float)vViewport.x, (float)vViewport.y);
	m_DisplaySize = ImVec2((float)vViewport.z, (float)vViewport.w);

	const auto context = ImGui::GetCurrentContext();
	if (context)
	{
		if (ImGui::BeginMainMenuBar())
		{
			DrawMainMenuBar();

			auto io = ImGui::GetIO();
			const auto label = ct::toStr("Dear ImGui %s (Docking)", ImGui::GetVersion());
			const auto size = ImGui::CalcTextSize(label.c_str());
			ImGui::Spacing(ImGui::GetContentRegionAvail().x - size.x - ImGui::GetStyle().FramePadding.x * 2.0f);
			ImGui::Text("%s", label.c_str());

			ImGui::EndMainMenuBar();
		}

		if (ImGui::BeginMainStatusBar())
		{
			Messaging::Instance()->Draw();

			const auto io = ImGui::GetIO();
			const auto fps = ct::toStr("%.1f ms/frame (%.1f fps)", 1000.0f / io.Framerate, io.Framerate);
			const auto size = ImGui::CalcTextSize(fps.c_str());
			ImGui::Spacing(ImGui::GetContentRegionAvail().x - size.x - ImGui::GetStyle().FramePadding.x * 2.0f);
			ImGui::Text("%s", fps.c_str());

			ImGui::EndMainStatusBar();
		}

		if (ProjectFile::Instance()->IsLoaded())
		{
			ImGui::SetNextWindowPos(m_DisplayPos + ImVec2(0, ImGui::GetFrameHeight()));
			ImGui::SetNextWindowSize(ImVec2(m_DisplaySize.x * 0.5f, m_DisplaySize.y - ImGui::GetFrameHeight() * 2.0f));

			ImGui::Begin("Parameters", 0,
				ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_NoDocking |
				ImGuiWindowFlags_NoCollapse |
				ImGuiWindowFlags_NoSavedSettings);

			DrawContent();

			ImGui::End();

			ImGui::SetNextWindowPos(m_DisplayPos + ImVec2(m_DisplaySize.x * 0.5f, ImGui::GetFrameHeight()));
			ImGui::SetNextWindowSize(ImVec2(m_DisplaySize.x * 0.5f, m_DisplaySize.y - ImGui::GetFrameHeight() * 2.0f));

			ImGui::Begin("Graph", 0,
				ImGuiWindowFlags_MenuBar |
				ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_NoDocking |
				ImGuiWindowFlags_NoCollapse |
				ImGuiWindowFlags_NoSavedSettings);

			DrawGraph();

			ImGui::End();
		}
		
		DisplayDialogsAndPopups(vCurrentFrame);

		ThemeHelper::Instance()->Draw();
	}
}

void MainFrame::NeedToNewProject(const std::string& vFilePathName)
{
	m_NeedToNewProject = true;
	m_FilePathNameToLoad = vFilePathName;
}

void MainFrame::NeedToLoadProject(const std::string& vFilePathName)
{
	m_NeedToLoadProject = true;
	m_FilePathNameToLoad = vFilePathName;
}

void MainFrame::NeedToCloseProject()
{
	m_NeedToCloseProject = true;
}


void MainFrame::DisplayDialogsAndPopups(const uint32_t& vCurrentFrame)
{
	m_ActionSystem.RunActions();

	if (ImGuiFileDialog::Instance()->Display("GenerateToPath"))
	{
		if (ImGuiFileDialog::Instance()->IsOk())
		{
			std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();

			ProjectFile::Instance()->GenerateGraphFiles(filePath);
		}

		ImGuiFileDialog::Instance()->Close();
	}
	if (m_ShowImGui)
		ImGui::ShowDemoWindow(&m_ShowImGui);
	if (m_ShowMetric)
		ImGui::ShowMetricsWindow(&m_ShowMetric);
}

void MainFrame::SetAppTitle(const std::string& vFilePathName)
{
	char bufTitle[1024];
	snprintf(bufTitle, 1023, "Lumo Code Generator Beta %s", LumoCodeGenerator_BuildId);
	glfwSetWindowTitle(m_Window, bufTitle);
}

void MainFrame::DrawMainMenuBar()
{
	if (ImGui::BeginMenu(ICON_NDP_PROJECT " Project"))
	{
		if (ImGui::MenuItem(ICON_NDP_FILE " New"))
		{
			Action_Menu_NewProject();
		}

		if (ImGui::MenuItem(ICON_NDP_FOLDER_OPEN " Open"))
		{
			Action_Menu_OpenProject();
		}

		if (ProjectFile::Instance()->IsLoaded())
		{
			ImGui::Separator();

			if (ImGui::MenuItem(ICON_NDP_FOLDER_OPEN " Re Open"))
			{
				Action_Menu_ReOpenProject();
			}

			ImGui::Separator();

			if (ImGui::MenuItem(ICON_NDP_SAVE " Save"))
			{
				Action_Menu_SaveProject();
			}

			if (ImGui::MenuItem(ICON_NDP_SAVE " Save As"))
			{
				Action_Menu_SaveAsProject();
			}

			ImGui::Separator();

			if (ImGui::MenuItem(ICON_NDP_DESTROY " Close"))
			{
				Action_Menu_CloseProject();
			}
		}

		ImGui::Separator();

		if (ImGui::MenuItem(ICON_NDP_QUESTION " About"))
		{
			m_ShowAboutDialog = true;
		}

		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu(ICON_NDP_COGS " Settings"))
	{
		if (ImGui::BeginMenu(ICON_NDP_PICTURE_O " Styles"))
		{
			ThemeHelper::Instance()->DrawMenu();

			ImGui::Separator();

			ImGui::MenuItem("Show ImGui", "", &m_ShowImGui);
			ImGui::MenuItem("Show ImGui Metric/Debug", "", &m_ShowMetric);

			ImGui::EndMenu();
		}

		ImGui::EndMenu();
	}
}

void MainFrame::DrawContent()
{
	if (ImGui::BeginTabBar("##tools"))
	{
		/*if (ImGui::BeginTabItem("Plugin Creation"))
		{
		

			ImGui::EndTabItem();
		}*/

		if (ImGui::BeginTabItem("Node Creation"))
		{
			DrawNodeCreationPane();

			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}
}

void MainFrame::DrawGraph()
{
	if (ProjectFile::Instance()->m_RootNodePtr)
	{
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::MenuItem("Layout", "apply Layout"))
			{
				GraphLayout::Instance()->ApplyLayout(ProjectFile::Instance()->m_RootNodePtr);
			}

			ProjectFile::Instance()->m_RootNodePtr->DrawToolMenu();

			if (ProjectFile::Instance()->m_RootNodePtr->m_BaseNodeState.m_NodeGraphContext)
			{
				nd::SetCurrentEditor(ProjectFile::Instance()->m_RootNodePtr->m_BaseNodeState.m_NodeGraphContext);
				if (nd::GetSelectedObjectCount())
				{
					if (ImGui::BeginMenu("Selection"))
					{
						if (ImGui::MenuItem("Zoom on Selection"))
						{
							ProjectFile::Instance()->m_RootNodePtr->ZoomToSelection();
						}

						if (ImGui::MenuItem("Center on Selection"))
						{
							ProjectFile::Instance()->m_RootNodePtr->NavigateToSelection();
						}

						ImGui::EndMenu();
					}
				}

				if (ImGui::BeginMenu("Content"))
				{
					if (ImGui::MenuItem("Zoom on Content"))
					{
						ProjectFile::Instance()->m_RootNodePtr->ZoomToContent();
					}

					if (ImGui::MenuItem("Center on Content"))
					{
						ProjectFile::Instance()->m_RootNodePtr->NavigateToContent();
					}

					ImGui::EndMenu();
				}
			}

			if (ImGui::BeginMenu("Style"))
			{
				ProjectFile::Instance()->m_RootNodePtr->DrawStyleMenu();
				GraphLayout::Instance()->DrawSettings();

				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		ProjectFile::Instance()->m_RootNodePtr->DrawGraph();
	}
}

void MainFrame::DrawPluginCreationPane()
{

}

void MainFrame::DrawNodeCreationPane()
{
	const float aw = ImGui::GetContentRegionAvail().x;

	if (ImGui::ContrastedButton("Clear Graph (WARNING, cant be canceled !)"))
	{
		ProjectFile::Instance()->m_RootNodePtr->ClearGraph();
	}

	if (ImGui::ContrastedButton("New Node"))
	{
		SelectNode(std::dynamic_pointer_cast<GeneratorNode>(
			ProjectFile::Instance()->m_RootNodePtr->AddChildNode(GeneratorNode::Create(m_VulkanCorePtr)).lock()));
		auto nodePtr = ProjectFile::Instance()->m_SelectedNode.lock();
		if (nodePtr)
		{
			nodePtr->name = "New Node";
		}
		m_NeedToApplyLayout = true;
	}

	auto nodePtr = ProjectFile::Instance()->m_SelectedNode.lock();
	if (nodePtr)
	{
		ImGui::SameLine();

		if (ImGui::ContrastedButton("Delete the Node"))
		{
			ProjectFile::Instance()->m_RootNodePtr->DestroyChildNode(ProjectFile::Instance()->m_SelectedNode);
		}

		if (m_NodeDisplayNameInputText.DisplayInputText(aw * 0.5f, "Node Display Name :", "New Node"))
		{
			nodePtr->m_NodeDisplayName = m_NodeDisplayNameInputText.GetText();
			nodePtr->name = nodePtr->m_NodeDisplayName;

			nodePtr->m_NodeCreationName = ct::toUpper(nodePtr->m_NodeDisplayName);
			ct::replaceString(nodePtr->m_NodeCreationName, " ", "_");
			m_NodeCreationNameInputText.SetText(nodePtr->m_NodeCreationName);

			nodePtr->m_ClassName = nodePtr->m_NodeDisplayName;
			ct::replaceString(nodePtr->m_ClassName, " ", "");
			m_ClassNameInputText.SetText(nodePtr->m_ClassName);

			nodePtr->m_ModuleDisplayName = nodePtr->m_NodeDisplayName;
			m_ModuleDisplayNameInputText.SetText(nodePtr->m_ModuleDisplayName);

			nodePtr->m_ModuleXmlName = ct::toLower(nodePtr->m_NodeCreationName) + "_module";
			m_ModuleXmlNameInputText.SetText(nodePtr->m_ModuleXmlName);
		}

		if (m_NodeCreationNameInputText.DisplayInputText(aw * 0.5f, "Node Creation Name :", "NEW_NODE"))
		{
			nodePtr->m_NodeCreationName = m_NodeCreationNameInputText.GetText();
			ct::replaceString(nodePtr->m_NodeCreationName, " ", "_");
			m_NodeCreationNameInputText.SetText(nodePtr->m_NodeCreationName);
		}

		ImGui::Separator();

		ImGui::CheckBoxBoolDefault("Is a Task ?", &nodePtr->m_IsATask, true);

		ImGui::Separator();

		if (ImGui::ContrastedButton("New Custom type"))
		{
			m_CustomTypeInputTexts.emplace_back();
		}

		auto it_to_erase = m_CustomTypeInputTexts.end();
		for (auto& it = m_CustomTypeInputTexts.begin(); it != m_CustomTypeInputTexts.end(); ++it)
		{
			if (ImGui::ContrastedButton(ICON_NDP_CANCEL))
			{
				it_to_erase = it;
			}

			ImGui::SameLine();

			it->DisplayInputText(aw * 0.5f, "Custom Type :", "SceneCustom");
		}

		if (it_to_erase != m_CustomTypeInputTexts.end())
		{
			m_CustomTypeInputTexts.erase(it_to_erase);
		}

		ImGui::Separator();

		m_SelectedNodeSlotInput = std::dynamic_pointer_cast<NodeSlotInput>(
			m_InputSlotEditor.DrawSlotCreationPane(ImVec2(aw * 0.5f, ImGui::GetFrameHeight() * 10.0f), 
				ProjectFile::Instance()->m_SelectedNode,
				m_SelectedNodeSlotInput,
				NodeSlot::PlaceEnum::INPUT).lock());

		ImGui::SameLine();

		m_SelectedNodeSlotOutput = std::dynamic_pointer_cast<NodeSlotOutput>(
			m_OutputSlotEditor.DrawSlotCreationPane(ImVec2(aw * 0.5f, ImGui::GetFrameHeight() * 10.0f),
				ProjectFile::Instance()->m_SelectedNode,
				m_SelectedNodeSlotOutput,
				NodeSlot::PlaceEnum::OUTPUT).lock());

		ImGui::Separator();

		ImGui::Text("Classes");

		if (m_ClassNameInputText.DisplayInputText(aw * 0.5f, "Name :", "NewClass"))
		{
			nodePtr->m_ClassName = m_ClassNameInputText.GetText();
			ct::replaceString(nodePtr->m_ClassName, " ", "");
			m_ClassNameInputText.SetText(nodePtr->m_ClassName);
		}

		if (m_NodeCategoryNameInputText.DisplayInputText(aw * 0.5f, "Category name :", "TestNodes"))
		{
			nodePtr->m_CategoryName = m_NodeCategoryNameInputText.GetText();
			ct::replaceString(nodePtr->m_CategoryName, " ", "");
			m_NodeCategoryNameInputText.SetText(nodePtr->m_CategoryName);
		}
		
		ImGui::Separator();

		ImGui::CheckBoxBoolDefault("Generate a Module ?", &nodePtr->m_GenerateAModule, true);

		if (nodePtr->m_GenerateAModule)
		{
			if (m_ModuleDisplayNameInputText.DisplayInputText(aw * 0.5f, "Module Display Name :", "New Node"))
			{
				nodePtr->m_ModuleDisplayName = m_ModuleDisplayNameInputText.GetText();
				m_ModuleDisplayNameInputText.SetText(nodePtr->m_ModuleDisplayName);
			}

			if (m_ModuleXmlNameInputText.DisplayInputText(aw * 0.5f, "Module Xml Name :", "toto_module"))
			{
				nodePtr->m_ModuleXmlName = m_ModuleXmlNameInputText.GetText();
				ct::replaceString(nodePtr->m_ModuleXmlName, " ", "");
				m_ModuleXmlNameInputText.SetText(nodePtr->m_ModuleXmlName);
			}

			ImGui::Text("Renderer Type");

			ImGui::SameLine();

			if (ImGui::RadioButtonLabeled(0.0f, RENDERER_TYPE_NONE, nodePtr->m_RendererType == RENDERER_TYPE_NONE, false))
				nodePtr->m_RendererType = RENDERER_TYPE_NONE;
			ImGui::SameLine();
			if (ImGui::RadioButtonLabeled(0.0f, RENDERER_TYPE_PIXEL_2D, nodePtr->m_RendererType == RENDERER_TYPE_PIXEL_2D, false))
				nodePtr->m_RendererType = RENDERER_TYPE_PIXEL_2D;
			ImGui::SameLine();
			if (ImGui::RadioButtonLabeled(0.0f, RENDERER_TYPE_COMPUTE_1D, nodePtr->m_RendererType == RENDERER_TYPE_COMPUTE_1D, false))
				nodePtr->m_RendererType = RENDERER_TYPE_COMPUTE_1D;
			ImGui::SameLine();
			if (ImGui::RadioButtonLabeled(0.0f, RENDERER_TYPE_COMPUTE_2D, nodePtr->m_RendererType == RENDERER_TYPE_COMPUTE_2D, false))
				nodePtr->m_RendererType = RENDERER_TYPE_COMPUTE_2D;
			ImGui::SameLine();
			if (ImGui::RadioButtonLabeled(0.0f, RENDERER_TYPE_COMPUTE_3D, nodePtr->m_RendererType == RENDERER_TYPE_COMPUTE_3D, false))
				nodePtr->m_RendererType = RENDERER_TYPE_COMPUTE_3D;
			ImGui::SameLine();
			if (ImGui::RadioButtonLabeled(0.0f, RENDERER_TYPE_RTX, nodePtr->m_RendererType == RENDERER_TYPE_RTX, false))
				nodePtr->m_RendererType = RENDERER_TYPE_RTX;
		}

		ImGui::CheckBoxBoolDefault("Generate a Pass ?", &nodePtr->m_GenerateAPass, true);

		if (nodePtr->m_GenerateAPass)
		{
			if (nodePtr->m_RendererType == RENDERER_TYPE_PIXEL_2D)
			{
				ImGui::Text("Renderer Specialization Type");

				ImGui::SameLine();

				if (ImGui::RadioButtonLabeled(0.0f, RENDERER_TYPE_PIXEL_2D_SPECIALIZATION_QUAD,
					nodePtr->m_RendererTypePixel2DSpecializationType == RENDERER_TYPE_PIXEL_2D_SPECIALIZATION_QUAD, false))
					nodePtr->m_RendererTypePixel2DSpecializationType = RENDERER_TYPE_PIXEL_2D_SPECIALIZATION_QUAD;
				ImGui::SameLine();
				if (ImGui::RadioButtonLabeled(0.0f, RENDERER_TYPE_PIXEL_2D_SPECIALIZATION_MESH,
					nodePtr->m_RendererTypePixel2DSpecializationType == RENDERER_TYPE_PIXEL_2D_SPECIALIZATION_MESH, false))
					nodePtr->m_RendererTypePixel2DSpecializationType = RENDERER_TYPE_PIXEL_2D_SPECIALIZATION_MESH;
				ImGui::SameLine();
				if (ImGui::RadioButtonLabeled(0.0f, RENDERER_TYPE_PIXEL_2D_SPECIALIZATION_TESSELATION,
					nodePtr->m_RendererTypePixel2DSpecializationType == RENDERER_TYPE_PIXEL_2D_SPECIALIZATION_TESSELATION, false))
					nodePtr->m_RendererTypePixel2DSpecializationType = RENDERER_TYPE_PIXEL_2D_SPECIALIZATION_TESSELATION;
				ImGui::SameLine();
				if (ImGui::RadioButtonLabeled(0.0f, RENDERER_TYPE_PIXEL_2D_SPECIALIZATION_VERTEX,
					nodePtr->m_RendererTypePixel2DSpecializationType == RENDERER_TYPE_PIXEL_2D_SPECIALIZATION_VERTEX, false))
					nodePtr->m_RendererTypePixel2DSpecializationType = RENDERER_TYPE_PIXEL_2D_SPECIALIZATION_VERTEX;
			}

			if (nodePtr->m_RendererTypePixel2DSpecializationType == RENDERER_TYPE_PIXEL_2D_SPECIALIZATION_MESH ||
				nodePtr->m_RendererTypePixel2DSpecializationType == RENDERER_TYPE_PIXEL_2D_SPECIALIZATION_TESSELATION)
			{
				ImGui::ContrastedComboVectorDefault(0.0f,
					"VertexStruct type", &nodePtr->m_VertexStructTypesIndex, nodePtr->m_BaseTypes.m_VertexStructTypes, 0);
			}


			nodePtr->m_UBOEditors.DrawPane(nodePtr->m_RendererType);

			/*ImGui::CheckBoxBoolDefault("Use A SBO", &nodePtr->m_UseASbo, false);
			if (nodePtr->m_UseASbo)
			{
				
			}*/
		}

		if (ImGui::ContrastedButton("Generate"))
		{
			ImGuiFileDialog::Instance()->OpenDialog("GenerateToPath", "Generate To Path", 
				nullptr, ProjectFile::Instance()->m_GenerationRootPath);
		}
	}
}

void MainFrame::SelectNode(const BaseNodeWeak& vNode)
{
	auto currentNodePtr = std::dynamic_pointer_cast<GeneratorNode>(ProjectFile::Instance()->m_SelectedNode.lock());
	auto nodePtr = std::dynamic_pointer_cast<GeneratorNode>(vNode.lock());
	if (nodePtr)
	{
		ProjectFile::Instance()->m_SelectedNode = nodePtr;

		m_NodeDisplayNameInputText.SetText(nodePtr->m_NodeDisplayName);
		m_NodeCreationNameInputText.SetText(nodePtr->m_NodeCreationName);
		m_NodeCategoryNameInputText.SetText(nodePtr->m_CategoryName);
		m_ClassNameInputText.SetText(nodePtr->m_ClassName);
		m_ModuleDisplayNameInputText.SetText(nodePtr->m_ModuleDisplayName);
		m_ModuleXmlNameInputText.SetText(nodePtr->m_ModuleXmlName);

		if (currentNodePtr && nodePtr != currentNodePtr)
		{
			NodeSlot::sSlotGraphOutputMouseLeft.reset();
			m_SelectedNodeSlotInput.reset();

			// selection of the first slot
			if (!nodePtr->m_Inputs.empty())
			{
				SelectSlot(nodePtr->m_Inputs.begin()->second, ImGuiMouseButton_Left);
			}

			NodeSlot::sSlotGraphOutputMouseRight.reset();
			m_SelectedNodeSlotOutput.reset();
			
			// selection of the first slot
			if (!nodePtr->m_Outputs.empty())
			{
				SelectSlot(nodePtr->m_Outputs.begin()->second, ImGuiMouseButton_Left);
			}
		}
	}
}

void MainFrame::SelectSlot(const NodeSlotWeak& vSlot, const ImGuiMouseButton& vButton)
{
	if (ProjectFile::Instance()->m_RootNodePtr)
	{
		if (vButton == ImGuiMouseButton_Left)
		{
			auto slotPtr = vSlot.lock();
			if (slotPtr)
			{
				if (slotPtr->IsAnInput())
				{
					m_SelectedNodeSlotInput = std::dynamic_pointer_cast<NodeSlotInput>(vSlot.lock());
					NodeSlot::sSlotGraphOutputMouseLeft = m_SelectedNodeSlotInput;
					m_InputSlotEditor.SelectSlot(m_SelectedNodeSlotInput);
				}
				else if (slotPtr->IsAnOutput())
				{
					m_SelectedNodeSlotOutput = std::dynamic_pointer_cast<NodeSlotOutput>(vSlot.lock());
					NodeSlot::sSlotGraphOutputMouseRight = m_SelectedNodeSlotOutput;
					m_OutputSlotEditor.SelectSlot(m_SelectedNodeSlotOutput);
				}
			}
		}
		else if (vButton == ImGuiMouseButton_Middle)
		{

		}
		else if (vButton == ImGuiMouseButton_Right)
		{

		}
	}
}

bool MainFrame::SaveProject()
{
	return ProjectFile::Instance()->Save();
}

void MainFrame::SaveAsProject(const std::string& vFilePathName)
{
	ProjectFile::Instance()->SaveAs(vFilePathName);

	if (m_NeedToCloseApp)
	{
		glfwSetWindowShouldClose(m_Window, 1); // close app
	}
}

///////////////////////////////////////////////////////
//// SAVE DIALOG WHEN UN SAVED CHANGES ////////////////
///////////////////////////////////////////////////////

void MainFrame::OpenUnSavedDialog()
{
	// force close dialog if any dialog is opened
	ImGuiFileDialog::Instance()->Close();

	m_SaveDialogIfRequired = true;
}

void MainFrame::CloseUnSavedDialog()
{
	m_SaveDialogIfRequired = false;
}

bool MainFrame::ShowUnSavedDialog()
{
	bool res = false;

	if (m_SaveDialogIfRequired)
	{
		if (ProjectFile::Instance()->IsLoaded())
		{
			if (ProjectFile::Instance()->IsThereAnyNotSavedChanged())
			{
				/*
				Unsaved dialog behavior :
				-	save :
					-	insert action : save project
				-	save as :
					-	insert action : save as project
				-	continue without saving :
					-	quit unsaved dialog
				-	cancel :
					-	clear actions
				*/

				ImGui::CloseCurrentPopup();
				ImGui::OpenPopup("Do you want to save before ?");
				if (ImGui::BeginPopupModal("Do you want to save before ?", (bool*)0,
					ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking))
				{
					if (ImGui::Button("Save"))
					{
						res = Action_UnSavedDialog_SaveProject();
					}
					ImGui::SameLine();
					if (ImGui::Button("Save As"))
					{
						Action_UnSavedDialog_SaveAsProject();
					}

					if (ImGui::Button("Continue without saving"))
					{
						res = true; // quit the action
					}
					ImGui::SameLine();
					if (ImGui::Button("Cancel"))
					{
						Action_Cancel();
					}

					ImGui::EndPopup();
				}
			}
		}

		return res; // quit if true, else continue on the next frame
	}

	return true; // quit the action
}

///////////////////////////////////////////////////////
//// ACTIONS //////////////////////////////////////////
///////////////////////////////////////////////////////

void MainFrame::Action_Menu_NewProject()
{
	/*
	new project :
	-	unsaved :
		-	add action : show unsaved dialog
		-	add action : open dialog for new project file name
	-	saved :
		-	add action : open dialog for new project file name
	*/
	m_ActionSystem.Clear();
	Action_OpenUnSavedDialog_IfNeeded();
	m_ActionSystem.Add([this]()
		{
			CloseUnSavedDialog();
			ImGuiFileDialog::Instance()->OpenDialog(
				"NewProjectDlg", "New Project File", ".lumcg", "", 1, nullptr, ImGuiFileDialogFlags_Modal);
			return true;
		});
	m_ActionSystem.Add([this]()
		{
			return Display_NewProjectDialog();
		});
}

void MainFrame::Action_Menu_OpenProject()
{
	/*
	open project :
	-	unsaved :
		-	add action : show unsaved dialog
		-	add action : open project
	-	saved :
		-	add action : open project
	*/
	m_ActionSystem.Clear();
	Action_OpenUnSavedDialog_IfNeeded();
	m_ActionSystem.Add([this]()
		{
			CloseUnSavedDialog();
			ImGuiFileDialog::Instance()->OpenDialog(
				"OpenProjectDlg", "Open Project File", ".lumcg", "", 1, nullptr, ImGuiFileDialogFlags_Modal);
			return true;
		});
	m_ActionSystem.Add([this]()
		{
			return Display_OpenProjectDialog();
		});
}

void MainFrame::Action_Menu_ReOpenProject()
{
	/*
	re open project :
	-	unsaved :
		-	add action : show unsaved dialog
		-	add action : re open project
	-	saved :
		-	add action : re open project
	*/
	m_ActionSystem.Clear();
	Action_OpenUnSavedDialog_IfNeeded();
	m_ActionSystem.Add([this]()
		{
			NeedToLoadProject(ProjectFile::Instance()->GetProjectFilepathName());
			return true;
		});
}

void MainFrame::Action_Menu_SaveProject()
{
	/*
	save project :
	-	never saved :
		-	add action : save as project
	-	saved in a file beofre :
		-	add action : save project
	*/
	m_ActionSystem.Clear();
	m_ActionSystem.Add([this]()
		{
			if (!SaveProject())
			{
				CloseUnSavedDialog();
				ImGuiFileDialog::Instance()->OpenDialog(
					"SaveProjectDlg", "Save Project File", ".lumcg", "", 1, nullptr, ImGuiFileDialogFlags_Modal);
			}
			return true;
		});
	m_ActionSystem.Add([this]()
		{
			return Display_SaveProjectDialog();
		});
}

void MainFrame::Action_Menu_SaveAsProject()
{
	/*
	save as project :
	-	add action : save as project
	*/
	m_ActionSystem.Clear();
	m_ActionSystem.Add([this]()
		{
			CloseUnSavedDialog();
			ImGuiFileDialog::Instance()->OpenDialog(
				"SaveProjectDlg", "Save Project File", ".lumcg", "",
				1, nullptr, ImGuiFileDialogFlags_ConfirmOverwrite | ImGuiFileDialogFlags_Modal);
			return true;
		});
	m_ActionSystem.Add([this]()
		{
			return Display_SaveProjectDialog();
		});
}

void MainFrame::Action_Menu_CloseProject()
{
	/*
	Close project :
	-	unsaved :
		-	add action : show unsaved dialog
		-	add action : Close project
	-	saved :
		-	add action : Close project
	*/
	m_ActionSystem.Clear();
	Action_OpenUnSavedDialog_IfNeeded();
	m_ActionSystem.Add([this]()
		{
			NeedToCloseProject();
			return true;
		});
}

void MainFrame::Action_Window_CloseApp()
{
	if (m_NeedToCloseApp) return; // block next call to close app when running
/*
Close app :
-	unsaved :
	-	add action : show unsaved dialog
	-	add action : Close app
-	saved :
	-	add action : Close app
*/
	m_NeedToCloseApp = true;

	m_ActionSystem.Clear();
	Action_OpenUnSavedDialog_IfNeeded();
	m_ActionSystem.Add([this]()
		{
			glfwSetWindowShouldClose(m_Window, 1); // close app
			return true;
		});
}

void MainFrame::Action_OpenUnSavedDialog_IfNeeded()
{
	if (ProjectFile::Instance()->IsLoaded() &&
		ProjectFile::Instance()->IsThereAnyNotSavedChanged())
	{
		OpenUnSavedDialog();
		m_ActionSystem.Add([this]()
			{
				return ShowUnSavedDialog();
			});
	}
}

void MainFrame::Action_Cancel()
{
	/*
	-	cancel :
		-	clear actions
	*/
	CloseUnSavedDialog();
	m_ActionSystem.Clear();
	m_NeedToCloseApp = false;
}

bool MainFrame::Action_UnSavedDialog_SaveProject()
{
	bool res = SaveProject();
	if (!res)
	{
		m_ActionSystem.Insert([this]()
			{
				return Display_SaveProjectDialog();
			});
		m_ActionSystem.Insert([this]()
			{
				CloseUnSavedDialog();
				ImGuiFileDialog::Instance()->OpenDialog(
					"SaveProjectDlg", "Save Project File", ".lumcg",
					".", 1, nullptr, ImGuiFileDialogFlags_ConfirmOverwrite | ImGuiFileDialogFlags_Modal);
				return true;
			});
	}
	return res;
}

void MainFrame::Action_UnSavedDialog_SaveAsProject()
{
	m_ActionSystem.Insert([this]()
		{
			return Display_SaveProjectDialog();
		});
	m_ActionSystem.Insert([this]()
		{
			CloseUnSavedDialog();
			ImGuiFileDialog::Instance()->OpenDialog(
				"SaveProjectDlg", "Save Project File", ".lumcg",
				".", 1, nullptr, ImGuiFileDialogFlags_ConfirmOverwrite | ImGuiFileDialogFlags_Modal);
			return true;
		});
}

void MainFrame::Action_UnSavedDialog_Cancel()
{
	Action_Cancel();
}

///////////////////////////////////////////////////////
//// DIALOG FUNCS /////////////////////////////////////
///////////////////////////////////////////////////////

bool MainFrame::Display_NewProjectDialog()
{
	// need to return false to continue to be displayed next frame

	ImVec2 min = MainFrame::Instance()->m_DisplaySize * 0.5f;
	ImVec2 max = MainFrame::Instance()->m_DisplaySize;

	if (ImGuiFileDialog::Instance()->Display("NewProjectDlg",
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking, min, max))
	{
		if (ImGuiFileDialog::Instance()->IsOk())
		{
			CloseUnSavedDialog();
			auto file = ImGuiFileDialog::Instance()->GetFilePathName();
			NeedToNewProject(file);
		}
		else // cancel
		{
			Action_Cancel(); // we interrupts all actions
		}

		ImGuiFileDialog::Instance()->Close();

		return true;
	}

	return false;
}

bool MainFrame::Display_OpenProjectDialog()
{
	// need to return false to continue to be displayed next frame

	ImVec2 min = MainFrame::Instance()->m_DisplaySize * 0.5f;
	ImVec2 max = MainFrame::Instance()->m_DisplaySize;

	if (ImGuiFileDialog::Instance()->Display("OpenProjectDlg",
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking, min, max))
	{
		if (ImGuiFileDialog::Instance()->IsOk())
		{
			CloseUnSavedDialog();
			NeedToLoadProject(ImGuiFileDialog::Instance()->GetFilePathName());
		}
		else // cancel
		{
			Action_Cancel(); // we interrupts all actions
		}

		ImGuiFileDialog::Instance()->Close();

		return true;
	}

	return false;
}

bool MainFrame::Display_SaveProjectDialog()
{
	// need to return false to continue to be displayed next frame

	ImVec2 min = MainFrame::Instance()->m_DisplaySize * 0.5f;
	ImVec2 max = MainFrame::Instance()->m_DisplaySize;

	if (ImGuiFileDialog::Instance()->Display("SaveProjectDlg",
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking, min, max))
	{
		if (ImGuiFileDialog::Instance()->IsOk())
		{
			CloseUnSavedDialog();
			SaveAsProject(ImGuiFileDialog::Instance()->GetFilePathName());
		}
		else // cancel
		{
			Action_Cancel(); // we interrupts all actions
		}

		ImGuiFileDialog::Instance()->Close();

		return true;
	}

	return false;
}

///////////////////////////////////////////////////////
//// CONFIGURATION ////////////////////////////////////
///////////////////////////////////////////////////////

std::string MainFrame::getXml(const std::string& vOffset, const std::string& vUserDatas)
{
	UNUSED(vUserDatas);

	std::string str;

	str += ThemeHelper::Instance()->getXml(vOffset);
	str += vOffset + "<bookmarks>" + ImGuiFileDialog::Instance()->SerializeBookmarks() + "</bookmarks>\n";
	str += vOffset + "<showaboutdialog>" + (m_ShowAboutDialog ? "true" : "false") + "</showaboutdialog>\n";
	str += vOffset + "<showimgui>" + (m_ShowImGui ? "true" : "false") + "</showimgui>\n";
	str += vOffset + "<showmetric>" + (m_ShowMetric ? "true" : "false") + "</showmetric>\n";
	str += vOffset + "<project>" + ProjectFile::Instance()->GetProjectFilepathName() + "</project>\n";
	str += vOffset + "<custom_types>\n";
	for (const auto& it : m_CustomTypeInputTexts)
	{
		str += vOffset + "\t<custom_type_name>" + it.GetText() + "</custom_type_name>\n";
	}
	str += vOffset + "</custom_types>\n";

	return str;
}

bool MainFrame::setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& vUserDatas)
{
	UNUSED(vUserDatas);

	// The value of this child identifies the name of this element
	std::string strName;
	std::string strValue;
	std::string strParentName;

	strName = vElem->Value();
	if (vElem->GetText())
		strValue = vElem->GetText();
	if (vParent != 0)
		strParentName = vParent->Value();

	ThemeHelper::Instance()->setFromXml(vElem, vParent);

	if (strName == "bookmarks")
		ImGuiFileDialog::Instance()->DeserializeBookmarks(strValue);
	else if (strName == "project")
	{
		m_NeedToLoadProject = true;
		m_FilePathNameToLoad = strValue;
	}
	else if (strName == "showaboutdialog")
		m_ShowAboutDialog = ct::ivariant(strValue).GetB();
	else if (strName == "showimgui")
		m_ShowImGui = ct::ivariant(strValue).GetB();
	else if (strName == "showmetric")
		m_ShowMetric = ct::ivariant(strValue).GetB();

	if (strParentName == "custom_types" && 
		strName == "custom_type_name")
	{
		m_CustomTypeInputTexts.emplace_back(strValue);
	}

	return true;
}

bool MainFrame::LoadNodeFromXML(
	BaseNodeWeak vBaseNodeWeak,
	tinyxml2::XMLElement* vElem,
	tinyxml2::XMLElement* vParent,
	const std::string& vNodeName,
	const std::string& vNodeType,
	const ct::fvec2& vPos,
	const size_t& vNodeId)
{
	ZoneScoped;

	bool continueXMLParsing = true;

	if (ProjectFile::Instance()->m_RootNodePtr)
	{
		GeneratorNodePtr nodePtr = GeneratorNode::Create(m_VulkanCorePtr);

		if (nodePtr)
		{
			if (!vNodeName.empty())
				nodePtr->name = vNodeName;
			nodePtr->pos = ImVec2(vPos.x, vPos.y);
			nodePtr->nodeID = vNodeId;

			ProjectFile::Instance()->m_RootNodePtr->AddChildNode(nodePtr);
			nd::SetNodePosition(vNodeId, nodePtr->pos);
			nodePtr->RecursParsingConfigChilds(vElem);

			// pour eviter que des slots aient le meme id qu'un nodePtr
			BaseNode::freeNodeId = ct::maxi<uint32_t>(BaseNode::freeNodeId, (uint32_t)vNodeId);

			continueXMLParsing = true;
		}
	}

	return continueXMLParsing;
}