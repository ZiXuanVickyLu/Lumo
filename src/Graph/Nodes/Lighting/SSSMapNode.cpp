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

#include "SSSMapNode.h"
#include <Modules/Lighting/SSSMapModule.h>
#include <Interfaces/ModelOutputInterface.h>

std::shared_ptr<SSSMapNode> SSSMapNode::Create(vkApi::VulkanCore* vVulkanCore)
{
	auto res = std::make_shared<SSSMapNode>();
	res->m_This = res;
	if (!res->Init(vVulkanCore))
	{
		res.reset();
	}
	return res;
}

SSSMapNode::SSSMapNode() : BaseNode()
{
	m_NodeType = NodeTypeEnum::SSS_MAPPING;
}

SSSMapNode::~SSSMapNode()
{
	Unit();
}

bool SSSMapNode::Init(vkApi::VulkanCore* vVulkanCore)
{
	name = "SSS Map";

	NodeSlot slot;

	slot.slotType = NodeSlotTypeEnum::LIGHT;
	slot.name = "Light";
	AddInput(slot, true, false);

	slot.slotType = NodeSlotTypeEnum::MESH;
	slot.name = "Mesh";
	AddInput(slot, true, false);

	slot.slotType = NodeSlotTypeEnum::LIGHT;
	slot.name = "Light";
	AddOutput(slot, true, true);

	slot.slotType = NodeSlotTypeEnum::DEPTH;
	slot.name = "Output";
	slot.descriptorBinding = 0U;
	AddOutput(slot, true, true);

	bool res = false;
	m_SSSMapModulePtr = SSSMapModule::Create(vVulkanCore);
	if (m_SSSMapModulePtr)
	{
		res = true;
	}

	return res;
}

void SSSMapNode::Unit()
{
	m_SSSMapModulePtr.reset();
}

bool SSSMapNode::Execute(const uint32_t& vCurrentFrame, vk::CommandBuffer* vCmd)
{
	BaseNode::ExecuteChilds(vCurrentFrame, vCmd);

	if (m_SSSMapModulePtr)
	{
		return m_SSSMapModulePtr->Execute(vCurrentFrame, vCmd);
	}
	return false;
}

bool SSSMapNode::DrawWidgets(const uint32_t& vCurrentFrame, ImGuiContext* vContext)
{
	if (m_SSSMapModulePtr)
	{
		return m_SSSMapModulePtr->DrawWidgets(vCurrentFrame, vContext);
	}

	return false;
}

void SSSMapNode::DrawOverlays(const uint32_t& vCurrentFrame, const ct::frect& vRect, ImGuiContext* vContext)
{
	if (m_SSSMapModulePtr)
	{
		m_SSSMapModulePtr->DrawOverlays(vCurrentFrame, vRect, vContext);
	}
}

void SSSMapNode::SetModel(SceneModelWeak vSceneModel)
{
	if (m_SSSMapModulePtr)
	{
		m_SSSMapModulePtr->SetModel(vSceneModel);
	}
}

void SSSMapNode::SetLightGroup(SceneLightGroupWeak vSceneLightGroup)
{
	if (m_SSSMapModulePtr)
	{
		m_SSSMapModulePtr->SetLightGroup(vSceneLightGroup);
	}
}

SceneLightGroupWeak SSSMapNode::GetLightGroup()
{
	if (m_SSSMapModulePtr)
	{
		return m_SSSMapModulePtr->GetLightGroup();
	}

	return SceneLightGroupWeak();
}

vk::DescriptorImageInfo* SSSMapNode::GetDescriptorImageInfo(const uint32_t& vBindingPoint)
{
	if (m_SSSMapModulePtr)
	{
		return m_SSSMapModulePtr->GetDescriptorImageInfo(vBindingPoint);
	}

	return nullptr;
}

