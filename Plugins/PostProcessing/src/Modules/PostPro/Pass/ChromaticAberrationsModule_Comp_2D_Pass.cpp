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

#include "ChromaticAberrationsModule_Comp_2D_Pass.h"

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

std::shared_ptr<ChromaticAberrationsModule_Comp_2D_Pass> ChromaticAberrationsModule_Comp_2D_Pass::Create(const ct::uvec2& vSize, GaiApi::VulkanCorePtr vVulkanCorePtr) {
	auto res_ptr = std::make_shared<ChromaticAberrationsModule_Comp_2D_Pass>(vVulkanCorePtr);
	if (!res_ptr->InitCompute2D(vSize, 1U, false, vk::Format::eR32G32B32A32Sfloat)) {
		res_ptr.reset();
	}
	return res_ptr;
}

//////////////////////////////////////////////////////////////
///// CTOR / DTOR ////////////////////////////////////////////
//////////////////////////////////////////////////////////////

ChromaticAberrationsModule_Comp_2D_Pass::ChromaticAberrationsModule_Comp_2D_Pass(GaiApi::VulkanCorePtr vVulkanCorePtr)
	: ShaderPass(vVulkanCorePtr)
{
	ZoneScoped;

	SetRenderDocDebugName("Comp Pass : Chromatic Aberrations", COMPUTE_SHADER_PASS_DEBUG_COLOR);

	//m_DontUseShaderFilesOnDisk = true;
}

ChromaticAberrationsModule_Comp_2D_Pass::~ChromaticAberrationsModule_Comp_2D_Pass()
{
	ZoneScoped;

	Unit();
}

void ChromaticAberrationsModule_Comp_2D_Pass::ActionBeforeInit()
{
	ZoneScoped;

	//m_CountIterations = ct::uvec4(0U, 10U, 1U, 1U);

	for (auto& info : m_ImageInfos)
	{
		info = *m_VulkanCorePtr->getEmptyTexture2DDescriptorImageInfo();
	}
}

bool ChromaticAberrationsModule_Comp_2D_Pass::DrawWidgets(const uint32_t& vCurrentFrame, ImGuiContext* vContextPtr, const std::string& vUserDatas)
{
	ZoneScoped;
	assert(vContextPtr); 
	ImGui::SetCurrentContext(vContextPtr);
	bool change = false;
	change |= DrawResizeWidget();

	change |= ImGui::SliderFloatDefaultCompact(0.0f, "amount", &m_UBO_Comp.u_amount, 0.000f, 0.000f, 0.000f, 0.0f, "%.3f");
	change |= ImGui::SliderFloatDefaultCompact(0.0f, "enabled", &m_UBO_Comp.u_enabled, 0.000f, 0.000f, 0.000f, 0.0f, "%.3f");

	if (change)
	{
		NeedNewUBOUpload();
	}
	return change;
}

bool ChromaticAberrationsModule_Comp_2D_Pass::DrawOverlays(const uint32_t& vCurrentFrame, const ImRect& vRect, ImGuiContext* vContextPtr, const std::string& vUserDatas)
{
	ZoneScoped;
	assert(vContextPtr); 
	ImGui::SetCurrentContext(vContextPtr);
	return false;
}

