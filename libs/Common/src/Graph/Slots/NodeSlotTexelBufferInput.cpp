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

// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "NodeSlotTexelBufferInput.h"
#include <Graph/Base/BaseNode.h>
#include <Interfaces/TexelBufferInputInterface.h>
#include <Interfaces/TexelBufferOutputInterface.h>

#include <utility>
static const float slotIconSize = 15.0f;

//////////////////////////////////////////////////////////////////////////////////////////////
//// STATIC //////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

NodeSlotTexelBufferInputPtr NodeSlotTexelBufferInput::Create(NodeSlotTexelBufferInput vSlot)
{
	auto res = std::make_shared<NodeSlotTexelBufferInput>(vSlot);
	res->m_This = res;
	return res;
}

NodeSlotTexelBufferInputPtr NodeSlotTexelBufferInput::Create(const std::string& vName, const std::string& vType)
{
	auto res = std::make_shared<NodeSlotTexelBufferInput>(vName, vType);
	res->m_This = res;
	return res;
}

NodeSlotTexelBufferInputPtr NodeSlotTexelBufferInput::Create(const std::string& vName, const std::string& vType, const uint32_t& vBindingPoint)
{
	auto res = std::make_shared<NodeSlotTexelBufferInput>(vName, vType, vBindingPoint);
	res->m_This = res;
	return res;
}

NodeSlotTexelBufferInputPtr NodeSlotTexelBufferInput::Create(const std::string& vName, const std::string& vType, const uint32_t& vBindingPoint, const bool& vHideName)
{
	auto res = std::make_shared<NodeSlotTexelBufferInput>(vName, vType, vBindingPoint, vHideName);
	res->m_This = res;
	return res;
}

