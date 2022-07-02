/*
MIT License

Copyright (c) 2022-2022 Stephane Cuillerdier (aka aiekick)

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

#include "MathNode.h"
#include <Modules/Utils/MathModule.h>

std::shared_ptr<MathNode> MathNode::Create(vkApi::VulkanCore* vVulkanCore)
{
	auto res = std::make_shared<MathNode>();
	res->m_This = res;
	if (!res->Init(vVulkanCore))
	{
		res.reset();
	}
	return res;
}

MathNode::MathNode() : BaseNode()
{
	m_NodeType = NodeTypeEnum::BLUR;
}

MathNode::~MathNode()
{
	Unit();
}

bool MathNode::Init(vkApi::VulkanCore* vVulkanCore)
{
	name = "Math";

	NodeSlot slot;

	slot.slotType = NodeSlotTypeEnum::TEXTURE_2D;
	slot.name = "Input";
	slot.descriptorBinding = 0U;
	AddInput(slot, true, false);

	slot.slotType = NodeSlotTypeEnum::TEXTURE_2D;
	slot.name = "Output";
	slot.descriptorBinding = 0U;
	AddOutput(slot, true, true);

	bool res = false;

	m_MathModulePtr = MathModule::Create(vVulkanCore);
	if (m_MathModulePtr)
	{
		res = true;
	}

	return res;
}

bool MathNode::Execute(const uint32_t& vCurrentFrame, vk::CommandBuffer* vCmd)
{
	BaseNode::ExecuteChilds(vCurrentFrame, vCmd);

	// for update input texture buffer infos => avoid vk crash
	UpdateInputDescriptorImageInfos(m_Inputs);

	if (m_MathModulePtr)
	{
		return m_MathModulePtr->Execute(vCurrentFrame, vCmd);
	}
	return false;
}

bool MathNode::DrawWidgets(const uint32_t& vCurrentFrame, ImGuiContext* vContext)
{
	if (m_MathModulePtr)
	{
		return m_MathModulePtr->DrawWidgets(vCurrentFrame, vContext);
	}

	return false;
}

void MathNode::DisplayDialogsAndPopups(const uint32_t& vCurrentFrame, const ct::ivec2& vMaxSize, ImGuiContext* vContext)
{
	if (m_MathModulePtr)
	{
		m_MathModulePtr->DisplayDialogsAndPopups(vCurrentFrame, vMaxSize, vContext);
	}
}

void MathNode::DisplayInfosOnTopOfTheNode(BaseNodeStateStruct* vCanvasState)
{
	if (vCanvasState && vCanvasState->debug_mode)
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

void MathNode::NeedResize(ct::ivec2* vNewSize, const uint32_t* vCountColorBuffer)
{
	if (m_MathModulePtr)
	{
		m_MathModulePtr->NeedResize(vNewSize, vCountColorBuffer);
	}

	// on fait ca apres
	BaseNode::NeedResize(vNewSize, vCountColorBuffer);
}

// le start est toujours le slot de ce node, l'autre le slot du node connect�
void MathNode::JustConnectedBySlots(NodeSlotWeak vStartSlot, NodeSlotWeak vEndSlot)
{
	auto startSlotPtr = vStartSlot.getValidShared();
	auto endSlotPtr = vEndSlot.getValidShared();
	if (startSlotPtr && endSlotPtr && m_MathModulePtr)
	{
		if (startSlotPtr->IsAnInput())
		{
			if (startSlotPtr->slotType == NodeSlotTypeEnum::TEXTURE_2D)
			{
				auto otherTextureNodePtr = dynamic_pointer_cast<TextureOutputInterface>(endSlotPtr->parentNode.getValidShared());
				if (otherTextureNodePtr)
				{
					SetTexture(startSlotPtr->descriptorBinding, otherTextureNodePtr->GetDescriptorImageInfo(endSlotPtr->descriptorBinding));
				}
			}
		}
	}
}

// le start est toujours le slot de ce node, l'autre le slot du node connect�
void MathNode::JustDisConnectedBySlots(NodeSlotWeak vStartSlot, NodeSlotWeak vEndSlot)
{
	auto startSlotPtr = vStartSlot.getValidShared();
	auto endSlotPtr = vEndSlot.getValidShared();
	if (startSlotPtr && endSlotPtr && m_MathModulePtr)
	{
		if (startSlotPtr->IsAnInput())
		{
			if (startSlotPtr->slotType == NodeSlotTypeEnum::TEXTURE_2D)
			{
				SetTexture(startSlotPtr->descriptorBinding, nullptr);
			}
		}
	}
}

void MathNode::SetTexture(const uint32_t& vBinding, vk::DescriptorImageInfo* vImageInfo)
{
	if (m_MathModulePtr)
	{
		m_MathModulePtr->SetTexture(vBinding, vImageInfo);
	}
}

vk::DescriptorImageInfo* MathNode::GetDescriptorImageInfo(const uint32_t& vBindingPoint)
{
	if (m_MathModulePtr)
	{
		return m_MathModulePtr->GetDescriptorImageInfo(vBindingPoint);
	}

	return nullptr;
}

void MathNode::Notify(const NotifyEvent& vEvent, const NodeSlotWeak& vEmmiterSlot, const NodeSlotWeak& vReceiverSlot)
{
	switch (vEvent)
	{
	case NotifyEvent::TextureUpdateDone:
	{
		auto emiterSlotPtr = vEmmiterSlot.getValidShared();
		if (emiterSlotPtr)
		{
			if (emiterSlotPtr->IsAnOutput())
			{
				auto otherNodePtr = dynamic_pointer_cast<TextureOutputInterface>(emiterSlotPtr->parentNode.getValidShared());
				if (otherNodePtr)
				{
					auto receiverSlotPtr = vReceiverSlot.getValidShared();
					if (receiverSlotPtr)
					{
						SetTexture(receiverSlotPtr->descriptorBinding, otherNodePtr->GetDescriptorImageInfo(emiterSlotPtr->descriptorBinding));
					}
				}
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

std::string MathNode::getXml(const std::string& vOffset, const std::string& vUserDatas)
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

		if (m_MathModulePtr)
		{
			res += m_MathModulePtr->getXml(vOffset + "\t", vUserDatas);
		}

		res += vOffset + "</node>\n";
	}

	return res;
}

bool MathNode::setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& vUserDatas)
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

	if (m_MathModulePtr)
	{
		m_MathModulePtr->setFromXml(vElem, vParent, vUserDatas);
	}

	return true;
}

void MathNode::UpdateShaders(const std::set<std::string>& vFiles)
{
	if (m_MathModulePtr)
	{
		m_MathModulePtr->UpdateShaders(vFiles);
	}
}