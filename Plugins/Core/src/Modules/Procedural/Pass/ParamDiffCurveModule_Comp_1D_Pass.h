/*
Copyright 2022 - 2022 Stephane Cuillerdier(aka aiekick)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http ://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissionsand
limitations under the License.
*/

#pragma once

#include <set>
#include <array>
#include <string>
#include <memory>

#include <Headers/Globals.h>

#include <ctools/cTools.h>
#include <ctools/ConfigAbstract.h>

#include <Base/BaseRenderer.h>
#include <Base/ShaderPass.h>
#include <vulkan/vulkan.hpp>
#include <vkFramework/Texture2D.h>
#include <vkFramework/VulkanCore.h>
#include <vkFramework/VulkanDevice.h>
#include <vkFramework/vk_mem_alloc.h>
#include <vkFramework/VulkanShader.h>
#include <vkFramework/ImGuiTexture.h>
#include <vkFramework/VulkanRessource.h>
#include <vkFramework/VulkanFrameBuffer.h>

#include <Interfaces/GuiInterface.h>
#include <Interfaces/NodeInterface.h>
#include <Interfaces/ModelOutputInterface.h>

#include <SceneGraph/SceneModel.h>
#include <SceneGraph/SceneMesh.h>

class ParamDiffCurveModule_Comp_1D_Pass :
	public ShaderPass,
	public ModelOutputInterface,
	public GuiInterface,
	public NodeInterface
{
private:
	struct UBO_0_Comp {
		alignas(16) ct::fvec3 u_start_pos = ct::fvec3(0.001f, 0.001f, 0.001f);
		alignas(4) uint32_t u_step_count = 5000U;
		alignas(4) float u_step_size = 0.01f;
	} m_UBO_0_Comp;
	VulkanBufferObjectPtr m_UBO_0_Comp_Ptr = nullptr;
	vk::DescriptorBufferInfo m_UBO_0_Comp_BufferInfos;

	SceneModelPtr m_SceneModelPtr = nullptr;
	SceneMeshPtr m_SceneMeshPtr = nullptr;

public:
	ParamDiffCurveModule_Comp_1D_Pass(vkApi::VulkanCorePtr vVulkanCorePtr);
	~ParamDiffCurveModule_Comp_1D_Pass() override;

	void ActionBeforeInit() override;
	void WasJustResized() override;

	void Compute(vk::CommandBuffer* vCmdBuffer, const int& vIterationNumber) override;
	bool DrawWidgets(const uint32_t& vCurrentFrame, ImGuiContext* vContext = nullptr) override;
	void DrawOverlays(const uint32_t& vCurrentFrame, const ct::frect& vRect, ImGuiContext* vContext = nullptr) override;
	void DisplayDialogsAndPopups(const uint32_t& vCurrentFrame, const ct::ivec2& vMaxSize, ImGuiContext* vContext = nullptr) override;

	// Interfaces Getters
	SceneModelWeak GetModel() override;


	std::string getXml(const std::string& vOffset, const std::string& vUserDatas) override;
	bool setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& vUserDatas) override;
	void AfterNodeXmlLoading() override;

protected:
	bool CreateUBO() override;
	void UploadUBO() override;
	void DestroyUBO() override;

	bool UpdateLayoutBindingInRessourceDescriptor() override;
	bool UpdateBufferInfoInRessourceDescriptor() override;

	std::string GetComputeShaderCode(std::string& vOutShaderName) override;
};