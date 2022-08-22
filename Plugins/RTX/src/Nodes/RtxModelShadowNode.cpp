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

#include "RtxModelShadowNode.h"
#include <Modules/RtxModelShadowModule.h>
#include <Interfaces/LightGroupOutputInterface.h>
#include <Interfaces/AccelStructureOutputInterface.h>
#include <Slots/NodeSlotAccelStructureInput.h>
#include <Graph/Slots/NodeSlotLightGroupInput.h>
#include <Graph/Slots/NodeSlotTextureOutput.h>

std::shared_ptr<RtxModelShadowNode> RtxModelShadowNode::Create(vkApi::VulkanCorePtr vVulkanCorePtr)
{
	auto res = std::make_shared<RtxModelShadowNode>();
	res->m_This = res;
	if (!res->Init(vVulkanCorePtr))
	{
		res.reset();
	}
	return res;
}

RtxModelShadowNode::RtxModelShadowNode() : BaseNode()
{
	m_NodeTypeString = "RTX_MODEL_SHADOW";
}

RtxModelShadowNode::~RtxModelShadowNode()
{
	Unit();
}

bool RtxModelShadowNode::Init(vkApi::VulkanCorePtr vVulkanCorePtr)
{
	name = "Rtx Model Shadow";

	AddInput(NodeSlotAccelStructureInput::Create("Accel Struct"), true, false);
	AddInput(NodeSlotLightGroupInput::Create("Lights"), true, false);
	AddOutput(NodeSlotTextureOutput::Create("Output", 0U), true, true);

	bool res = false;

	m_RtxModelShadowModulePtr = RtxModelShadowModule::Create(vVulkanCorePtr);
	if (m_RtxModelShadowModulePtr)
	{
		res = true;
	}

	return res;
}

void RtxModelShadowNode::Unit()
{
	m_RtxModelShadowModulePtr.reset();
}

bool RtxModelShadowNode::ExecuteAllTime(const uint32_t& vCurrentFrame, vk::CommandBuffer* vCmd, BaseNodeState* vBaseNodeState)
{
	BaseNode::ExecuteChilds(vCurrentFrame, vCmd, vBaseNodeState);

	if (m_RtxModelShadowModulePtr)
	{
		return m_RtxModelShadowModulePtr->Execute(vCurrentFrame, vCmd, vBaseNodeState);
	}

	return false;
}

bool RtxModelShadowNode::DrawWidgets(const uint32_t& vCurrentFrame, ImGuiContext* vContext)
{
	assert(vContext); ImGui::SetCurrentContext(vContext);

	if (m_RtxModelShadowModulePtr)
	{
		return m_RtxModelShadowModulePtr->DrawWidgets(vCurrentFrame, vContext);
	}

	return false;
}

void RtxModelShadowNode::DisplayDialogsAndPopups(const uint32_t& vCurrentFrame, const ct::ivec2& vMaxSize, ImGuiContext* vContext)
{
	assert(vContext); ImGui::SetCurrentContext(vContext);

	if (m_RtxModelShadowModulePtr)
	{
		m_RtxModelShadowModulePtr->DisplayDialogsAndPopups(vCurrentFrame, vMaxSize, vContext);
	}
}

void RtxModelShadowNode::DisplayInfosOnTopOfTheNode(BaseNodeState* vBaseNodeState)
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

void RtxModelShadowNode::NeedResizeByResizeEvent(ct::ivec2* vNewSize, const uint32_t* vCountColorBuffers)
{
	if (m_RtxModelShadowModulePtr)
	{
		m_RtxModelShadowModulePtr->NeedResizeByResizeEvent(vNewSize, vCountColorBuffers);
	}

	// on fait ca apres
	BaseNode::NeedResizeByResizeEvent(vNewSize, vCountColorBuffers);
}

vk::DescriptorImageInfo* RtxModelShadowNode::GetDescriptorImageInfo(const uint32_t& vBindingPoint, ct::fvec2* vOutSize)
{
	if (m_RtxModelShadowModulePtr)
	{
		return m_RtxModelShadowModulePtr->GetDescriptorImageInfo(vBindingPoint, vOutSize);
	}

	return nullptr;
}

void RtxModelShadowNode::SetAccelStructure(SceneAccelStructureWeak vSceneAccelStructure)
{
	if (m_RtxModelShadowModulePtr)
	{
		m_RtxModelShadowModulePtr->SetAccelStructure(vSceneAccelStructure);
	}
}

void RtxModelShadowNode::SetLightGroup(SceneLightGroupWeak vSceneLightGroup)
{
	if (m_RtxModelShadowModulePtr)
	{
		m_RtxModelShadowModulePtr->SetLightGroup(vSceneLightGroup);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////
//// CONFIGURATION ///////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

std::string RtxModelShadowNode::getXml(const std::string& vOffset, const std::string& vUserDatas)
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
			(uint32_t)nodeID.Get());

		for (auto slot : m_Inputs)
		{
			res += slot.second->getXml(vOffset + "\t", vUserDatas);
		}
			
		for (auto slot : m_Outputs)
		{
			res += slot.second->getXml(vOffset + "\t", vUserDatas);
		}

		if (m_RtxModelShadowModulePtr)
		{
			res += m_RtxModelShadowModulePtr->getXml(vOffset + "\t", vUserDatas);
		}

		res += vOffset + "</node>\n";
	}

	return res;
}

bool RtxModelShadowNode::setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& vUserDatas)
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

	if (m_RtxModelShadowModulePtr)
	{
		m_RtxModelShadowModulePtr->setFromXml(vElem, vParent, vUserDatas);
	}

	return true;
}

void RtxModelShadowNode::UpdateShaders(const std::set<std::string>& vFiles)
{
	if (m_RtxModelShadowModulePtr)
	{
		m_RtxModelShadowModulePtr->UpdateShaders(vFiles);
	}
}