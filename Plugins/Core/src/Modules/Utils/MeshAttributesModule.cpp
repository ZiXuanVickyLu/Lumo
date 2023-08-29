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

#include "MeshAttributesModule.h"

#include <functional>



#include <ctools/Logger.h>
#include <ctools/FileHelper.h>

#include <Gaia/Buffer/FrameBuffer.h>

#include <ImWidgets.h>

#include <LumoBackend/Systems/CommonSystem.h>



#include <Gaia/Core/VulkanCore.h>
#include <Gaia/Shader/VulkanShader.h>

#include <Modules/Utils/Pass/MeshAttributesModule_Mesh_Pass.h>

using namespace GaiApi;

#ifdef PROFILER_INCLUDE
#include <Gaia/gaia.h>
#include PROFILER_INCLUDE
#endif
#ifndef ZoneScoped
#define ZoneScoped
#endif

//////////////////////////////////////////////////////////////
//// STATIC //////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

std::shared_ptr<MeshAttributesModule> MeshAttributesModule::Create(GaiApi::VulkanCorePtr vVulkanCorePtr)
{
	if (!vVulkanCorePtr) return nullptr;
	auto res = std::make_shared<MeshAttributesModule>(vVulkanCorePtr);
	res->m_This = res;
	if (!res->Init())
	{
		res.reset();
	}
	return res;
}

//////////////////////////////////////////////////////////////
//// CTOR / DTOR /////////////////////////////////////////////
//////////////////////////////////////////////////////////////

MeshAttributesModule::MeshAttributesModule(GaiApi::VulkanCorePtr vVulkanCorePtr)
	: BaseRenderer(vVulkanCorePtr)
{

}

MeshAttributesModule::~MeshAttributesModule()
{
	Unit();
}

//////////////////////////////////////////////////////////////
//// INIT ////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

bool MeshAttributesModule::Init()
{
	ZoneScoped;

	ct::uvec2 map_size = 512;

	m_Loaded = true;

	if (BaseRenderer::InitPixel(map_size))
	{
		m_MeshAttributesModule_Mesh_Pass_Ptr = std::make_shared<MeshAttributesModule_Mesh_Pass>(m_VulkanCorePtr);
		if (m_MeshAttributesModule_Mesh_Pass_Ptr)
		{
			if (m_MeshAttributesModule_Mesh_Pass_Ptr->InitPixel(map_size, 7U, true, true, 0.0f,
				false, false, vk::Format::eR32G32B32A32Sfloat, vk::SampleCountFlagBits::e1))
			{
				AddGenericPass(m_MeshAttributesModule_Mesh_Pass_Ptr);
				m_Loaded = true;
			}
		}
	}

	return m_Loaded;
}

//////////////////////////////////////////////////////////////
//// OVERRIDES ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////

bool MeshAttributesModule::ExecuteAllTime(const uint32_t& vCurrentFrame, vk::CommandBuffer* vCmd, BaseNodeState* vBaseNodeState)
{
	ZoneScoped;

	BaseRenderer::Render("Mesh Attributes Module", vCmd);

	return true;
}

bool MeshAttributesModule::DrawWidgets(const uint32_t& vCurrentFrame, ImGuiContext* vContext, const std::string& vUserDatas)
{
	assert(vContext); ImGui::SetCurrentContext(vContext);

	if (m_LastExecutedFrame == vCurrentFrame)
	{
		if (ImGui::CollapsingHeader_CheckBox("Mesh Attributes", -1.0f, true, true, &m_CanWeRender))
		{
			if (m_MeshAttributesModule_Mesh_Pass_Ptr)
			{
				return m_MeshAttributesModule_Mesh_Pass_Ptr->DrawWidgets(vCurrentFrame, vContext, vUserDatas);
			}
		}
	}

	return false;
}

bool MeshAttributesModule::DrawOverlays(
    const uint32_t& vCurrentFrame, const ImRect& vRect, ImGuiContext* vContext, const std::string& vUserDatas) {
	assert(vContext); ImGui::SetCurrentContext(vContext);

	if (m_LastExecutedFrame == vCurrentFrame)
	{

	}
    return false;
}

bool MeshAttributesModule::DrawDialogsAndPopups(
    const uint32_t& vCurrentFrame, const ImVec2& vMaxSize, ImGuiContext* vContext, const std::string& vUserDatas) {
	assert(vContext); ImGui::SetCurrentContext(vContext);

	if (m_LastExecutedFrame == vCurrentFrame)
	{

	}
    return false;
}

void MeshAttributesModule::NeedResizeByResizeEvent(ct::ivec2* vNewSize, const uint32_t* vCountColorBuffers)
{
	BaseRenderer::NeedResizeByResizeEvent(vNewSize, vCountColorBuffers);
}

void MeshAttributesModule::SetModel(SceneModelWeak vSceneModel)
{
	ZoneScoped;

	if (m_MeshAttributesModule_Mesh_Pass_Ptr)
	{
		m_MeshAttributesModule_Mesh_Pass_Ptr->SetModel(vSceneModel);
	}
}

void MeshAttributesModule::SetTexture(const uint32_t& vBindingPoint, vk::DescriptorImageInfo* vImageInfo, ct::fvec2* vTextureSize)
{
	ZoneScoped;

	if (m_MeshAttributesModule_Mesh_Pass_Ptr)
	{
		m_MeshAttributesModule_Mesh_Pass_Ptr->SetTexture(vBindingPoint, vImageInfo, vTextureSize);
	}
}

vk::DescriptorImageInfo* MeshAttributesModule::GetDescriptorImageInfo(const uint32_t& vBindingPoint, ct::fvec2* vOutSize)
{
	ZoneScoped;

	if (m_MeshAttributesModule_Mesh_Pass_Ptr)
	{
		return m_MeshAttributesModule_Mesh_Pass_Ptr->GetDescriptorImageInfo(vBindingPoint, vOutSize);
	}

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// CONFIGURATION /////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::string MeshAttributesModule::getXml(const std::string& vOffset, const std::string& /*vUserDatas*/)
{
	std::string str;

	return str;
}

bool MeshAttributesModule::setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& /*vUserDatas*/)
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
