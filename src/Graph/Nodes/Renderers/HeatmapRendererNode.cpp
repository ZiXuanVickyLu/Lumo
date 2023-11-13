/*
Copyright 2022-2023 Stephane Cuillerdier (aka aiekick)

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

#include "HeatmapRendererNode.h"
#include <Graph/Modules/Renderers/HeatmapRenderer.h>
#include <LumoBackend/Interfaces/ModelOutputInterface.h>
#include <LumoBackend/Graph/Slots/NodeSlotModelInput.h>
#include <LumoBackend/Graph/Slots/NodeSlotTextureOutput.h>
#include <LumoBackend/Graph/Slots/NodeSlotShaderPassOutput.h>

#ifdef PROFILER_INCLUDE
#include <Gaia/gaia.h>
#include PROFILER_INCLUDE
#endif
#ifndef ZoneScoped
#define ZoneScoped
#endif

std::shared_ptr<HeatmapRendererNode> HeatmapRendererNode::Create(GaiApi::VulkanCorePtr vVulkanCorePtr)
{
	auto res = std::make_shared<HeatmapRendererNode>();
	res->m_This = res;
	if (!res->Init(vVulkanCorePtr))
	{
		res.reset();
	}
	return res;
}

HeatmapRendererNode::HeatmapRendererNode() : BaseNode()
{
	m_NodeTypeString = "HEATMAP_RENDERER";
}

HeatmapRendererNode::~HeatmapRendererNode()
{
	Unit();
}

bool HeatmapRendererNode::Init(GaiApi::VulkanCorePtr vVulkanCorePtr)
{
	name = "Heatmap";

	AddInput(NodeSlotModelInput::Create("Model"), true, true);
	AddOutput(NodeSlotTextureOutput::Create("Output", 0U), true, true);
	AddOutput(NodeSlotShaderPassOutput::Create("Output", 1U), true, true);

	bool res = false;

	m_HeatmapRenderer = HeatmapRenderer::Create(vVulkanCorePtr);
	if (m_HeatmapRenderer)
	{
		res = true;
	}

	return res;
}

void HeatmapRendererNode::Unit()
{
	m_HeatmapRenderer.reset();
}

bool HeatmapRendererNode::ExecuteAllTime(const uint32_t& vCurrentFrame, vk::CommandBuffer* vCmd, BaseNodeState* vBaseNodeState)
{
	BaseNode::ExecuteInputTasks(vCurrentFrame, vCmd, vBaseNodeState);

	if (m_HeatmapRenderer)
	{
		return m_HeatmapRenderer->Execute(vCurrentFrame, vCmd, vBaseNodeState);
	}
	return false;
}

bool HeatmapRendererNode::DrawWidgets(const uint32_t& vCurrentFrame, ImGuiContext* vContextPtr, const std::string& vUserDatas)
{
	assert(vContextPtr); ImGui::SetCurrentContext(vContextPtr);

	if (m_HeatmapRenderer)
	{
		return m_HeatmapRenderer->DrawWidgets(vCurrentFrame, vContextPtr, vUserDatas);
	}

	return false;
}

bool HeatmapRendererNode::DrawOverlays(
    const uint32_t& vCurrentFrame, const ImRect& vRect, ImGuiContext* vContextPtr, const std::string& vUserDatas) {
    assert(vContextPtr);
    ImGui::SetCurrentContext(vContextPtr);

    return false;
}

bool HeatmapRendererNode::DrawDialogsAndPopups(
    const uint32_t& vCurrentFrame, const ImVec2& vMaxSize, ImGuiContext* vContextPtr, const std::string& vUserDatas) {
	assert(vContextPtr); ImGui::SetCurrentContext(vContextPtr);

	if (m_HeatmapRenderer)
	{
        return m_HeatmapRenderer->DrawDialogsAndPopups(vCurrentFrame, vMaxSize, vContextPtr, vUserDatas);
    }
    return false;
}

void HeatmapRendererNode::DisplayInfosOnTopOfTheNode(BaseNodeState* vBaseNodeState)
{
	if (vBaseNodeState && vBaseNodeState->debug_mode)
	{
		auto drawList = nd::GetNodeBackgroundDrawList(nodeID);
		if (drawList)
		{
			char debugBuffer[255] = "\0";
			snprintf(debugBuffer, 254,
				"Used(%s)\nCell(%i, %i)"/*\nPos(%.1f, %.1f)\nSize(%.1f, %.1f)*/,
				(used ? "true" : "false"), cell.x, cell.y/*, pos.x, pos.y, size.x, size.y*/);
			ImVec2 txtSize = ImGui::CalcTextSize(debugBuffer);
			drawList->AddText(pos - ImVec2(0, txtSize.y), ImGui::GetColorU32(ImGuiCol_Text), debugBuffer);
		}
	}
}

