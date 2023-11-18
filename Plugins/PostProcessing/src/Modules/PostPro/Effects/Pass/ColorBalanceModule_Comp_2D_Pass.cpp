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

// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "ColorBalanceModule_Comp_2D_Pass.h"

#include <cinttypes>
#include <functional>
#include <ctools/Logger.h>
#include <ctools/FileHelper.h>
#include <ImGuiPack.h>
#include <LumoBackend/Systems/CommonSystem.h>
#include <Gaia/Core/VulkanCore.h>
#include <Gaia/Shader/VulkanShader.h>
#include <Gaia/Core/VulkanSubmitter.h>
#include <LumoBackend/Utils/Mesh/VertexStruct.h>
#include <Gaia/Buffer/FrameBuffer.h>

using namespace GaiApi;

#ifdef PROFILER_INCLUDE
#include PROFILER_INCLUDE
#endif
#ifndef ZoneScoped
#define ZoneScoped
#endif

//////////////////////////////////////////////////////////////
///// STATIC /////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

std::shared_ptr<ColorBalanceModule_Comp_2D_Pass> ColorBalanceModule_Comp_2D_Pass::Create(const ct::uvec2& vSize, GaiApi::VulkanCorePtr vVulkanCorePtr) {
	auto res_ptr = std::make_shared<ColorBalanceModule_Comp_2D_Pass>(vVulkanCorePtr);
	if (!res_ptr->InitCompute2D(vSize, 1U, false, vk::Format::eR32G32B32A32Sfloat)) {
		res_ptr.reset();
	}
	return res_ptr;
}

//////////////////////////////////////////////////////////////
///// CTOR / DTOR ////////////////////////////////////////////
//////////////////////////////////////////////////////////////

ColorBalanceModule_Comp_2D_Pass::ColorBalanceModule_Comp_2D_Pass(GaiApi::VulkanCorePtr vVulkanCorePtr)
	: ShaderPass(vVulkanCorePtr) {
	ZoneScoped;
	SetRenderDocDebugName("Comp Pass : Color Balance", COMPUTE_SHADER_PASS_DEBUG_COLOR);
	m_DontUseShaderFilesOnDisk = true;
	*IsEffectEnabled() = true;
}

ColorBalanceModule_Comp_2D_Pass::~ColorBalanceModule_Comp_2D_Pass()
{
	ZoneScoped;

	Unit();
}

void ColorBalanceModule_Comp_2D_Pass::ActionBeforeInit()
{
	ZoneScoped;

	//m_CountIterations = ct::uvec4(0U, 10U, 1U, 1U);

	for (auto& info : m_ImageInfos)
	{
		info = *m_VulkanCorePtr->getEmptyTexture2DDescriptorImageInfo();
	}
}

bool ColorBalanceModule_Comp_2D_Pass::DrawWidgets(const uint32_t& vCurrentFrame, ImGuiContext* vContextPtr, const std::string& vUserDatas)
{
	ZoneScoped;
	assert(vContextPtr); 
	ImGui::SetCurrentContext(vContextPtr);
	bool change = false;
	//change |= DrawResizeWidget();

	if (ImGui::CollapsingHeader_CheckBox("Color Balance##ColorBalanceModule_Comp_2D_Pass", -1.0f, false, true, IsEffectEnabled())) {
		change |= ImGui::SliderFloatDefaultCompact(0.0f, "red", &m_UBO_Comp.u_red, 0.000f, 0.000f, 0.000f, 0.0f, "%.3f");
		change |= ImGui::SliderFloatDefaultCompact(0.0f, "green", &m_UBO_Comp.u_green, 0.000f, 0.000f, 0.000f, 0.0f, "%.3f");
		change |= ImGui::SliderFloatDefaultCompact(0.0f, "blue", &m_UBO_Comp.u_blue, 0.000f, 0.000f, 0.000f, 0.0f, "%.3f");

		if (change)
		{
			NeedNewUBOUpload();
		}
	}

	return change;
}

bool ColorBalanceModule_Comp_2D_Pass::DrawOverlays(const uint32_t& vCurrentFrame, const ImRect& vRect, ImGuiContext* vContextPtr, const std::string& vUserDatas)
{
	ZoneScoped;
	assert(vContextPtr); 
	ImGui::SetCurrentContext(vContextPtr);
	return false;
}

