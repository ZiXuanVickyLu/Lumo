/*
MIT License

Copyright (c) 2022-2022 Stephane Cuillerdier (aka aiekick)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "MeshAttributesModule_Pass.h"

#include <functional>

#include <Gui/MainFrame.h>

#include <ctools/Logger.h>
#include <ctools/FileHelper.h>

#include <Base/FrameBuffer.h>

#include <ImWidgets/ImWidgets.h>

#include <Systems/CommonSystem.h>

#include <Profiler/vkProfiler.hpp>

#include <vkFramework/VulkanCore.h>
#include <vkFramework/VulkanShader.h>

using namespace vkApi;

//////////////////////////////////////////////////////////////
//// FIRST PASS //////////////////////////////////////////////
//////////////////////////////////////////////////////////////

MeshAttributesModule_Pass::MeshAttributesModule_Pass(vkApi::VulkanCore* vVulkanCore)
	: ShaderPass(vVulkanCore)
{
	SetRenderDocDebugName("Mesh Pass 1 : Mesh Attributes", MESH_SHADER_PASS_DEBUG_COLOR);
}

MeshAttributesModule_Pass::~MeshAttributesModule_Pass()
{
	Unit();
}

bool MeshAttributesModule_Pass::DrawWidgets(const uint32_t& vCurrentFrame, ImGuiContext* vContext)
{
	if (ImGui::CollapsingHeader("Attributes"))
	{
		bool change = false;

		DrawInputTexture(m_VulkanCore, "Input Mask", 0U, m_OutputRatio);

		return change;
	}

	return false;
}

void MeshAttributesModule_Pass::DrawOverlays(const uint32_t& vCurrentFrame, const ct::frect& vRect, ImGuiContext* vContext)
{

}

void MeshAttributesModule_Pass::DisplayDialogsAndPopups(const uint32_t& vCurrentFrame, const ct::ivec2& vMaxSize, ImGuiContext* vContext)
{

}

void MeshAttributesModule_Pass::SetModel(SceneModelWeak vSceneModel)
{
	ZoneScoped;

	m_SceneModel = vSceneModel;

	m_NeedModelUpdate = true;
}

void MeshAttributesModule_Pass::SetTexture(const uint32_t& vBinding, vk::DescriptorImageInfo* vImageInfo)
{
	ZoneScoped;

	if (m_Loaded)
	{
		if (vBinding < m_ImageInfos.size())
		{
			if (vImageInfo)
			{
				m_ImageInfos[vBinding] = *vImageInfo;

				if ((&m_UBOFrag.use_sampler_mask)[vBinding] < 1.0f)
				{
					(&m_UBOFrag.use_sampler_mask)[vBinding] = 1.0f;
					NeedNewUBOUpload();
				}
			}
			else
			{
				if ((&m_UBOFrag.use_sampler_mask)[vBinding] > 0.0f)
				{
					(&m_UBOFrag.use_sampler_mask)[vBinding] = 0.0f;
					NeedNewUBOUpload();
				}

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

vk::DescriptorImageInfo* MeshAttributesModule_Pass::GetDescriptorImageInfo(const uint32_t& vBindingPoint)
{
	if (m_FrameBufferPtr)
	{
		return m_FrameBufferPtr->GetFrontDescriptorImageInfo(vBindingPoint);
	}

	return nullptr;
}

void MeshAttributesModule_Pass::DrawModel(vk::CommandBuffer* vCmdBuffer, const int& vIterationNumber)
{
	ZoneScoped;

	if (!m_Loaded) return;

	if (vCmdBuffer)
	{
		auto modelPtr = m_SceneModel.getValidShared();
		if (!modelPtr || modelPtr->empty()) return;

		vCmdBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, m_Pipeline);
		{
			VKFPScoped(*vCmdBuffer, "MeshAttributesModule_Pass", "DrawModel");

			vCmdBuffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_PipelineLayout, 0, m_DescriptorSet, nullptr);

			for (auto meshPtr : *modelPtr)
			{
				if (meshPtr)
				{
					vk::DeviceSize offsets = 0;
					vCmdBuffer->bindVertexBuffers(0, meshPtr->GetVerticesBuffer(), offsets);

					if (meshPtr->GetIndicesCount())
					{
						vCmdBuffer->bindIndexBuffer(meshPtr->GetIndicesBuffer(), 0, vk::IndexType::eUint32);
						vCmdBuffer->drawIndexed(meshPtr->GetIndicesCount(), 1, 0, 0, 0);
					}
					else
					{
						vCmdBuffer->draw(meshPtr->GetVerticesCount(), 1, 0, 0);
					}
				}
			}
		}
	}
}

void MeshAttributesModule_Pass::DestroyModel(const bool& vReleaseDatas)
{
	ZoneScoped;

	if (vReleaseDatas)
	{
		m_SceneModel.reset();
	}
}

bool MeshAttributesModule_Pass::CreateUBO()
{
	ZoneScoped;

	m_UBO_Vert = VulkanRessource::createUniformBufferObject(m_VulkanCore, sizeof(UBOVert));
	if (m_UBO_Vert)
	{
		m_DescriptorBufferInfo_Vert.buffer = m_UBO_Vert->buffer;
		m_DescriptorBufferInfo_Vert.range = sizeof(UBOVert);
		m_DescriptorBufferInfo_Vert.offset = 0;
	}

	m_UBO_Frag = VulkanRessource::createUniformBufferObject(m_VulkanCore, sizeof(UBOFrag));
	if (m_UBO_Frag)
	{
		m_DescriptorBufferInfo_Frag.buffer = m_UBO_Frag->buffer;
		m_DescriptorBufferInfo_Frag.range = sizeof(UBOFrag);
		m_DescriptorBufferInfo_Frag.offset = 0;
	}

	m_EmptyTexturePtr = Texture2D::CreateEmptyTexture(m_VulkanCore, ct::uvec2(1, 1), vk::Format::eR8G8B8A8Unorm);

	for (auto& a : m_ImageInfos)
	{
		a = m_EmptyTexturePtr->m_DescriptorImageInfo;
	}

	NeedNewUBOUpload();

	return true;
}

void MeshAttributesModule_Pass::UploadUBO()
{
	ZoneScoped;

	VulkanRessource::upload(m_VulkanCore, *m_UBO_Vert, &m_UBOVert, sizeof(UBOVert));
	VulkanRessource::upload(m_VulkanCore, *m_UBO_Frag, &m_UBOFrag, sizeof(UBOFrag));
}

void MeshAttributesModule_Pass::DestroyUBO()
{
	ZoneScoped;

	m_UBO_Vert.reset();
	m_UBO_Frag.reset();
	m_EmptyTexturePtr.reset();
}

bool MeshAttributesModule_Pass::UpdateLayoutBindingInRessourceDescriptor()
{
	ZoneScoped;

	m_LayoutBindings.clear();
	m_LayoutBindings.emplace_back(0U, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
	m_LayoutBindings.emplace_back(1U, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment);
	m_LayoutBindings.emplace_back(2U, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);

	return true;
}

bool MeshAttributesModule_Pass::UpdateBufferInfoInRessourceDescriptor()
{
	ZoneScoped;

	writeDescriptorSets.clear();
	writeDescriptorSets.emplace_back(m_DescriptorSet, 0U, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, CommonSystem::Instance()->GetBufferInfo());
	writeDescriptorSets.emplace_back(m_DescriptorSet, 1U, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &m_DescriptorBufferInfo_Frag);
	writeDescriptorSets.emplace_back(m_DescriptorSet, 2U, 0, 1, vk::DescriptorType::eCombinedImageSampler, &m_ImageInfos[0], nullptr); // depth

	return true;
}

void MeshAttributesModule_Pass::SetInputStateBeforePipelineCreation()
{
	VertexStruct::P3_N3_TA3_BTA3_T2_C4::GetInputState(m_InputState);
}

std::string MeshAttributesModule_Pass::GetVertexShaderCode(std::string& vOutShaderName)
{
	vOutShaderName = "MeshAttributesModule_Pass";

	return u8R"(#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aTangent;
layout(location = 3) in vec3 aBiTangent;
layout(location = 4) in vec2 aUv;
layout(location = 5) in vec4 aColor;

layout(location = 0) out vec3 vertPosition;
layout(location = 1) out vec3 vertNormal;
layout(location = 2) out vec3 vertTangent;
layout(location = 3) out vec3 vertBiTangent;
layout(location = 4) out vec2 vertUv;
layout(location = 5) out vec4 vertColor;
)"
+ CommonSystem::GetBufferObjectStructureHeader(0U) +
u8R"(
void main() 
{
	vertPosition = aPosition;
	vertNormal = aNormal;
	vertTangent = aTangent;
	vertBiTangent = aBiTangent;
	vertUv = aUv;
	vertColor = aColor;
	gl_Position = cam * vec4(aPosition, 1.0);
}
)";
}

std::string MeshAttributesModule_Pass::GetFragmentShaderCode(std::string& vOutShaderName)
{
	vOutShaderName = "MeshAttributesModule_Pass";

	return u8R"(#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragPos;
layout(location = 1) out vec4 fragNor;
layout(location = 2) out vec4 fragTan;
layout(location = 3) out vec4 fragBTan;
layout(location = 4) out vec4 fragUV;
layout(location = 5) out vec4 fragCol;
layout(location = 6) out vec4 fragDep;

layout(location = 0) in vec3 vertPosition;
layout(location = 1) in vec3 vertNormal;
layout(location = 2) in vec3 vertTangent;
layout(location = 3) in vec3 vertBiTangent;
layout(location = 4) in vec2 vertUv;
layout(location = 5) in vec4 vertColor;
)"
+ CommonSystem::GetBufferObjectStructureHeader(0U) +
u8R"(

layout (std140, binding = 1) uniform UBO_Vert 
{ 
	float use_sampler_mask;
};
layout(binding = 2) uniform sampler2D mask_map_sampler;

void main() 
{
	fragPos = vec4(vertPosition,1);
	fragNor = vec4(vertNormal * 0.5 + 0.5,1);
	fragTan = vec4(vertTangent,1);
	fragBTan = vec4(vertBiTangent,1);
	fragUV = vec4(vertUv,0,1);
	fragCol = vertColor;

	float depth = gl_FragCoord.z / gl_FragCoord.w;
	if (cam_far > 0.0)
		depth /= cam_far;
	fragDep = vec4(vec3(depth), 1.0);

	if (use_sampler_mask > 0.5)
	{
		float mask = texture(mask_map_sampler, vertUv).r;
		if (mask < 0.5)
		{
			discard;
		}
	}
}
)";
}