void HeatmapRendererNode::NeedResizeByResizeEvent(ct::ivec2* vNewSize, const uint32_t* vCountColorBuffers)
{
	if (m_HeatmapRenderer)
	{
		m_HeatmapRenderer->NeedResizeByResizeEvent(vNewSize, vCountColorBuffers);
	}

	// on fait ca apres
	BaseNode::NeedResizeByResizeEvent(vNewSize, vCountColorBuffers);
}

void HeatmapRendererNode::SetModel(SceneModelWeak vSceneModel)
{
	if (m_HeatmapRenderer)
	{
		m_HeatmapRenderer->SetModel(vSceneModel);
	}
}

vk::DescriptorImageInfo* HeatmapRendererNode::GetDescriptorImageInfo(const uint32_t& vBindingPoint, ct::fvec2* vOutSize)
{
	if (m_HeatmapRenderer)
	{
		return m_HeatmapRenderer->GetDescriptorImageInfo(vBindingPoint, vOutSize);
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//// SHADER PASS SLOT OUTPUT /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

SceneShaderPassWeak HeatmapRendererNode::GetShaderPasses(const uint32_t& vSlotID)
{
	if (m_HeatmapRenderer)
	{
		return m_HeatmapRenderer->GetShaderPasses(vSlotID);
	}

	return SceneShaderPassWeak();
}

//////////////////////////////////////////////////////////////////////////////////////////////
//// CONFIGURATION ///////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

std::string HeatmapRendererNode::getXml(const std::string& vOffset, const std::string& vUserDatas)
{
	std::string res;

	if (!m_ChildNodes.empty())
	{
		res += BaseNode::getXml(vOffset, vUserDatas);
	}
	else
	{
		res += vOffset + ct::toStr("<node name=\"%s\" type=\"%s\" pos=\"%s\" id=\"%u\">\n",
			name.c_str(),
			m_NodeTypeString.c_str(),
			ct::fvec2(pos.x, pos.y).string().c_str(),
			(uint32_t)GetNodeID());

		for (auto slot : m_Inputs)
		{
			res += slot.second->getXml(vOffset + "\t", vUserDatas);
		}
			
		for (auto slot : m_Outputs)
		{
			res += slot.second->getXml(vOffset + "\t", vUserDatas);
		}

		if (m_HeatmapRenderer)
		{
			res += m_HeatmapRenderer->getXml(vOffset + "\t", vUserDatas);
		}

		res += vOffset + "</node>\n";
	}

	return res;
}

bool HeatmapRendererNode::setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& vUserDatas)
{
	// The value of this child identifies the name of this element
	std::string strName;
	std::string strValue;
	std::string strParentName;

	strName = vElem->Value();
	if (vElem->GetText())
		strValue = vElem->GetText();
	if (vParent != nullptr)
		strParentName = vParent->Value();

	BaseNode::setFromXml(vElem, vParent, vUserDatas);

	if (m_HeatmapRenderer)
	{
		m_HeatmapRenderer->setFromXml(vElem, vParent, vUserDatas);
	}

	return true;
}