bool ChromaticAberrationsModule_Comp_2D_Pass::DrawDialogsAndPopups(const uint32_t& vCurrentFrame, const ImVec2& vMaxSize, ImGuiContext* vContextPtr, const std::string& vUserDatas)
{
	ZoneScoped;
	assert(vContextPtr); 
	ImGui::SetCurrentContext(vContextPtr);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//// TEXTURE SLOT INPUT //////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

void ChromaticAberrationsModule_Comp_2D_Pass::SetTexture(const uint32_t& vBindingPoint, vk::DescriptorImageInfo* vImageInfo, ct::fvec2* vTextureSize)
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

vk::DescriptorImageInfo* ChromaticAberrationsModule_Comp_2D_Pass::GetDescriptorImageInfo(const uint32_t& vBindingPoint, ct::fvec2* vOutSize)
{	
	ZoneScoped;
	if (m_ComputeBufferPtr)
	{
		if (vOutSize)
		{
			*vOutSize = m_ComputeBufferPtr->GetOutputSize();
		}

		return m_ComputeBufferPtr->GetFrontDescriptorImageInfo(vBindingPoint);
	}

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// PRIVATE ///////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ChromaticAberrationsModule_Comp_2D_Pass::WasJustResized()
{
	ZoneScoped;
}

void ChromaticAberrationsModule_Comp_2D_Pass::Compute(vk::CommandBuffer* vCmdBufferPtr, const int& vIterationNumber)
{
	if (vCmdBufferPtr)
	{
		vCmdBufferPtr->bindPipeline(vk::PipelineBindPoint::eCompute, m_Pipelines[0].m_Pipeline);
		{
			//VKFPScoped(*vCmdBufferPtr, "Chromatic Aberrations", "Compute");

			vCmdBufferPtr->bindDescriptorSets(vk::PipelineBindPoint::eCompute, m_Pipelines[0].m_PipelineLayout, 0, m_DescriptorSets[0].m_DescriptorSet, nullptr);

			for (uint32_t iter = 0; iter < m_CountIterations.w; iter++)
			{
				Dispatch(vCmdBufferPtr);
			}
		}
	}
}


bool ChromaticAberrationsModule_Comp_2D_Pass::CreateUBO()
{
	ZoneScoped;

	m_UBO_Comp_Ptr = VulkanRessource::createUniformBufferObject(m_VulkanCorePtr, sizeof(UBO_Comp));
	m_UBO_Comp_BufferInfos = vk::DescriptorBufferInfo{ VK_NULL_HANDLE, 0, VK_WHOLE_SIZE };
	if (m_UBO_Comp_Ptr)
	{
		m_UBO_Comp_BufferInfos.buffer = m_UBO_Comp_Ptr->buffer;
		m_UBO_Comp_BufferInfos.range = sizeof(UBO_Comp);
		m_UBO_Comp_BufferInfos.offset = 0;
	}

	NeedNewUBOUpload();

	return true;
}

void ChromaticAberrationsModule_Comp_2D_Pass::UploadUBO()
{
	ZoneScoped;

	VulkanRessource::upload(m_VulkanCorePtr, m_UBO_Comp_Ptr, &m_UBO_Comp, sizeof(UBO_Comp));
}

void ChromaticAberrationsModule_Comp_2D_Pass::DestroyUBO()
{
	ZoneScoped;

	m_UBO_Comp_Ptr.reset();
	m_UBO_Comp_BufferInfos = vk::DescriptorBufferInfo{ VK_NULL_HANDLE, 0, VK_WHOLE_SIZE };
}

bool ChromaticAberrationsModule_Comp_2D_Pass::UpdateLayoutBindingInRessourceDescriptor()
{
	ZoneScoped;

	bool res = true;
	res &= AddOrSetLayoutDescriptor(0U, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute);
	res &= AddOrSetLayoutDescriptor(0U, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute);

	return res;
}

bool ChromaticAberrationsModule_Comp_2D_Pass::UpdateBufferInfoInRessourceDescriptor()
{
	ZoneScoped;

	bool res = true;
	res &= AddOrSetWriteDescriptorBuffer(0U, vk::DescriptorType::eUniformBuffer, &m_UBO_Comp_BufferInfos);
	res &= AddOrSetWriteDescriptorImage(0U, vk::DescriptorType::eStorageImage, m_ComputeBufferPtr->GetFrontDescriptorImageInfo(0U)); // output
	
	return res;
}

std::string ChromaticAberrationsModule_Comp_2D_Pass::GetComputeShaderCode(std::string& vOutShaderName)
{
	vOutShaderName = "ChromaticAberrationsModule_Comp_2D_Pass_Compute";

	SetLocalGroupSize(ct::uvec3(1U, 1U, 1U));

	return u8R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1 ) in;

layout(binding = 0, rgba32f) uniform image2D colorBuffer;

layout(std140, binding = 0) uniform UBO_Comp
{
	float u_amount;
	float u_enabled;
};

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

std::string ChromaticAberrationsModule_Comp_2D_Pass::getXml(const std::string& vOffset, const std::string& vUserDatas)
{
	ZoneScoped;

	std::string str;

	str += ShaderPass::getXml(vOffset, vUserDatas);

	str += vOffset + "<amount>" + ct::toStr(m_UBO_Comp.u_amount) + "</amount>\n";
	str += vOffset + "<enabled>" + ct::toStr(m_UBO_Comp.u_enabled) + "</enabled>\n";

	return str;
}

bool ChromaticAberrationsModule_Comp_2D_Pass::setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& vUserDatas)
{
	ZoneScoped;

	// The value of this child identifies the name of this element
	std::string strName;
	std::string strValue;
	std::string strParentName;

	strName = vElem->Value();
	if (vElem->GetText())
		strValue = vElem->GetText();
	if (vParent != nullptr)
		strParentName = vParent->Value();

	ShaderPass::setFromXml(vElem, vParent, vUserDatas);

	if (strParentName == "chromatic_aberrations_module")
	{

		if (strName == "amount")
			m_UBO_Comp.u_amount = ct::fvariant(strValue).GetF();
		else if (strName == "enabled")
			m_UBO_Comp.u_enabled = ct::fvariant(strValue).GetF();
	}

	return true;
}

void ChromaticAberrationsModule_Comp_2D_Pass::AfterNodeXmlLoading()
{
	ZoneScoped;

	// code to do after end of the xml loading of this node
	// by ex :
	NeedNewUBOUpload();
}