// le start est toujours le slot de ce node, l'autre le slot du node connect�
void SSSMapNode::JustConnectedBySlots(NodeSlotWeak vStartSlot, NodeSlotWeak vEndSlot)
{
	auto startSlotPtr = vStartSlot.getValidShared();
	auto endSlotPtr = vEndSlot.getValidShared();
	if (startSlotPtr && endSlotPtr && m_SSSMapModulePtr)
	{
		if (startSlotPtr->IsAnInput())
		{
			if (startSlotPtr->slotType == NodeSlotTypeEnum::MESH)
			{
				auto otherModelNodePtr = dynamic_pointer_cast<ModelOutputInterface>(endSlotPtr->parentNode.getValidShared());
				if (otherModelNodePtr)
				{
					SetModel(otherModelNodePtr->GetModel());
				}
			}
			else if (startSlotPtr->slotType == NodeSlotTypeEnum::LIGHT)
			{
				auto otherLightGroupNodePtr = dynamic_pointer_cast<LightOutputInterface>(endSlotPtr->parentNode.getValidShared());
				if (otherLightGroupNodePtr)
				{
					SetLightGroup(otherLightGroupNodePtr->GetLightGroup());
				}
			}
		}
	}
}

// le start est toujours le slot de ce node, l'autre le slot du node connect�
void SSSMapNode::JustDisConnectedBySlots(NodeSlotWeak vStartSlot, NodeSlotWeak vEndSlot)
{
	auto startSlotPtr = vStartSlot.getValidShared();
	auto endSlotPtr = vEndSlot.getValidShared();
	if (startSlotPtr && endSlotPtr && m_SSSMapModulePtr)
	{
		if (startSlotPtr->IsAnInput())
		{
			if (startSlotPtr->slotType == NodeSlotTypeEnum::MESH)
			{
				SetModel();
			}
			else if (startSlotPtr->slotType == NodeSlotTypeEnum::LIGHT)
			{
				SetLightGroup();
			}
		}
	}
}

void SSSMapNode::UpdateShaders(const std::set<std::string>& vFiles)
{
	if (m_SSSMapModulePtr)
	{
		return m_SSSMapModulePtr->UpdateShaders(vFiles);
	}
}

void SSSMapNode::Notify(const NotifyEvent& vEvent, const NodeSlotWeak& vEmmiterSlot, const NodeSlotWeak& vReceiverSlot)
{
	switch (vEvent)
	{
	case NotifyEvent::ModelUpdateDone:
	{
		auto emiterSlotPtr = vEmmiterSlot.getValidShared();
		if (emiterSlotPtr)
		{
			if (emiterSlotPtr->IsAnOutput())
			{
				auto otherNodePtr = dynamic_pointer_cast<ModelOutputInterface>(emiterSlotPtr->parentNode.getValidShared());
				if (otherNodePtr)
				{
					SetModel(otherNodePtr->GetModel());
				}
			}
		}
		break;
	}
	case NotifyEvent::LightUpdateDone:
	{
		// maj dans ce node
		auto emiterSlotPtr = vEmmiterSlot.getValidShared();
		if (emiterSlotPtr)
		{
			if (emiterSlotPtr->IsAnOutput())
			{
				auto otherNodePtr = dynamic_pointer_cast<LightOutputInterface>(emiterSlotPtr->parentNode.getValidShared());
				if (otherNodePtr)
				{
					SetLightGroup(otherNodePtr->GetLightGroup());
				}
			}
		}

		// propagation en output
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

void SSSMapNode::NeedResize(ct::ivec2* vNewSize, const uint32_t* vCountColorBuffer)
{
	if (m_SSSMapModulePtr)
	{
		m_SSSMapModulePtr->NeedResize(vNewSize, vCountColorBuffer);
	}

	// on fait ca apres
	BaseNode::NeedResize(vNewSize, vCountColorBuffer);
}

//////////////////////////////////////////////////////////////////////////////////////////////
//// CONFIGURATION ///////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

std::string SSSMapNode::getXml(const std::string& vOffset, const std::string& vUserDatas)
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
			Graph::GetStringFromNodeTypeEnum(m_NodeType).c_str(),
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

		if (m_SSSMapModulePtr)
		{
			res += m_SSSMapModulePtr->getXml(vOffset + "\t", vUserDatas);
		}

		res += vOffset + "</node>\n";
	}

	return res;
}

bool SSSMapNode::setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& vUserDatas)
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

	if (m_SSSMapModulePtr)
	{
		m_SSSMapModulePtr->setFromXml(vElem, vParent, vUserDatas);
	}

	return true;
}
