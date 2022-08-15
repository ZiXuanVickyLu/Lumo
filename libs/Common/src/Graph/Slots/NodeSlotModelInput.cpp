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

#include "NodeSlotModelInput.h"
#include <Graph/Base/BaseNode.h>
#include <Interfaces/ModelInputInterface.h>
#include <Interfaces/ModelOutputInterface.h>

#include <utility>
static const float slotIconSize = 15.0f;

//////////////////////////////////////////////////////////////////////////////////////////////
//// STATIC //////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

NodeSlotModelInputPtr NodeSlotModelInput::Create(NodeSlotModelInput vSlot)
{
	auto res = std::make_shared<NodeSlotModelInput>(vSlot);
	res->m_This = res;
	return res;
}

NodeSlotModelInputPtr NodeSlotModelInput::Create(const std::string& vName)
{
	auto res = std::make_shared<NodeSlotModelInput>(vName);
	res->m_This = res;
	return res;
}

NodeSlotModelInputPtr NodeSlotModelInput::Create(const std::string& vName, const bool& vHideName)
{
	auto res = std::make_shared<NodeSlotModelInput>(vName, vHideName);
	res->m_This = res;
	return res;
}

NodeSlotModelInputPtr NodeSlotModelInput::Create(const std::string& vName, const bool& vHideName, const bool& vShowWidget)
{
	auto res = std::make_shared<NodeSlotModelInput>(vName, vHideName, vShowWidget);
	res->m_This = res;
	return res;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//// NODESLOT CLASS //////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

NodeSlotModelInput::NodeSlotModelInput()
	: NodeSlotInput("", "MESH")
{
	pinID = sGetNewSlotId();
	color = sGetSlotColors()->GetSlotColor(slotType);
	colorIsSet = true;
}

NodeSlotModelInput::NodeSlotModelInput(const std::string& vName)
	: NodeSlotInput(vName, "MESH")
{
	pinID = sGetNewSlotId();
	color = sGetSlotColors()->GetSlotColor(slotType);
	colorIsSet = true;
}

NodeSlotModelInput::NodeSlotModelInput(const std::string& vName, const bool& vHideName)
	: NodeSlotInput(vName, "MESH", vHideName)
{
	pinID = sGetNewSlotId();
	color = sGetSlotColors()->GetSlotColor(slotType);
	colorIsSet = true;
}

NodeSlotModelInput::NodeSlotModelInput(const std::string& vName, const bool& vHideName, const bool& vShowWidget)
	: NodeSlotInput(vName, "MESH", vHideName, vShowWidget)
{
	pinID = sGetNewSlotId();
	color = sGetSlotColors()->GetSlotColor(slotType);
	colorIsSet = true;
}

NodeSlotModelInput::~NodeSlotModelInput() = default;

void NodeSlotModelInput::Init()
{
	
}

void NodeSlotModelInput::Unit()
{
	// ici pas besoin du assert sur le m_This 
	// car NodeSlotModelInput peut etre isntancié à l'ancienne en copie local donc sans shared_ptr
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

void NodeSlotModelInput::Connect(NodeSlotWeak vOtherSlot)
{
	if (slotType == "MESH")
	{
		auto endSlotPtr = vOtherSlot.getValidShared();
		if (endSlotPtr)
		{
			auto parentNodePtr = dynamic_pointer_cast<ModelInputInterface>(parentNode.getValidShared());
			if (parentNodePtr)
			{
				auto otherModelNodePtr = dynamic_pointer_cast<ModelOutputInterface>(endSlotPtr->parentNode.getValidShared());
				if (otherModelNodePtr)
				{
					parentNodePtr->SetModel(otherModelNodePtr->GetModel());
				}
			}
		}
	}
}

void NodeSlotModelInput::DisConnect(NodeSlotWeak vOtherSlot)
{
	if (slotType == "MESH")
	{
		auto endSlotPtr = vOtherSlot.getValidShared();
		if (endSlotPtr)
		{
			auto parentNodePtr = dynamic_pointer_cast<ModelInputInterface>(parentNode.getValidShared());
			if (parentNodePtr)
			{
				parentNodePtr->SetModel(SceneModelWeak());
			}
		}
	}
}

void NodeSlotModelInput::TreatNotification(
	const NotifyEvent& vEvent,
	const NodeSlotWeak& vEmitterSlot,
	const NodeSlotWeak& vReceiverSlot)
{
	if (vEvent == ModelUpdateDone)
	{
		auto emiterSlotPtr = vEmitterSlot.getValidShared();
		if (emiterSlotPtr)
		{
			if (emiterSlotPtr->IsAnOutput())
			{
				auto parentModelInputNodePtr = dynamic_pointer_cast<ModelInputInterface>(parentNode.getValidShared());
				if (parentModelInputNodePtr)
				{
					auto otherNodePtr = dynamic_pointer_cast<ModelOutputInterface>(emiterSlotPtr->parentNode.getValidShared());
					if (otherNodePtr)
					{
						parentModelInputNodePtr->SetModel(otherNodePtr->GetModel());
					}
				}
			}
		}
	}
}

void NodeSlotModelInput::DrawDebugInfos()
{
	ImGui::Text("--------------------");
	ImGui::Text("Slot %s", name.c_str());
	ImGui::Text(IsAnInput() ? "Input" : "Output");
	ImGui::Text("Count connections : %u", (uint32_t)linkedSlots.size());
}
