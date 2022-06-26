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

#include "ShadowMapModule_Pass_1.h"

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
#include <Generic/FrameBuffer.h>

using namespace vkApi;

//////////////////////////////////////////////////////////////
//// CTOR / DTOR /////////////////////////////////////////////
//////////////////////////////////////////////////////////////

ShadowMapModule_Pass_1::ShadowMapModule_Pass_1(vkApi::VulkanCore* vVulkanCore)
	: ShaderPass(vVulkanCore)
{
	SetRenderDocDebugName("Mesh Pass 1 : Light Shadow Map", MESH_SHADER_PASS_DEBUG_COLOR);
}

ShadowMapModule_Pass_1::~ShadowMapModule_Pass_1()
{
	Unit();
}

//////////////////////////////////////////////////////////////
//// OVERRIDES ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void ShadowMapModule_Pass_1::DrawModel(vk::CommandBuffer* vCmdBuffer, const int& vIterationNumber)
{
	ZoneScoped;

	if (!m_Loaded) return;

	if (vCmdBuffer)
	{
		auto modelPtr = m_SceneModel.getValidShared();
		if (!modelPtr || modelPtr->empty()) return;

		vCmdBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, m_Pipeline);
		{
			VKFPScoped(*vCmdBuffer, "ShadowMapModule_Pass_1", "DrawMesh");

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

bool ShadowMapModule_Pass_1::DrawWidgets(const uint32_t& vCurrentFrame, ImGuiContext* vContext)
{
	bool change = false;

	if (change)
	{
		NeedNewUBOUpload();
	}

	return change;
}

void ShadowMapModule_Pass_1::DrawOverlays(const uint32_t& vCurrentFrame, const ct::frect& vRect, ImGuiContext* vContext)
{

}

void ShadowMapModule_Pass_1::DisplayDialogsAndPopups(const uint32_t& vCurrentFrame, const ct::ivec2& vMaxSize, ImGuiContext* vContext)
{

}

void ShadowMapModule_Pass_1::SetModel(SceneModelWeak vSceneModel)
{
	ZoneScoped;

	m_SceneModel = vSceneModel;

	m_NeedModelUpdate = true;
}

void ShadowMapModule_Pass_1::SetLightGroup(SceneLightGroupWeak vSceneLightGroup)
{
	ZoneScoped;

	m_SceneLightGroup = vSceneLightGroup;

	m_NeedLightGroupUpdate = true;
}

SceneLightGroupWeak ShadowMapModule_Pass_1::GetLightGroup()
{
	ZoneScoped;

	return m_SceneLightGroup;
}

vk::DescriptorImageInfo* ShadowMapModule_Pass_1::GetDescriptorImageInfo(const uint32_t& vBindingPoint)
{
	ZoneScoped;

	if (m_FrameBufferPtr)
	{
		return m_FrameBufferPtr->GetFrontDescriptorImageInfo(vBindingPoint);
	}

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// CONFIGURATION /////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::string ShadowMapModule_Pass_1::getXml(const std::string& vOffset, const std::string& /*vUserDatas*/)
{
	std::string str;

	return str;
}

bool ShadowMapModule_Pass_1::setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& /*vUserDatas*/)
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

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool ShadowMapModule_Pass_1::CreateUBO()
{
	ZoneScoped;

	m_UBO_Vert = VulkanRessource::createUniformBufferObject(m_VulkanCore, sizeof(UBOVert));
	m_DescriptorBufferInfo_Vert.buffer = m_UBO_Vert->buffer;
	m_DescriptorBufferInfo_Vert.range = sizeof(UBOVert);
	m_DescriptorBufferInfo_Vert.offset = 0;

	m_EmptyTexturePtr = Texture2D::CreateEmptyTexture(m_VulkanCore, ct::uvec2(100, 100), vk::Format::eR8G8B8A8Unorm);

	NeedNewUBOUpload();

	return true;
}

void ShadowMapModule_Pass_1::DestroyModel(const bool& vReleaseDatas)
{
	ZoneScoped;

	if (vReleaseDatas)
	{
		m_SceneModel.reset();
	}
}

void ShadowMapModule_Pass_1::UploadUBO()
{
	ZoneScoped;

	VulkanRessource::upload(m_VulkanCore, *m_UBO_Vert, &m_UBOVert, sizeof(UBOVert));
}

void ShadowMapModule_Pass_1::DestroyUBO()
{
	ZoneScoped;

	m_UBO_Vert.reset();
	m_EmptyTexturePtr.reset();
}

bool ShadowMapModule_Pass_1::UpdateLayoutBindingInRessourceDescriptor()
{
	ZoneScoped;

	m_LayoutBindings.clear();
	m_LayoutBindings.emplace_back(0U, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex);
	m_LayoutBindings.emplace_back(1U, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment);

	return true;
}

bool ShadowMapModule_Pass_1::UpdateBufferInfoInRessourceDescriptor()
{
	ZoneScoped;

	writeDescriptorSets.clear();
	writeDescriptorSets.emplace_back(m_DescriptorSet, 0U, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &m_DescriptorBufferInfo_Vert);
	writeDescriptorSets.emplace_back(m_DescriptorSet, 1U, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, CommonSystem::Instance()->GetBufferInfo());

	return true;
}

void ShadowMapModule_Pass_1::UpdateRessourceDescriptor()
{
	ZoneScoped;

	m_Device.waitIdle();

	auto lightGroupPtr = m_SceneLightGroup.getValidShared();
	if (lightGroupPtr && !lightGroupPtr->empty())
	{
		auto lightPtr = lightGroupPtr->Get(0).getValidShared();
		if (lightPtr)
		{
			if (m_NeedLightGroupUpdate)
			{
				lightPtr->NeedUpdateCamera();

				m_UBOVert.light_cam = lightPtr->lightDatas.lightView;

				// va provoquer une nouvel upload dans 
				// MixedMeshRenderer::UpdateRessourceDescriptor();
				// juste apres
				NeedNewUBOUpload();

				m_NeedLightGroupUpdate = false;
			}
		}
	}

	ShaderPass::UpdateRessourceDescriptor();
}

void ShadowMapModule_Pass_1::SetInputStateBeforePipelineCreation()
{
	VertexStruct::P3_N3_TA3_BTA3_T2_C4::GetInputState(m_InputState);
}

std::string ShadowMapModule_Pass_1::GetVertexShaderCode(std::string& vOutShaderName)
{
	vOutShaderName = "ShadowMapModule_Pass_1_Vertex";

	return u8R"(#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 vertPosition;
layout(location = 1) in vec3 vertNormal;
layout(location = 2) in vec3 vertTangent;
layout(location = 3) in vec3 vertBiTangent;
layout(location = 4) in vec2 vertUv;
layout(location = 5) in vec4 vertColor;

layout (std140, binding = 0) uniform UBO_Vert 
{ 
	mat4 light_cam;
};

void main() 
{
	gl_Position = light_cam * vec4(vertPosition, 1.0);
}
)";
}

std::string ShadowMapModule_Pass_1::GetFragmentShaderCode(std::string& vOutShaderName)
{
	vOutShaderName = "ShadowMapModule_Pass_1_Fragment";

	return u8R"(#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out float fragDepth;
)"
+ CommonSystem::Instance()->GetBufferObjectStructureHeader(1U) +
u8R"(
void main() 
{
	float depth = gl_FragCoord.z / gl_FragCoord.w;
	//if (depth > 0.0)
	{
		if (cam_far > 0.0)
			depth /= cam_far;
		fragDepth = depth;
	}
	//else
	{
	//	discard;
	}
}
)";
}
