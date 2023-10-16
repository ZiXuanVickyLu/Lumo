/*
Copyright 2022 - 2022 Stephane Cuillerdier(aka aiekick)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http:://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissionsand
limitations under the License.
*/

#include <LumoBackend/Graph/Graph.h>
#include <LumoBackend/Graph/Base/BaseNode.h>
#include <Interfaces/SceneAudiArtInputInterface.h>
#include <Interfaces/SceneAudiArtOutputInterface.h>

class BlackmanFilterModule;
class BlackmanFilterNode :
	public SceneAudiArtInputInterface,
	public SceneAudiArtOutputInterface,
	public BaseNode
{
public:
	static std::shared_ptr<BlackmanFilterNode> Create(GaiApi::VulkanCorePtr vVulkanCorePtr);

private:
	std::shared_ptr<BlackmanFilterModule> m_BlackmanFilterModulePtr = nullptr;

public:
	BlackmanFilterNode();
	~BlackmanFilterNode() override;

	// Init / Unit
	bool Init(GaiApi::VulkanCorePtr vVulkanCorePtr) override;

	// Execute Task
	bool ExecuteAllTime(const uint32_t & vCurrentFrame, vk::CommandBuffer * vCmd = nullptr, BaseNodeState * vBaseNodeState = nullptr) override;

	// Draw Widgets
    bool DrawWidgets(const uint32_t& vCurrentFrame, ImGuiContext* vContextPtr = nullptr, const std::string& vUserDatas = {}) override;
    bool DrawOverlays(const uint32_t& vCurrentFrame, const ImRect& vRect, ImGuiContext* vContextPtr = nullptr, const std::string& vUserDatas = {}) override;
    bool DrawDialogsAndPopups(const uint32_t& vCurrentFrame, const ImVec2& vMaxSize, ImGuiContext* vContextPtr = nullptr, const std::string& vUserDatas = {}) override;
	void DisplayInfosOnTopOfTheNode(BaseNodeState* vBaseNodeState) override;

	// Interfaces Setters
	void SetSceneAudiArt(const std::string& vName, SceneAudiArtWeak vSceneAudiArt) override;


	// Interfaces Getters
	SceneAudiArtWeak GetSceneAudiArt(const std::string& vName) override;


	// Configuration
	std::string getXml(const std::string& vOffset, const std::string& vUserDatas = "") override;
	bool setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& vUserDatas) override;
	void AfterNodeXmlLoading() override;

};