bool ColorBalanceModule_Comp_2D_Pass::DrawDialogsAndPopups(const uint32_t& vCurrentFrame, const ImVec2& vMaxSize, ImGuiContext* vContextPtr, const std::string& vUserDatas)
{
	ZoneScoped;
	assert(vContextPtr); 
	ImGui::SetCurrentContext(vContextPtr);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//// TEXTURE SLOT INPUT //////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

void ColorBalanceModule_Comp_2D_Pass::SetTexture(const uint32_t& vBindingPoint, vk::DescriptorImageInfo* vImageInfo, ct::fvec2* vTextureSize)
{	
	ZoneScoped;

	if (m_Loaded)
	{
		if (vBindingPoint < m_ImageInfos.size())
		{
			if (vImageInfo)
			{
				if (vTextureSize)
				{
					m_ImageInfosSize[vBindingPoint] = *vTextureSize;

					NeedResizeByHandIfChanged(m_ImageInfosSize[vBindingPoint]);
				}

				m_ImageInfos[vBindingPoint] = *vImageInfo;
			}
			else
			{
				m_ImageInfos[vBindingPoint] = *m_VulkanCorePtr->getEmptyTexture2DDescriptorImageInfo();
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////
//// TEXTURE SLOT OUTPUT /////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

vk::DescriptorImageInfo* ColorBalanceModule_Comp_2D_Pass::GetDescriptorImageInfo(const uint32_t& vBindingPoint, ct::fvec2* vOutSize)
{	
	ZoneScoped;
	if (m_ComputeBufferPtr) {
        AutoResizeBuffer(std::dynamic_pointer_cast<OutputSizeInterface>(m_ComputeBufferPtr).get(), vOutSize);
		return m_ComputeBufferPtr->GetFrontDescriptorImageInfo(vBindingPoint);
	}
	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// PRIVATE ///////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ColorBalanceModule_Comp_2D_Pass::WasJustResized()
{
	ZoneScoped;
}

void ColorBalanceModule_Comp_2D_Pass::Compute(vk::CommandBuffer* vCmdBufferPtr, const int& vIterationNumber)
{
	if (vCmdBufferPtr)
	{
		vCmdBufferPtr->bindPipeline(vk::PipelineBindPoint::eCompute, m_Pipelines[0].m_Pipeline);
		{
			//VKFPScoped(*vCmdBufferPtr, "Color Balance", "Compute");

			vCmdBufferPtr->bindDescriptorSets(vk::PipelineBindPoint::eCompute, m_Pipelines[0].m_PipelineLayout, 0, m_DescriptorSets[0].m_DescriptorSet, nullptr);

			for (uint32_t iter = 0; iter < m_CountIterations.w; iter++)
			{
				Dispatch(vCmdBufferPtr);
			}
		}
	}
}


bool ColorBalanceModule_Comp_2D_Pass::CreateUBO() {
	ZoneScoped;

	m_UBO_Comp_Ptr = VulkanRessource::createUniformBufferObject(m_VulkanCorePtr, sizeof(UBO_Comp));
	m_UBO_Comp_BufferInfos = vk::DescriptorBufferInfo{ VK_NULL_HANDLE, 0, VK_WHOLE_SIZE };
	if (m_UBO_Comp_Ptr) {
		m_UBO_Comp_BufferInfos.buffer = m_UBO_Comp_Ptr->buffer;
		m_UBO_Comp_BufferInfos.range = sizeof(UBO_Comp);
		m_UBO_Comp_BufferInfos.offset = 0;
	}

	NeedNewUBOUpload();

	return true;
}

void ColorBalanceModule_Comp_2D_Pass::UploadUBO() {
	ZoneScoped;
    assert(IsEffectEnabled() != nullptr);
    m_UBO_Comp.u_enabled = (*IsEffectEnabled()) ? 1.0f : 0.0f;
	VulkanRessource::upload(m_VulkanCorePtr, m_UBO_Comp_Ptr, &m_UBO_Comp, sizeof(UBO_Comp));
}

void ColorBalanceModule_Comp_2D_Pass::DestroyUBO() {
	ZoneScoped;

	m_UBO_Comp_Ptr.reset();
	m_UBO_Comp_BufferInfos = vk::DescriptorBufferInfo{ VK_NULL_HANDLE, 0, VK_WHOLE_SIZE };
}

bool ColorBalanceModule_Comp_2D_Pass::UpdateLayoutBindingInRessourceDescriptor()
{
	ZoneScoped;

	bool res = true;
	res &= AddOrSetLayoutDescriptor(0U, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute);
	res &= AddOrSetLayoutDescriptor(1U, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eCompute); // sampler input 
	res &= AddOrSetLayoutDescriptor(2U, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute); // color output 


	return res;
}

bool ColorBalanceModule_Comp_2D_Pass::UpdateBufferInfoInRessourceDescriptor()
{
	ZoneScoped;

	bool res = true;
	res &= AddOrSetWriteDescriptorBuffer(0U, vk::DescriptorType::eUniformBuffer, &m_UBO_Comp_BufferInfos);
	res &= AddOrSetWriteDescriptorImage(1U, vk::DescriptorType::eCombinedImageSampler, &m_ImageInfos[0U]);  // sampler input
	res &= AddOrSetWriteDescriptorImage(2U, vk::DescriptorType::eStorageImage, m_ComputeBufferPtr->GetFrontDescriptorImageInfo(0U)); // output

	
	return res;
}

std::string ColorBalanceModule_Comp_2D_Pass::GetComputeShaderCode(std::string& vOutShaderName)
{
	vOutShaderName = "ColorBalanceModule_Comp_2D_Pass_Compute";

	SetLocalGroupSize(ct::uvec3(1U, 1U, 1U));

	return u8R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1 ) in;


layout(std140, binding = 0) uniform UBO_Comp
{
	float u_red;
	float u_green;
	float u_blue;
	float u_enabled;
};

layout(binding = 1) uniform sampler2D input_new_slot_map;

layout(binding = 2, rgba32f) uniform image2D colorBuffer; // output


void main()
{
	const ivec2 coords = ivec2(gl_GlobalInvocationID.xy);

	vec4 color = vec4(coords, 0, 1);

	imageStore(colorBuffer, coords, color); 
}
)";
		}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// CONFIGURATION /////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::string ColorBalanceModule_Comp_2D_Pass::getXml(const std::string& vOffset, const std::string& vUserDatas)
{
	ZoneScoped;
	std::string str;
    str += vOffset + "<color_balance_pass>\n";
	str += ShaderPass::getXml(vOffset + "\t", vUserDatas);
	str += vOffset + "\t<red>" + ct::toStr(m_UBO_Comp.u_red) + "</red>\n";
	str += vOffset + "\t<green>" + ct::toStr(m_UBO_Comp.u_green) + "</green>\n";
	str += vOffset + "\t<blue>" + ct::toStr(m_UBO_Comp.u_blue) + "</blue>\n";
	str += vOffset + "\t<enabled>" + ct::toStr(m_UBO_Comp.u_enabled) + "</enabled>\n";
    str += vOffset + "</color_balance_pass>\n";
	return str;
}

bool ColorBalanceModule_Comp_2D_Pass::setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& vUserDatas)
{
	ZoneScoped;

	// The value of this child identifies the name of this element
	std::string strName;
	std::string strValue;
	std::string strParentName;

	strName = vElem->Value();
	if (vElem->GetText()) {
		strValue = vElem->GetText();
	}
	if (vParent != nullptr) {
		strParentName = vParent->Value();
	}

	if (strParentName == "color_balance_pass") {
		ShaderPass::setFromXml(vElem, vParent, vUserDatas);
		if (strName == "red") {
			m_UBO_Comp.u_red = ct::fvariant(strValue).GetF();
		} else if (strName == "green") {
			m_UBO_Comp.u_green = ct::fvariant(strValue).GetF();
		} else if (strName == "blue") {
			m_UBO_Comp.u_blue = ct::fvariant(strValue).GetF();
		} else if (strName == "enabled") {
			m_UBO_Comp.u_enabled = ct::fvariant(strValue).GetF();
			*IsEffectEnabled() = m_UBO_Comp.u_enabled;
		}
	}

	return true;
}

void ColorBalanceModule_Comp_2D_Pass::AfterNodeXmlLoading()
{
	ZoneScoped;
	NeedNewUBOUpload();
}
