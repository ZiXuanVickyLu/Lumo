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

#pragma once

#include <Graph/Graph.h>
#include <ctools/cTools.h>
#include <Graph/Base/BaseNode.h>
#include <Interfaces/TextureOutputInterface.h>

class WidgetColorModule;
class WidgetColorNode : 
	public BaseNode, 
	public TextureOutputInterface
{
public:
	static std::shared_ptr<WidgetColorNode> Create(vkApi::VulkanCorePtr vVulkanCorePtr);

private:
	std::shared_ptr<WidgetColorModule> m_WidgetColorModule = nullptr;

public:
	WidgetColorNode();
	~WidgetColorNode() override;
	bool Init(vkApi::VulkanCorePtr vVulkanCorePtr) override;
	void Unit() override;
	bool ExecuteAllTime(const uint32_t& vCurrentFrame, vk::CommandBuffer* vCmd = nullptr, BaseNodeState* vBaseNodeState = nullptr) override;
	bool DrawWidgets(const uint32_t& vCurrentFrame, ImGuiContext* vContext = nullptr) override;
	void Notify(const NotifyEvent& vEvent, const NodeSlotWeak& vEmmiterSlot = NodeSlotWeak(), const NodeSlotWeak& vReceiverSlot = NodeSlotWeak()) override;
	void DrawOutputWidget(BaseNodeState* vBaseNodeState, NodeSlotWeak vSlot) override;
	void DisplayDialogsAndPopups(const uint32_t& vCurrentFrame, const ct::ivec2& vMaxSize, ImGuiContext* vContext = nullptr) override;
	void DisplayInfosOnTopOfTheNode(BaseNodeState* vBaseNodeState) override;
	void JustConnectedBySlots(NodeSlotWeak vStartSlot, NodeSlotWeak vEndSlot) override;
	void JustDisConnectedBySlots(NodeSlotWeak vStartSlot, NodeSlotWeak vEndSlot) override;
	vk::DescriptorImageInfo* GetDescriptorImageInfo(const uint32_t& vBindingPoint, ct::fvec2* vOutSize = nullptr) override;
	std::string getXml(const std::string& vOffset, const std::string& vUserDatas = "") override;
	bool setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& vUserDatas) override;
};