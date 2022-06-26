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

#include "DeferredRenderer_Pass_1.h"

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

DeferredRenderer_Pass_1::DeferredRenderer_Pass_1(vkApi::VulkanCore* vVulkanCore)
	: QuadShaderPass(vVulkanCore, MeshShaderPassType::PIXEL)
{
	SetRenderDocDebugName("Quad Pass 1 : Deferred", QUAD_SHADER_PASS_DEBUG_COLOR);
}

DeferredRenderer_Pass_1::~DeferredRenderer_Pass_1()
{
	Unit();
}

//////////////////////////////////////////////////////////////
//// OVERRIDES ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////

bool DeferredRenderer_Pass_1::DrawWidgets(const uint32_t& vCurrentFrame, ImGuiContext* vContext)
{
	DrawTexture("Position", 0U);
	DrawTexture("Normal", 1U);
	DrawTexture("Albedo", 2U);
	DrawTexture("Diffuse", 3U);
	DrawTexture("Specular", 4U);
	DrawTexture("Attenuation", 5U);
	DrawTexture("Mask", 6U);
	DrawTexture("Ao", 7U);
	DrawTexture("shadow", 8U);

	return false;
}

void DeferredRenderer_Pass_1::DrawOverlays(const uint32_t& vCurrentFrame, const ct::frect& vRect, ImGuiContext* vContext)
{

}

void DeferredRenderer_Pass_1::DisplayDialogsAndPopups(const uint32_t& vCurrentFrame, const ct::ivec2& vMaxSize, ImGuiContext* vContext)
{

}

