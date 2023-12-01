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

#include <array>
#include <memory>
#include <ctools/ConfigAbstract.h>
#include <LumoBackend/Base/ShaderPass.h>
#include <Gaia/Resources/Texture2D.h>
#include <Gaia/Resources/VulkanRessource.h>
#include <Gaia/Core/VulkanDevice.h>
#include <LumoBackend/Interfaces/ModelInputInterface.h>
#include <LumoBackend/Interfaces/NodeInterface.h>
#include <LumoBackend/Interfaces/GuiInterface.h>
#include <LumoBackend/Interfaces/CameraInterface.h>
#include <LumoBackend/Interfaces/TextureOutputInterface.h>
#include <LumoBackend/Interfaces/MergedInterface.h>


class HeatmapRenderer_Mesh_Pass :
	public ShaderPass,
	
	public ModelInputInterface,
	public TextureOutputInterface
{
private:
	VulkanBufferObjectPtr m_UBOVertPtr = nullptr;
	vk::DescriptorBufferInfo m_DescriptorBufferInfo_Vert;

	struct UBOVert {
		alignas(16) glm::mat4x4 transform = glm::mat4x4(1.0f);
	} m_UBOVert;

	VulkanBufferObjectPtr m_UBOFragPtr = nullptr;
	vk::DescriptorBufferInfo m_DescriptorBufferInfo_Frag;

	struct UBOFrag {
		alignas(4) uint32_t channel_idx = 0U;
		alignas(4) uint32_t count_colors = 0U;
	} m_UBOFrag;

	std::vector<ct::fvec4> m_Colors;
	std::vector<ct::fvec4> m_DefaultColors;
	VulkanBufferObjectPtr m_SBO_Colors = nullptr;
	vk::DescriptorBufferInfo m_SBO_ColorsDescriptorBufferInfo = vk::DescriptorBufferInfo { VK_NULL_HANDLE, 0, VK_WHOLE_SIZE };
	//VulkanBufferObjectPtr m_SBO_Empty_Colors = nullptr;

public:
	HeatmapRenderer_Mesh_Pass(GaiApi::VulkanCoreWeak vVulkanCore);
	~HeatmapRenderer_Mesh_Pass() override;

	void ActionBeforeInit() override;
	void DrawModel(vk::CommandBuffer* vCmdBufferPtr, const int& vIterationNumber) override;
	bool DrawWidgets(const uint32_t& vCurrentFrame, ImGuiContext* vContextPtr = nullptr, const std::string& vUserDatas = {}) override;
	bool DrawOverlays(const uint32_t& vCurrentFrame, const ImRect& vRect, ImGuiContext* vContextPtr = nullptr, const std::string& vUserDatas = {}) override;
	bool DrawDialogsAndPopups(const uint32_t& vCurrentFrame, const ImVec2& vMaxSize, ImGuiContext* vContextPtr = nullptr, const std::string& vUserDatas = {}) override;
	void SetModel(SceneModelWeak vSceneModel = SceneModelWeak()) override;
	vk::DescriptorImageInfo* GetDescriptorImageInfo(const uint32_t& vBindingPoint, ct::fvec2* vOutSize = nullptr) override;

	std::string getXml(const std::string& vOffset, const std::string& vUserDatas = "") override;
	bool setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& vUserDatas = "") override;

private:
	void ClearColorBuffer();
	void AddColorToBuffer(const ct::fvec4& vColor);

	void DestroyModel(const bool& vReleaseDatas = false) override;

	bool CreateSBO() override;
	void UploadSBO() override;
	void DestroySBO() override;

	bool CreateUBO() override;
	void UploadUBO() override;
	void DestroyUBO() override;

	bool UpdateLayoutBindingInRessourceDescriptor() override;
	bool UpdateBufferInfoInRessourceDescriptor() override;

	void SetInputStateBeforePipelineCreation() override;

	std::string GetVertexShaderCode(std::string& vOutShaderName) override;
	std::string GetFragmentShaderCode(std::string& vOutShaderName) override;
};