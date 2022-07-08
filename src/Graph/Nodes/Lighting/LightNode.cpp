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

#include "LightNode.h"
#include <Modules/Lighting/LightGroupModule.h>

std::shared_ptr<LightNode> LightNode::Create(vkApi::VulkanCorePtr vVulkanCorePtr)
{
	auto res = std::make_shared<LightNode>();
	res->m_This = res;
	if (!res->Init(vVulkanCorePtr))
	{
		res.reset();
	}
	return res;
}

LightNode::LightNode() : BaseNode()
{
	m_NodeTypeString = "LIGHT";
}

LightNode::~LightNode()
{

}

bool LightNode::Init(vkApi::VulkanCorePtr vVulkanCorePtr)
{
	name = "Light";

	NodeSlot slot;
	slot.slotType = NodeSlotTypeEnum::LIGHT;
	slot.name = "Output";
	AddOutput(slot, true, true);

	bool res = false;
	m_LightGroupModulePtr = LightGroupModule::Create(vVulkanCorePtr, m_This);
	if (m_LightGroupModulePtr)
	{
		res = true;
	}

	return res;
}

bool LightNode::ExecuteAllTime(const uint32_t& vCurrentFrame, vk::CommandBuffer* vCmd)
{
	if (m_LightGroupModulePtr)
	{
		return m_LightGroupModulePtr->Execute(vCurrentFrame, vCmd);
	}

	return false;
}

bool LightNode::DrawWidgets(const uint32_t& vCurrentFrame, ImGuiContext* vContext)
{
	if (m_LightGroupModulePtr)
	{
		return m_LightGroupModulePtr->DrawWidgets(vCurrentFrame, vContext);
	}

	return false;
}

void LightNode::DrawOverlays(const uint32_t& vCurrentFrame, const ct::frect& vRect, ImGuiContext* vContext)
{
	if (m_LightGroupModulePtr)
	{
		m_LightGroupModulePtr->DrawOverlays(vCurrentFrame, vRect, vContext);
	}
}

SceneLightGroupWeak LightNode::GetLightGroup()
{
	if (m_LightGroupModulePtr)
	{
		return m_LightGroupModulePtr->GetLightGroup();
	}

	return SceneLightGroupWeak();
}

void LightNode::Notify(const NotifyEvent& vEvent, const NodeSlotWeak& vEmmiterSlot, const NodeSlotWeak& vReceiverSlot)
{
	switch (vEvent)
	{
	case NotifyEvent::LightUpdateDone:
	{
		auto slots = GetOutputSlotsOfType(NodeSlotTypeEnum::LIGHT);
		for (const auto& slot : slots)
		{
			auto slotPtr = slot.getValidShared();
			if (slotPtr)
			{
				slotPtr->Notify(NotifyEvent::LightUpdateDone, slot);
			}
		}
		break;
	}
	default:
		break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////
//// CONFIGURATION ///////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

std::string LightNode::getXml(const std::string& vOffset, const std::string& vUserDatas)
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

		if (m_LightGroupModulePtr)
		{
			res += m_LightGroupModulePtr->getXml(vOffset + "\t", vUserDatas);
		}

		res += vOffset + "</node>\n";
	}

	return res;
}

bool LightNode::setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& vUserDatas)
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

	if (m_LightGroupModulePtr)
	{
		m_LightGroupModulePtr->setFromXml(vElem, vParent, vUserDatas);
	}

	return true;
}