void DeferredRenderer_Pass_1::SetTexture(const uint32_t& vBinding, vk::DescriptorImageInfo* vImageInfo)
{
	ZoneScoped;

	if (m_Loaded)
	{
		if (vBinding >= 2U && vBinding <= 10U)
		{
			if (vImageInfo)
			{
				m_SamplesImageInfos[vBinding - 2U] = *vImageInfo;

				if ((&m_UBOFrag.use_sampler_position)[vBinding - 2U] < 1.0f)
				{
					(&m_UBOFrag.use_sampler_position)[vBinding - 2U] = 1.0f;
					NeedNewUBOUpload();
				}
			}
			else
			{
				if ((&m_UBOFrag.use_sampler_position)[vBinding - 2U] > 0.0f)
				{
					(&m_UBOFrag.use_sampler_position)[vBinding - 2U] = 0.0f;
					NeedNewUBOUpload();
				}

				if (m_EmptyTexturePtr)
				{
					m_SamplesImageInfos[vBinding - 2U] = m_EmptyTexturePtr->m_DescriptorImageInfo;
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

vk::DescriptorImageInfo* DeferredRenderer_Pass_1::GetDescriptorImageInfo(const uint32_t& vBindingPoint)
{
	if (m_FrameBufferPtr)
	{
		return m_FrameBufferPtr->GetFrontDescriptorImageInfo(vBindingPoint);
	}

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// CONFIGURATION /////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::string DeferredRenderer_Pass_1::getXml(const std::string& vOffset, const std::string& vUserDatas)
{
	std::string str;

	return str;
}

bool DeferredRenderer_Pass_1::setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& vUserDatas)
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

void DeferredRenderer_Pass_1::UpdateShaders(const std::set<std::string>& vFiles)
{
	bool needReCompil = false;

	if (vFiles.find("shaders/DeferredRenderer_Pass_1.vert") != vFiles.end())
	{
		auto shader_path = FileHelper::Instance()->GetAppPath() + "/shaders/DeferredRenderer_Pass_1.vert";
		if (FileHelper::Instance()->IsFileExist(shader_path))
		{
			m_VertexShaderCode = FileHelper::Instance()->LoadFileToString(shader_path);
			needReCompil = true;
		}

	}
	else if (vFiles.find("shaders/DeferredRenderer_Pass_1.frag") != vFiles.end())
	{
		auto shader_path = FileHelper::Instance()->GetAppPath() + "/shaders/DeferredRenderer_Pass_1.frag";
		if (FileHelper::Instance()->IsFileExist(shader_path))
		{
			m_FragmentShaderCode = FileHelper::Instance()->LoadFileToString(shader_path);
			needReCompil = true;
		}
	}

	if (needReCompil)
	{
		ReCompil();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool DeferredRenderer_Pass_1::CreateUBO()
{
	ZoneScoped;

	auto size_in_bytes = sizeof(UBOFrag);
	m_UBO_Frag = VulkanRessource::createUniformBufferObject(m_VulkanCore, size_in_bytes);
	m_DescriptorBufferInfo_Frag.buffer = m_UBO_Frag->buffer;
	m_DescriptorBufferInfo_Frag.range = size_in_bytes;
	m_DescriptorBufferInfo_Frag.offset = 0;

	m_EmptyTexturePtr = Texture2D::CreateEmptyTexture(m_VulkanCore, ct::uvec2(100, 100), vk::Format::eR8G8B8A8Unorm);

	for (auto& a : m_SamplesImageInfos)
	{
		a = m_EmptyTexturePtr->m_DescriptorImageInfo;
	}

	NeedNewUBOUpload();

	return true;
}

void DeferredRenderer_Pass_1::UploadUBO()
{
	ZoneScoped;

	VulkanRessource::upload(m_VulkanCore, *m_UBO_Frag, &m_UBOFrag, sizeof(UBOFrag));
}

void DeferredRenderer_Pass_1::DestroyUBO()
{
	ZoneScoped;

	m_UBO_Frag.reset();
	m_EmptyTexturePtr.reset();
}

bool DeferredRenderer_Pass_1::UpdateLayoutBindingInRessourceDescriptor()
{
	ZoneScoped;

	m_LayoutBindings.clear();
	m_LayoutBindings.emplace_back(0U, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment);
	m_LayoutBindings.emplace_back(1U, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment);
	m_LayoutBindings.emplace_back(2U, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
	m_LayoutBindings.emplace_back(3U, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
	m_LayoutBindings.emplace_back(4U, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
	m_LayoutBindings.emplace_back(5U, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
	m_LayoutBindings.emplace_back(6U, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
	m_LayoutBindings.emplace_back(7U, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
	m_LayoutBindings.emplace_back(8U, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
	m_LayoutBindings.emplace_back(9U, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
	m_LayoutBindings.emplace_back(10U, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);

	return true;
}

bool DeferredRenderer_Pass_1::UpdateBufferInfoInRessourceDescriptor()
{
	ZoneScoped;

	writeDescriptorSets.clear();
	writeDescriptorSets.emplace_back(m_DescriptorSet, 0U, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, CommonSystem::Instance()->GetBufferInfo());
	writeDescriptorSets.emplace_back(m_DescriptorSet, 1U, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &m_DescriptorBufferInfo_Frag);
	writeDescriptorSets.emplace_back(m_DescriptorSet, 2U, 0, 1, vk::DescriptorType::eCombinedImageSampler, &m_SamplesImageInfos[0], nullptr); // position
	writeDescriptorSets.emplace_back(m_DescriptorSet, 3U, 0, 1, vk::DescriptorType::eCombinedImageSampler, &m_SamplesImageInfos[1], nullptr); // normal
	writeDescriptorSets.emplace_back(m_DescriptorSet, 4U, 0, 1, vk::DescriptorType::eCombinedImageSampler, &m_SamplesImageInfos[2], nullptr); // albedo
	writeDescriptorSets.emplace_back(m_DescriptorSet, 5U, 0, 1, vk::DescriptorType::eCombinedImageSampler, &m_SamplesImageInfos[3], nullptr); // diffuse
	writeDescriptorSets.emplace_back(m_DescriptorSet, 6U, 0, 1, vk::DescriptorType::eCombinedImageSampler, &m_SamplesImageInfos[4], nullptr); // specular
	writeDescriptorSets.emplace_back(m_DescriptorSet, 7U, 0, 1, vk::DescriptorType::eCombinedImageSampler, &m_SamplesImageInfos[5], nullptr); // attenuation
	writeDescriptorSets.emplace_back(m_DescriptorSet, 8U, 0, 1, vk::DescriptorType::eCombinedImageSampler, &m_SamplesImageInfos[6], nullptr); // mask
	writeDescriptorSets.emplace_back(m_DescriptorSet, 9U, 0, 1, vk::DescriptorType::eCombinedImageSampler, &m_SamplesImageInfos[7], nullptr); // ao
	writeDescriptorSets.emplace_back(m_DescriptorSet, 10U, 0, 1, vk::DescriptorType::eCombinedImageSampler, &m_SamplesImageInfos[8], nullptr); // shadow

	return true;
}

std::string DeferredRenderer_Pass_1::GetVertexShaderCode(std::string& vOutShaderName)
{
	vOutShaderName = "DeferredRenderer_Pass_1_Vertex";

	auto shader_path = FileHelper::Instance()->GetAppPath() + "/shaders/DeferredRenderer_Pass_1.vert";

	if (FileHelper::Instance()->IsFileExist(shader_path))
	{
		m_VertexShaderCode = FileHelper::Instance()->LoadFileToString(shader_path);
	}
	else
	{
		m_VertexShaderCode = u8R"(#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 vertPosition;
layout(location = 1) in vec2 vertUv;
layout(location = 0) out vec2 v_uv;

void main() 
{
	v_uv = vertUv;
	gl_Position = vec4(vertPosition, 0.0, 1.0);
}
)";
		FileHelper::Instance()->SaveStringToFile(m_VertexShaderCode, shader_path);
	}

	return m_VertexShaderCode;
}

std::string DeferredRenderer_Pass_1::GetFragmentShaderCode(std::string& vOutShaderName)
{
	vOutShaderName = "DeferredRenderer_Pass_1_Fragment";

	auto shader_path = FileHelper::Instance()->GetAppPath() + "/shaders/DeferredRenderer_Pass_1.frag";

	if (FileHelper::Instance()->IsFileExist(shader_path))
	{
		m_FragmentShaderCode = FileHelper::Instance()->LoadFileToString(shader_path);
	}
	else
	{
		m_FragmentShaderCode = u8R"(#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec2 v_uv;
)"
+ CommonSystem::Instance()->GetBufferObjectStructureHeader(0U) +
u8R"(
layout (std140, binding = 1) uniform UBO_Frag 
{ 
	float use_sampler_position;		// position
	float use_sampler_normal;		// normal
	float use_sampler_albedo;		// albedo
	float use_sampler_diffuse;		// diffuse
	float use_sampler_specular;		// specular
	float use_sampler_attenuation;	// attenuation
	float use_sampler_mask;			// mask
	float use_sampler_ssao;			// ssao
	float use_sampler_shadow;		// shadow
};

layout(binding = 2) uniform sampler2D position_map_sampler;
layout(binding = 3) uniform sampler2D normal_map_sampler;
layout(binding = 4) uniform sampler2D albedo_map_sampler;
layout(binding = 5) uniform sampler2D diffuse_map_sampler;
layout(binding = 6) uniform sampler2D specular_map_sampler;
layout(binding = 7) uniform sampler2D attenuation_map_sampler;
layout(binding = 8) uniform sampler2D mask_map_sampler;
layout(binding = 9) uniform sampler2D ssao_map_sampler;
layout(binding = 10) uniform sampler2D shadow_map_sampler;

void main() 
{
	fragColor = vec4(0);
	
	if (use_sampler_albedo > 0.5)
	{
		fragColor = texture(albedo_map_sampler, v_uv);
	}
	
	if (use_sampler_shadow > 0.5)
	{
		fragColor.rgb *= texture(shadow_map_sampler, v_uv).r;
	}

	if (use_sampler_ssao > 0.5)
	{
		fragColor.rgb *= texture(ssao_map_sampler, v_uv).r;
	}
	
	fragColor.a = 1.0;

	if (use_sampler_mask > 0.5)
	{
		float mask =  texture(mask_map_sampler, v_uv).r;
		if (mask < 0.5)
		{
			discard; // a faire en dernier
		}
	}

	/*
	vec3 pos = texture(gPosition, TexCoords).rgb;
    vec3 normal = texture(gNormal, TexCoords).rgb;
    vec3 color = texture(gAlbedo, TexCoords).rgb;
    float AmbientOcclusion = texture(ssao, TexCoords).r;
    
    // blinn-phong (in view-space)
    vec3 ambient = vec3(0.3 * Diffuse * AmbientOcclusion); // here we add occlusion factor
    vec3 lighting  = ambient; 
    vec3 viewDir  = normalize(-FragPos); // viewpos is (0.0.0) in view-space
    // diffuse
    vec3 lightDir = normalize(light.Position - FragPos);
    vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * light.Color;
    // specular
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(Normal, halfwayDir), 0.0), 8.0);
    vec3 specular = light.Color * spec;
    // attenuation
    float dist = length(light.Position - FragPos);
    float attenuation = 1.0 / (1.0 + light.Linear * dist + light.Quadratic * dist * dist);
    diffuse  *= attenuation;
    specular *= attenuation;
    lighting += diffuse + specular;
    FragColor = vec4(lighting, 1.0);
	*/
}
)";
		FileHelper::Instance()->SaveStringToFile(m_FragmentShaderCode, shader_path);
	}

	return m_FragmentShaderCode;
}

void DeferredRenderer_Pass_1::DrawTexture(const char* vLabel, const uint32_t& vIdx)
{
	if (vLabel && vIdx >= 0U && vIdx <= m_SamplesImageInfos.size())
	{
		auto imguiRendererPtr = m_VulkanCore->GetVulkanImGuiRenderer().getValidShared();
		if (imguiRendererPtr)
		{
			if (ImGui::CollapsingHeader(vLabel, ImGuiTreeNodeFlags_DefaultOpen))
			{
				m_ImGuiTexture[vIdx].SetDescriptor(imguiRendererPtr.get(),
					&m_SamplesImageInfos[vIdx], m_OutputRatio);

				if (m_ImGuiTexture[vIdx].canDisplayPreview)
				{
					int w = (int)ImGui::GetContentRegionAvail().x;
					auto rect = ct::GetScreenRectWithRatio<int32_t>(m_ImGuiTexture[vIdx].ratio, ct::ivec2(w, w), false);
					const ImVec2 pos = ImVec2((float)rect.x, (float)rect.y);
					const ImVec2 siz = ImVec2((float)rect.w, (float)rect.h);
					ImGui::ImageRect((ImTextureID)&m_ImGuiTexture[vIdx].descriptor, pos, siz);
				}
			}
		}
	}
}