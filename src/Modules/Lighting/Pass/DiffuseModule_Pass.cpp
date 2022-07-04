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

#include "DiffuseModule_Pass.h"

#include <functional>
#include <Gui/MainFrame.h>
#include <ctools/Logger.h>
#include <ctools/FileHelper.h>
#include <ImWidgets/ImWidgets.h>
#include <Systems/CommonSystem.h>
#include <Profiler/vkProfiler.hpp>
#include <vkFramework/VulkanCore.h>
#include <vkFramework/VulkanShader.h>
#include <vkFramework/VulkanSubmitter.h>
#include <utils/Mesh/VertexStruct.h>
#include <cinttypes>
#include <Base/FrameBuffer.h>
#include <Modules/Lighting/LightGroupModule.h>

using namespace vkApi;

#define COUNT_BUFFERS 2

//////////////////////////////////////////////////////////////
//// SSAO SECOND PASS : BLUR /////////////////////////////////
//////////////////////////////////////////////////////////////

DiffuseModule_Pass::DiffuseModule_Pass(vkApi::VulkanCorePtr vVulkanCorePtr)
	: ShaderPass(vVulkanCorePtr)
{
	SetRenderDocDebugName("Comp Pass : Diffuse", COMPUTE_SHADER_PASS_DEBUG_COLOR);
}

DiffuseModule_Pass::~DiffuseModule_Pass()
{
	Unit();
}

bool DiffuseModule_Pass::DrawWidgets(const uint32_t& vCurrentFrame, ImGuiContext* vContext)
{
	return false;
}

void DiffuseModule_Pass::DrawOverlays(const uint32_t& vCurrentFrame, const ct::frect& vRect, ImGuiContext* vContext)
{

}

void DiffuseModule_Pass::DisplayDialogsAndPopups(const uint32_t& vCurrentFrame, const ct::ivec2& vMaxSize, ImGuiContext* vContext)
{

}

void DiffuseModule_Pass::SetTexture(const uint32_t& vBinding, vk::DescriptorImageInfo* vImageInfo)
{
	ZoneScoped;

	if (m_Loaded)
	{
		if (vBinding < m_ImageInfos.size())
		{
			if (vImageInfo)
			{
				m_ImageInfos[vBinding] = *vImageInfo;
			}
			else
			{
				if (m_EmptyTexturePtr)
				{
					m_ImageInfos[vBinding] = m_EmptyTexturePtr->m_DescriptorImageInfo;
				}
				else
				{
					CTOOL_DEBUG_BREAK;
				}
			}

			m_NeedSamplerUpdate = true;
		}
	}
}

vk::DescriptorImageInfo* DiffuseModule_Pass::GetDescriptorImageInfo(const uint32_t& vBindingPoint)
{
	if (m_ComputeBufferPtr)
	{
		return m_ComputeBufferPtr->GetFrontDescriptorImageInfo(vBindingPoint);
	}

	return nullptr;
}

void DiffuseModule_Pass::SetLightGroup(SceneLightGroupWeak vSceneLightGroup)
{
	m_SceneLightGroup = vSceneLightGroup;
}

void DiffuseModule_Pass::SwapOutputDescriptors()
{
	writeDescriptorSets[0].pImageInfo = m_ComputeBufferPtr->GetFrontDescriptorImageInfo(0U); // output
}

void DiffuseModule_Pass::Compute(vk::CommandBuffer* vCmdBuffer, const int& vIterationNumber)
{
	if (vCmdBuffer)
	{
		vCmdBuffer->bindPipeline(vk::PipelineBindPoint::eCompute, m_Pipeline);
		vCmdBuffer->bindDescriptorSets(vk::PipelineBindPoint::eCompute, m_PipelineLayout, 0, m_DescriptorSet, nullptr);
		vCmdBuffer->dispatch(m_DispatchSize.x, m_DispatchSize.y, m_DispatchSize.z);
	}
}

bool DiffuseModule_Pass::CreateUBO()
{
	ZoneScoped;

	m_EmptyTexturePtr = Texture2D::CreateEmptyTexture(m_VulkanCorePtr, ct::uvec2(1, 1), vk::Format::eR8G8B8A8Unorm);
	for (auto& a : m_ImageInfos)
	{
		a = m_EmptyTexturePtr->m_DescriptorImageInfo;
	}

	NeedNewUBOUpload();

	return true;
}

void DiffuseModule_Pass::DestroyUBO()
{
	ZoneScoped;

	m_EmptyTexturePtr.reset();
}

