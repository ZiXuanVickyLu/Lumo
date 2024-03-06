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

#pragma once
#pragma warning(disable : 4251)

#include <LumoBackend/Graph/Graph.h>
#include <LumoBackend/Graph/Base/NodeSlotOutput.h>

class NodeSlotTaskOutput;
typedef std::weak_ptr<NodeSlotTaskOutput> NodeSlotTaskOutputWeak;
typedef std::shared_ptr<NodeSlotTaskOutput> NodeSlotTaskOutputPtr;

class LUMO_BACKEND_API NodeSlotTaskOutput : public NodeSlotOutput {
public:
    static NodeSlotTaskOutputPtr Create(NodeSlotTaskOutput vSlot);
    static NodeSlotTaskOutputPtr Create(const std::string& vName);
    static NodeSlotTaskOutputPtr Create(const std::string& vName, const bool& vHideName);
    static NodeSlotTaskOutputPtr Create(const std::string& vName, const bool& vHideName, const bool& vShowWidget);

public:
    explicit NodeSlotTaskOutput();
    explicit NodeSlotTaskOutput(const std::string& vName);
    explicit NodeSlotTaskOutput(const std::string& vName, const bool& vHideName);
    explicit NodeSlotTaskOutput(const std::string& vName, const bool& vHideName, const bool& vShowWidget);
    virtual ~NodeSlotTaskOutput();

    void Init();
    void Unit();

    void SendFrontNotification(const NotifyEvent& vEvent) override;

    void DrawDebugInfos() override;
};