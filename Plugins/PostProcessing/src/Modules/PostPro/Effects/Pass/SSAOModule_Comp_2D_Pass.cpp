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

#include "SSAOModule_Comp_2D_Pass.h"

#include <functional>

#include <ctools/Logger.h>
#include <ctools/FileHelper.h>
#include <ImWidgets.h>
#include <LumoBackend/Systems/CommonSystem.h>

#include <Gaia/Core/VulkanCore.h>
#include <Gaia/Shader/VulkanShader.h>
#include <Gaia/Core/VulkanSubmitter.h>
#include <LumoBackend/Utils/Mesh/VertexStruct.h>
#include <cinttypes>
#include <Gaia/Buffer/FrameBuffer.h>

using namespace GaiApi;

#ifdef PROFILER_INCLUDE
#include <Gaia/gaia.h>
#include PROFILER_INCLUDE
#endif
#ifndef ZoneScoped
#define ZoneScoped
#endif

//////////////////////////////////////////////////////////////
//// SSAO FIRST PASS : AO ////////////////////////////////////
//////////////////////////////////////////////////////////////

SSAOModule_Comp_2D_Pass::SSAOModule_Comp_2D_Pass(GaiApi::VulkanCorePtr vVulkanCorePtr) : ShaderPass(vVulkanCorePtr) {
    SetRenderDocDebugName("Comp Pass : SSAO", COMPUTE_SHADER_PASS_DEBUG_COLOR);

    m_DontUseShaderFilesOnDisk = true;
}

SSAOModule_Comp_2D_Pass::~SSAOModule_Comp_2D_Pass() {
    Unit();
}

void SSAOModule_Comp_2D_Pass::Compute(vk::CommandBuffer* vCmdBuffer, const int& vIterationNumber) {
    if (vCmdBuffer) {
        vCmdBuffer->bindPipeline(vk::PipelineBindPoint::eCompute, m_Pipelines[0].m_Pipeline);
        {
            // VKFPScoped(*vCmdBuffer, "SSAO", "Compute");

            vCmdBuffer->bindDescriptorSets(
                vk::PipelineBindPoint::eCompute, m_Pipelines[0].m_PipelineLayout, 0, m_DescriptorSets[0].m_DescriptorSet, nullptr);

            for (uint32_t iter = 0; iter < m_CountIterations.w; iter++) {
                Dispatch(vCmdBuffer);
            }
        }
    }
}

