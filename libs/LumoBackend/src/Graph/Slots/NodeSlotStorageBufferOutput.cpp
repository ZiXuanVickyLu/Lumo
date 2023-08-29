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

#include <LumoBackend/Graph/Slots/NodeSlotStorageBufferOutput.h>

#include <utility>
#include <LumoBackend/Graph/Base/BaseNode.h>

static const float slotIconSize = 15.0f;

//////////////////////////////////////////////////////////////////////////////////////////////
//// STATIC //////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

NodeSlotStorageBufferOutputPtr NodeSlotStorageBufferOutput::Create(NodeSlotStorageBufferOutput vSlot)
{
	auto res = std::make_shared<NodeSlotStorageBufferOutput>(vSlot);
	res->m_This = res;
	return res;
}

NodeSlotStorageBufferOutputPtr NodeSlotStorageBufferOutput::Create(const std::string& vName, const std::string& vType)
{
	auto res = std::make_shared<NodeSlotStorageBufferOutput>(vName, vType);
	res->m_This = res;
	return res;
}

NodeSlotStorageBufferOutputPtr NodeSlotStorageBufferOutput::Create(const std::string& vName, const std::string& vType, const uint32_t& vBindingPoint)
{
	auto res = std::make_shared<NodeSlotStorageBufferOutput>(vName, vType, vBindingPoint);
	res->m_This = res;
	return res;
}

NodeSlotStorageBufferOutputPtr NodeSlotStorageBufferOutput::Create(const std::string& vName, const std::string& vType, const uint32_t& vBindingPoint, const bool& vHideName)
{
	auto res = std::make_shared<NodeSlotStorageBufferOutput>(vName, vType, vBindingPoint, vHideName);
	res->m_This = res;
	return res;
}

NodeSlotStorageBufferOutputPtr NodeSlotStorageBufferOutput::Create(const std::string& vName, const std::string& vType, const uint32_t& vBindingPoint, const bool& vHideName, const bool& vShowWidget)
{
	auto res = std::make_shared<NodeSlotStorageBufferOutput>(vName, vType, vBindingPoint, vHideName, vShowWidget);
	res->m_This = res;
	return res;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//// NODESLOT CLASS //////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

NodeSlotStorageBufferOutput::NodeSlotStorageBufferOutput()
	: NodeSlotOutput("", "")
{
	pinID = sGetNewSlotId();
	color = sGetSlotColors()->GetSlotColor(slotType);
	colorIsSet = true;
}

NodeSlotStorageBufferOutput::NodeSlotStorageBufferOutput(const std::string& vName, const std::string& vType)
	: NodeSlotOutput(vName, vType)
{
	pinID = sGetNewSlotId();
	color = sGetSlotColors()->GetSlotColor(slotType);
	colorIsSet = true;
}

NodeSlotStorageBufferOutput::NodeSlotStorageBufferOutput(const std::string& vName, const std::string& vType, const uint32_t& vBindingPoint)
	: NodeSlotOutput(vName, vType)
{
	pinID = sGetNewSlotId();
	color = sGetSlotColors()->GetSlotColor(slotType);
	colorIsSet = true;
	descriptorBinding = vBindingPoint;
}

NodeSlotStorageBufferOutput::NodeSlotStorageBufferOutput(const std::string& vName, const std::string& vType, const uint32_t& vBindingPoint, const bool& vHideName)
	: NodeSlotOutput(vName, vType, vHideName)
{
	pinID = sGetNewSlotId();
	color = sGetSlotColors()->GetSlotColor(slotType);
	colorIsSet = true;
	descriptorBinding = vBindingPoint;
}

NodeSlotStorageBufferOutput::NodeSlotStorageBufferOutput(const std::string& vName, const std::string& vType, const uint32_t& vBindingPoint, const bool& vHideName, const bool& vShowWidget)
	: NodeSlotOutput(vName, vType, vHideName, vShowWidget)
{
	pinID = sGetNewSlotId();
	color = sGetSlotColors()->GetSlotColor(slotType);
	colorIsSet = true;
	descriptorBinding = vBindingPoint;
}

NodeSlotStorageBufferOutput::~NodeSlotStorageBufferOutput() = default;

void NodeSlotStorageBufferOutput::Init()
{
	
}

void NodeSlotStorageBufferOutput::Unit()
{
	// ici pas besoin du assert sur le m_This 
	// car NodeSlotStorageBufferOutput peut etre instancié à l'ancienne en copie local donc sans shared_ptr
	// donc pour gagner du temps on va checker le this, si expiré on va pas plus loins
	if (!m_This.expired())
	{
		if (!parentNode.expired())
		{
			auto parentNodePtr = parentNode.lock();
			if (parentNodePtr)
			{
				auto graph = parentNodePtr->GetParentNode();
				if (!graph.expired())
				{
					auto graphPtr = graph.lock();
					if (graphPtr)
					{
						graphPtr->BreakAllLinksConnectedToSlot(m_This);
					}
				}
			}
		}
	}
}

void NodeSlotStorageBufferOutput::SendFrontNotification(const NotifyEvent& vEvent)
{
	if (vEvent == StorageBufferUpdateDone)
	{
		SendNotification(slotType, vEvent);
	}
}

void NodeSlotStorageBufferOutput::DrawDebugInfos()
{
	ImGui::Text("--------------------");
	ImGui::Text("Slot %s", name.c_str());
	ImGui::Text(IsAnInput() ? "Input" : "Output");
	ImGui::Text("Count connections : %u", (uint32_t)linkedSlots.size());
}