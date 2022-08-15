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
#include <Graph/Base/NodeSlotInput.h>

class NodeSlotLightGroupInput;
typedef ct::cWeak<NodeSlotLightGroupInput> NodeSlotLightGroupInputWeak;
typedef std::shared_ptr<NodeSlotLightGroupInput> NodeSlotLightGroupInputPtr;

class NodeSlotLightGroupInput : 
	public NodeSlotInput
{
public:
	static NodeSlotLightGroupInputPtr Create(NodeSlotLightGroupInput vSlot);
	static NodeSlotLightGroupInputPtr Create(const std::string& vName);
	static NodeSlotLightGroupInputPtr Create(const std::string& vName, const bool& vHideName);
	static NodeSlotLightGroupInputPtr Create(const std::string& vName, const bool& vHideName, const bool& vShowWidget);

public:
	explicit NodeSlotLightGroupInput();
	explicit NodeSlotLightGroupInput(const std::string& vName);
	explicit NodeSlotLightGroupInput(const std::string& vName, const bool& vHideName);
	explicit NodeSlotLightGroupInput(const std::string& vName, const bool& vHideName, const bool& vShowWidget);
	~NodeSlotLightGroupInput();

	void Init();
	void Unit();

	void Connect(NodeSlotWeak vOtherSlot) override;
	void DisConnect(NodeSlotWeak vOtherSlot) override;

	void TreatNotification(
		const NotifyEvent& vEvent,
		const NodeSlotWeak& vEmitterSlot = NodeSlotWeak(),
		const NodeSlotWeak& vReceiverSlot = NodeSlotWeak());

	void DrawDebugInfos();
};