NodeSlotTexelBufferInputPtr NodeSlotTexelBufferInput::Create(const std::string& vName, const std::string& vType, const uint32_t& vBindingPoint, const bool& vHideName, const bool& vShowWidget)
{
	auto res = std::make_shared<NodeSlotTexelBufferInput>(vName, vType, vBindingPoint, vHideName, vShowWidget);
	res->m_This = res;
	return res;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//// NODESLOT CLASS //////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

NodeSlotTexelBufferInput::NodeSlotTexelBufferInput()
	: NodeSlotInput("", "")
{
	pinID = sGetNewSlotId();
	color = sGetSlotColors()->GetSlotColor(slotType);
	colorIsSet = true;
}

NodeSlotTexelBufferInput::NodeSlotTexelBufferInput(const std::string& vName, const std::string& vType)
	: NodeSlotInput(vName, vType)
{
	pinID = sGetNewSlotId();
	color = sGetSlotColors()->GetSlotColor(slotType);
	colorIsSet = true;
}

NodeSlotTexelBufferInput::NodeSlotTexelBufferInput(const std::string& vName, const std::string& vType, const uint32_t& vBindingPoint)
	: NodeSlotInput(vName, vType)
{
	pinID = sGetNewSlotId();
	color = sGetSlotColors()->GetSlotColor(slotType);
	colorIsSet = true; 
	descriptorBinding = vBindingPoint;
}

NodeSlotTexelBufferInput::NodeSlotTexelBufferInput(const std::string& vName, const std::string& vType, const uint32_t& vBindingPoint, const bool& vHideName)
	: NodeSlotInput(vName, vType, vHideName)
{
	pinID = sGetNewSlotId();
	color = sGetSlotColors()->GetSlotColor(slotType);
	colorIsSet = true;
	descriptorBinding = vBindingPoint;
}

NodeSlotTexelBufferInput::NodeSlotTexelBufferInput(const std::string& vName, const std::string& vType, const uint32_t& vBindingPoint, const bool& vHideName, const bool& vShowWidget)
	: NodeSlotInput(vName, vType, vHideName, vShowWidget)
{
	pinID = sGetNewSlotId();
	color = sGetSlotColors()->GetSlotColor(slotType);
	colorIsSet = true;
	descriptorBinding = vBindingPoint;
}

NodeSlotTexelBufferInput::~NodeSlotTexelBufferInput() = default;

void NodeSlotTexelBufferInput::Init()
{
	
}

void NodeSlotTexelBufferInput::Unit()
{
	// ici pas besoin du assert sur le m_This 
	// car NodeSlotTexelBufferInput peut etre isntancié à l'ancienne en copie local donc sans shared_ptr
	// donc pour gagner du temps on va checker le this, si expiré on va pas plus loins
	if (!m_This.expired())
	{
		if (!parentNode.expired())
		{
			auto parentNodePtr = parentNode.lock();
			if (parentNodePtr)
			{
				auto graph = parentNodePtr->m_ParentNode;
				if (!graph.expired())
				{
					auto graphPtr = graph.lock();
					if (graphPtr)
					{
						graphPtr->DisConnectSlot(m_This);
					}
				}
			}
		}
	}
}

void NodeSlotTexelBufferInput::Connect(NodeSlotWeak vOtherSlot)
{
	auto endSlotPtr = vOtherSlot.getValidShared();
	if (endSlotPtr)
	{
		if (endSlotPtr->slotType == slotType)
		{
			auto parentNodePtr = dynamic_pointer_cast<TexelBufferInputInterface<0u>>(parentNode.getValidShared());
			if (parentNodePtr)
			{
				auto otherTexelBufferNodePtr = dynamic_pointer_cast<TexelBufferOutputInterface>(endSlotPtr->parentNode.getValidShared());
				if (otherTexelBufferNodePtr)
				{
					ct::uvec2 texelBufferSize;
					auto bufferPtr = otherTexelBufferNodePtr->GetTexelBuffer(endSlotPtr->descriptorBinding, &texelBufferSize);
					parentNodePtr->SetTexelBuffer(descriptorBinding, bufferPtr, &texelBufferSize);
					auto bufferViewPtr = otherTexelBufferNodePtr->GetTexelBufferView(endSlotPtr->descriptorBinding, &texelBufferSize);
					parentNodePtr->SetTexelBufferView(descriptorBinding, bufferViewPtr, &texelBufferSize);
				}
			}
		}
	}
}

void NodeSlotTexelBufferInput::DisConnect(NodeSlotWeak vOtherSlot)
{
	auto endSlotPtr = vOtherSlot.getValidShared();
	if (endSlotPtr)
	{
		if (endSlotPtr->slotType == slotType)
		{
			auto parentNodePtr = dynamic_pointer_cast<TexelBufferInputInterface<0u>>(parentNode.getValidShared());
			if (parentNodePtr)
			{
				parentNodePtr->SetTexelBuffer(descriptorBinding, nullptr, nullptr);
				parentNodePtr->SetTexelBufferView(descriptorBinding, nullptr, nullptr);
			}
		}
	}
}

void NodeSlotTexelBufferInput::TreatNotification(
	const NotifyEvent& vEvent,
	const NodeSlotWeak& vEmitterSlot,
	const NodeSlotWeak& vReceiverSlot)
{
	if (vEvent == NotifyEvent::TexelBufferUpdateDone)
	{
		auto emiterSlotPtr = vEmitterSlot.getValidShared();
		if (emiterSlotPtr)
		{
			if (emiterSlotPtr->IsAnOutput())
			{
				auto parentNodePtr = parentNode.getValidShared();
				if (parentNodePtr)
				{
					auto parentTexelBufferInputNodePtr = dynamic_pointer_cast<TexelBufferInputInterface<0u>>(parentNodePtr);
					if (parentTexelBufferInputNodePtr)
					{
						auto otherNodePtr = dynamic_pointer_cast<TexelBufferOutputInterface>(emiterSlotPtr->parentNode.getValidShared());
						if (otherNodePtr)
						{
							auto receiverSlotPtr = vReceiverSlot.getValidShared();
							if (receiverSlotPtr)
							{
								ct::uvec2 texelBufferSize;
								auto bufferPtr = otherNodePtr->GetTexelBuffer(emiterSlotPtr->descriptorBinding, &texelBufferSize);
								parentTexelBufferInputNodePtr->SetTexelBuffer(receiverSlotPtr->descriptorBinding, bufferPtr, &texelBufferSize);
								auto bufferViewPtr = otherNodePtr->GetTexelBufferView(emiterSlotPtr->descriptorBinding, &texelBufferSize);
								parentTexelBufferInputNodePtr->SetTexelBufferView(receiverSlotPtr->descriptorBinding, bufferViewPtr, &texelBufferSize);
							}
						}
					}
				}
			}
		}
	}
}

void NodeSlotTexelBufferInput::DrawDebugInfos()
{
	ImGui::Text("--------------------");
	ImGui::Text("Slot %s", name.c_str());
	ImGui::Text(IsAnInput() ? "Input" : "Output");
	ImGui::Text("Count connections : %u", (uint32_t)linkedSlots.size());
}

//////////////////////////////////////////////////////////////////////////////////////////////
//// CONFIGURATION ///////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

std::string NodeSlotTexelBufferInput::getXml(const std::string& vOffset, const std::string& /*vUserDatas*/)
{
	std::string res;

	res += vOffset + ct::toStr("<slot index=\"%u\" name=\"%s\" type=\"%s\" place=\"%s\" id=\"%u\"/>\n",
		index,
		name.c_str(),
		slotType.c_str(),
		NodeSlot::sGetStringFromNodeSlotPlaceEnum(slotPlace).c_str(),
		(uint32_t)pinID.Get());

	return res;
}

bool NodeSlotTexelBufferInput::setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& /*vUserDatas*/)
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

	if (strName == "slot" && strParentName == "node")
	{
		uint32_t _index = 0U;
		std::string _name;
		std::string _type = "NONE";
		auto _place = NodeSlot::PlaceEnum::NONE;
		uint32_t _pinId = 0U;

		for (const tinyxml2::XMLAttribute* attr = vElem->FirstAttribute(); attr != nullptr; attr = attr->Next())
		{
			std::string attName = attr->Name();
			std::string attValue = attr->Value();

			if (attName == "index")
				_index = ct::ivariant(attValue).GetU();
			else if (attName == "name")
				_name = attValue;
			else if (attName == "type")
				_type = attValue;
			else if (attName == "place")
				_place = NodeSlot::sGetNodeSlotPlaceEnumFromString(attValue);
			else if (attName == "id")
				_pinId = ct::ivariant(attValue).GetU();
		}

		if (index == _index &&
			slotType == _type && 
			slotPlace == _place && 
			!idAlreadySetbyXml)
		{
			pinID = _pinId;
			idAlreadySetbyXml = true;

			// pour eviter que des slots aient le meme id qu'un nodePtr
			BaseNode::freeNodeId = ct::maxi<uint32_t>(BaseNode::freeNodeId, (uint32_t)pinID.Get());

			return false;
		}
	}	

	return true;
}