bool SSAOModule_Comp_2D_Pass::DrawWidgets(const uint32_t& vCurrentFrame, ImGuiContext* vContextPtr, const std::string& vUserDatas) {
    assert(vContextPtr);
    ImGui::SetCurrentContext(vContextPtr);

    if (ImGui::CollapsingHeader("SSAO", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool change = false;

        if (ImGui::CollapsingHeader("Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
            change |= ImGui::SliderFloatDefaultCompact(0.0f, "Noise Scale", &m_UBOComp.u_noise_scale, 0.0f, 2.0f, 1.0f);
            change |= ImGui::SliderFloatDefaultCompact(0.0f, "Radius", &m_UBOComp.u_ao_radius, 0.0f, 0.25f, 0.01f);
            change |= ImGui::SliderFloatDefaultCompact(0.0f, "Scale", &m_UBOComp.u_ao_scale, 0.0f, 1.0f, 1.0f);
            change |= ImGui::SliderFloatDefaultCompact(0.0f, "Bias", &m_UBOComp.u_ao_bias, 0.0f, 0.1f, 0.001f);
            change |= ImGui::SliderFloatDefaultCompact(0.0f, "Intensity", &m_UBOComp.u_ao_intensity, 0.0f, 5.0f, 2.0f);

            if (change) {
                NeedNewUBOUpload();
            }
        }

        DrawInputTexture(m_VulkanCorePtr, "Input Position", 0U, m_OutputRatio);
        DrawInputTexture(m_VulkanCorePtr, "Input Normal", 1U, m_OutputRatio);
        DrawInputTexture(m_VulkanCorePtr, "Input Blue Noise", 2U, m_OutputRatio);

        return change;
    }

    return false;
}

bool SSAOModule_Comp_2D_Pass::DrawOverlays(
    const uint32_t& vCurrentFrame, const ImRect& vRect, ImGuiContext* vContextPtr, const std::string& vUserDatas) {
    assert(vContextPtr);
    ImGui::SetCurrentContext(vContextPtr);
    return false;
}

bool SSAOModule_Comp_2D_Pass::DrawDialogsAndPopups(
    const uint32_t& vCurrentFrame, const ImVec2& vMaxSize, ImGuiContext* vContextPtr, const std::string& vUserDatas) {
    assert(vContextPtr);
    ImGui::SetCurrentContext(vContextPtr);
    return false;
}

void SSAOModule_Comp_2D_Pass::SetTexture(const uint32_t& vBindingPoint, vk::DescriptorImageInfo* vImageInfo, ct::fvec2* vTextureSize) {
    ZoneScoped;

    if (m_Loaded) {
        if (vBindingPoint < m_ImageInfos.size()) {
            if (vImageInfo) {
                if (vTextureSize) {
                    m_ImageInfosSize[vBindingPoint] = *vTextureSize;
                    NeedResizeByHandIfChanged(m_ImageInfosSize[0]); // pos and nor must have the same size, but noise no
                }

                m_ImageInfos[vBindingPoint] = *vImageInfo;

                if ((&m_UBOComp.use_sampler_pos)[vBindingPoint] < 1.0f) {
                    (&m_UBOComp.use_sampler_pos)[vBindingPoint] = 1.0f;
                    NeedNewUBOUpload();
                }
            } else {
                if ((&m_UBOComp.use_sampler_pos)[vBindingPoint] > 0.0f) {
                    (&m_UBOComp.use_sampler_pos)[vBindingPoint] = 0.0f;
                    NeedNewUBOUpload();
                }

                m_ImageInfos[vBindingPoint] = *m_VulkanCorePtr->getEmptyTexture2DDescriptorImageInfo();
            }
        }
    }
}

vk::DescriptorImageInfo* SSAOModule_Comp_2D_Pass::GetDescriptorImageInfo(const uint32_t& vBindingPoint, ct::fvec2* vOutSize) {
    if (m_ComputeBufferPtr) {
        AutoResizeBuffer(std::dynamic_pointer_cast<OutputSizeInterface>(m_ComputeBufferPtr).get(), vOutSize);
        return m_ComputeBufferPtr->GetFrontDescriptorImageInfo(vBindingPoint);
    }
    return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// PRIVATE ///////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool SSAOModule_Comp_2D_Pass::CreateUBO() {
    ZoneScoped;

    auto size_in_bytes = sizeof(UBOComp);
    m_UBOCompPtr = VulkanRessource::createUniformBufferObject(m_VulkanCorePtr, size_in_bytes);
    m_DescriptorBufferInfo_Comp.buffer = m_UBOCompPtr->buffer;
    m_DescriptorBufferInfo_Comp.range = size_in_bytes;
    m_DescriptorBufferInfo_Comp.offset = 0;

    for (auto& info : m_ImageInfos) {
        info = *m_VulkanCorePtr->getEmptyTexture2DDescriptorImageInfo();
    }

    NeedNewUBOUpload();

    return true;
}

void SSAOModule_Comp_2D_Pass::UploadUBO() {
    ZoneScoped;

    VulkanRessource::upload(m_VulkanCorePtr, m_UBOCompPtr, &m_UBOComp, sizeof(UBOComp));
}

void SSAOModule_Comp_2D_Pass::DestroyUBO() {
    ZoneScoped;

    m_UBOCompPtr.reset();
}

bool SSAOModule_Comp_2D_Pass::UpdateLayoutBindingInRessourceDescriptor() {
    ZoneScoped;
    if (m_ComputeBufferPtr) {
        bool res = true;
        res &= AddOrSetLayoutDescriptor(0U, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute);  // output
        res &= AddOrSetLayoutDescriptor(1U, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute);
        res &= AddOrSetLayoutDescriptor(2U, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eCompute);
        res &= AddOrSetLayoutDescriptor(3U, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eCompute);
        res &= AddOrSetLayoutDescriptor(4U, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eCompute);
        return res;
    }
    return false;
}

bool SSAOModule_Comp_2D_Pass::UpdateBufferInfoInRessourceDescriptor() {
    ZoneScoped;
    if (m_ComputeBufferPtr) {
        bool res = true;
        res &= AddOrSetWriteDescriptorImage(0U, vk::DescriptorType::eStorageImage, m_ComputeBufferPtr->GetFrontDescriptorImageInfo(0U));  // output
        res &= AddOrSetWriteDescriptorBuffer(1U, vk::DescriptorType::eUniformBuffer, &m_DescriptorBufferInfo_Comp);
        res &= AddOrSetWriteDescriptorImage(2U, vk::DescriptorType::eCombinedImageSampler, &m_ImageInfos[0]);  // depth
        res &= AddOrSetWriteDescriptorImage(3U, vk::DescriptorType::eCombinedImageSampler, &m_ImageInfos[1]);  // normal
        res &= AddOrSetWriteDescriptorImage(4U, vk::DescriptorType::eCombinedImageSampler, &m_ImageInfos[2]);  // RGB noise
        return res;
    }
    return false;
}

std::string SSAOModule_Comp_2D_Pass::GetComputeShaderCode(std::string& vOutShaderName) {
    vOutShaderName = "SSAOModule_Comp_2D_Pass";

    SetLocalGroupSize(ct::uvec3(8U, 8U, 1U));

    return u8R"(#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1 ) in;

layout(binding = 0, rgba32f) uniform writeonly image2D outColor;

layout (std140, binding = 1) uniform UBO_Comp {
	float use_sampler_pos;
	float use_sampler_nor;
	float use_sampler_noise;
	float u_noise_scale;
	float u_ao_radius;
	float u_ao_scale;
	float u_ao_bias;
	float u_ao_intensity;
};
layout(binding = 2) uniform sampler2D pos_map_sampler;
layout(binding = 3) uniform sampler2D nor_map_sampler;
layout(binding = 4) uniform sampler2D noise_map_sampler;

// https://www.gamedev.net/tutorials/programming/graphics/a-simple-and-practical-approach-to-ssao-r2753/

vec3 getPos(vec2 uv) {
	return texture(pos_map_sampler, uv).xyz;
}

vec3 getNor(vec2 uv) {
	return texture(nor_map_sampler, uv).xyz * 2.0 - 1.0;
}

vec2 getNoi(vec2 uv) {
	return normalize(texture(noise_map_sampler, uv * u_noise_scale).xy * 2.0 - 1.0); 
}
	
float getAO(vec2 uv, vec2 off, vec3 p, vec3 n) {
	vec3 diff = getPos(uv + off) - p; 
	vec3 v = normalize(diff); 
	float d = length(diff) * u_ao_scale; 
	return max(0.0, dot(n, v) - u_ao_bias) * (1.0 / (1.0 + d)) * u_ao_intensity;
}

void main() {
	const ivec2 coords = ivec2(gl_GlobalInvocationID.xy);
	const vec2 outputSize = vec2(imageSize(outColor));
	vec4 res = vec4(0.0);
	if (use_sampler_pos > 0.5 && use_sampler_nor > 0.5)	{
		const vec2 uv = vec2(coords) / outputSize;
		const vec3 pos = getPos(uv);
		if (dot(pos, pos) > 0.0) {	
			const vec2 vec[4] = { vec2(1, 0), vec2(-1, 0), vec2(0, 1), vec2(0, -1) };
			const vec3 p = getPos(uv); 
			const vec3 n = getNor(uv); 
			const vec2 rand = getNoi(uv); 
			float occ = 0.0; 
			const int iterations = 4; 
			for (int j = 0; j < iterations; ++j) {	
				vec2 c1 = reflect(vec[j], rand) * u_ao_radius; 
				vec2 c2 = vec2(c1.x - c1.y, c1.x + c1.y) * 0.707; 
				occ += getAO(uv, c1 * 0.25, p, n); 
				occ += getAO(uv, c2 * 0.50, p, n); 
				occ += getAO(uv, c1 * 0.75, p, n); 
				occ += getAO(uv, c2 * 1.00, p, n); 
			}
			occ /= float(iterations) * 4.0; 	
			res = vec4(1.0 - occ);
		} else {
			//discard;
		}
	} else {
		//discard;
	}
	imageStore(outColor, coords, res);
}
)";
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// CONFIGURATION /////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::string SSAOModule_Comp_2D_Pass::getXml(const std::string& vOffset, const std::string& /*vUserDatas*/) {
    std::string str;

    str += vOffset + "<noise_scale>" + ct::toStr(m_UBOComp.u_noise_scale) + "</noise_scale>\n";
    str += vOffset + "<ao_radius>" + ct::toStr(m_UBOComp.u_ao_radius) + "</ao_radius>\n";
    str += vOffset + "<ao_scale>" + ct::toStr(m_UBOComp.u_ao_scale) + "</ao_scale>\n";
    str += vOffset + "<ao_bias>" + ct::toStr(m_UBOComp.u_ao_bias) + "</ao_bias>\n";
    str += vOffset + "<ao_intensity>" + ct::toStr(m_UBOComp.u_ao_intensity) + "</ao_intensity>\n";

    return str;
}

bool SSAOModule_Comp_2D_Pass::setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& /*vUserDatas*/) {
    // The value of this child identifies the name of this element
    std::string strName;
    std::string strValue;
    std::string strParentName;

    strName = vElem->Value();
    if (vElem->GetText())
        strValue = vElem->GetText();
    if (vParent != nullptr)
        strParentName = vParent->Value();

    if (strParentName == "ssao_module") {
        if (strName == "noise_scale")
            m_UBOComp.u_noise_scale = ct::fvariant(strValue).GetF();
        else if (strName == "ao_radius")
            m_UBOComp.u_ao_radius = ct::fvariant(strValue).GetF();
        else if (strName == "ao_scale")
            m_UBOComp.u_ao_scale = ct::fvariant(strValue).GetF();
        else if (strName == "ao_bias")
            m_UBOComp.u_ao_bias = ct::fvariant(strValue).GetF();
        else if (strName == "ao_intensity")
            m_UBOComp.u_ao_intensity = ct::fvariant(strValue).GetF();

        NeedNewUBOUpload();
    }

    return true;
}