bool DiffuseModule_Pass::CreateSBO()
{
	m_EmptyLightSBOPtr = SceneLightGroup::CreateEmptyBuffer(m_VulkanCorePtr);
	return (m_EmptyLightSBOPtr != nullptr);
}

void DiffuseModule_Pass::DestroySBO()
{
	ZoneScoped;

	m_EmptyLightSBOPtr.reset();
}

bool DiffuseModule_Pass::UpdateLayoutBindingInRessourceDescriptor()
{
	ZoneScoped;

	m_LayoutBindings.clear();
	m_LayoutBindings.emplace_back(0U, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute);
	m_LayoutBindings.emplace_back(1U, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute);
	m_LayoutBindings.emplace_back(2U, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute);
	m_LayoutBindings.emplace_back(3U, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute);

	return true;
}

bool DiffuseModule_Pass::UpdateBufferInfoInRessourceDescriptor()
{
	ZoneScoped;

	writeDescriptorSets.clear();

	assert(m_ComputeBufferPtr);
	writeDescriptorSets.emplace_back(m_DescriptorSet, 0U, 0, 1, vk::DescriptorType::eStorageImage, 
		m_ComputeBufferPtr->GetBackDescriptorImageInfo(0U), nullptr); // output

	auto lightGrouPtr = m_SceneLightGroup.getValidShared();
	if (lightGrouPtr && lightGrouPtr->GetBufferInfo())
	{
		writeDescriptorSets.emplace_back(m_DescriptorSet, 1U, 0, 1, vk::DescriptorType::eStorageBuffer, 
			nullptr, lightGrouPtr->GetBufferInfo());
	}
	else
	{
		// empty buffer
		assert(m_EmptyLightSBOPtr);
		writeDescriptorSets.emplace_back(m_DescriptorSet, 1U, 0, 1, vk::DescriptorType::eStorageBuffer,
			nullptr, &m_EmptyLightSBOPtr->bufferInfo);
	}

	writeDescriptorSets.emplace_back(m_DescriptorSet, 2U, 0, 1, vk::DescriptorType::eCombinedImageSampler, 
		&m_ImageInfos[0], nullptr); // pos

	writeDescriptorSets.emplace_back(m_DescriptorSet, 3U, 0, 1, vk::DescriptorType::eCombinedImageSampler,
		&m_ImageInfos[1], nullptr); // nor

	return true;
}

std::string DiffuseModule_Pass::GetComputeShaderCode(std::string& vOutShaderName)
{
	vOutShaderName = "DiffuseModule_Pass";

	return u8R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1 ) in;

layout(binding = 0, rgba32f) uniform writeonly image2D outColor;
)" 
+
SceneLightGroup::GetBufferObjectStructureHeader(1U)
+ 
u8R"(
layout(binding = 2) uniform sampler2D pos_map_sampler;
layout(binding = 3) uniform sampler2D nor_map_sampler;

void main()
{
	const ivec2 coords = ivec2(gl_GlobalInvocationID.xy);
	
	vec4 res = vec4(0.0);
	
	if (lightDatas.length() > 0)
	{
		// only the light 0 for the moment
		vec3 light0_pos = lightDatas[0].lightView[3].xyz;
		float light0_intensity = lightDatas[0].lightIntensity;

		vec3 pos = texelFetch(pos_map_sampler, coords, 0).xyz;
		vec3 normal = normalize(texelFetch(nor_map_sampler, coords, 0).xyz * 2.0 - 1.0);
		vec3 light_dir = normalize(light0_pos - pos);
		float diff = min(max(dot(normal, light_dir), 0.0) * light0_intensity, 1.0);

		res = vec4(diff);
	}
	
	imageStore(outColor, coords, res); 
}
)";
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// CONFIGURATION /////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::string DiffuseModule_Pass::getXml(const std::string& vOffset, const std::string& /*vUserDatas*/)
{
	std::string str;

	//str += vOffset + "<blur_radius>" + ct::toStr(m_UBOComp.u_blur_radius) + "</blur_radius>\n";
	
	return str;
}

bool DiffuseModule_Pass::setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& /*vUserDatas*/)
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

	if (strParentName == "diffuse_module")
	{
		//if (strName == "blur_radius")
		//	m_UBOComp.u_blur_radius = ct::uvariant(strValue).GetU();

		NeedNewUBOUpload();
	}

	return true;
}