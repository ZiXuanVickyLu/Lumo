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

#include "RtxModelShadowModule.h"

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
#include <Modules/Lighting/Pass/ModelShadow_Rtx_Pass.h>

using namespace vkApi;



//////////////////////////////////////////////////////////////
//// STATIC //////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

std::shared_ptr<RtxModelShadowModule> RtxModelShadowModule::Create(vkApi::VulkanCorePtr vVulkanCorePtr)
{
	if (!vVulkanCorePtr) return nullptr;
	auto res = std::make_shared<RtxModelShadowModule>(vVulkanCorePtr);
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

RtxModelShadowModule::RtxModelShadowModule(vkApi::VulkanCorePtr vVulkanCorePtr)
	: BaseRenderer(vVulkanCorePtr)
{

}

RtxModelShadowModule::~RtxModelShadowModule()
{
	Unit();
}

//////////////////////////////////////////////////////////////
//// INIT / UNIT /////////////////////////////////////////////
//////////////////////////////////////////////////////////////

bool RtxModelShadowModule::Init()
{
	ZoneScoped;

	ct::uvec2 map_size = 512;

	m_Loaded = true;

	if (BaseRenderer::InitRtx(map_size))
	{
		m_ModelShadow_Rtx_Pass_Ptr = std::make_shared<ModelShadow_Rtx_Pass>(m_VulkanCorePtr);
		if (m_ModelShadow_Rtx_Pass_Ptr)
		{
			if (m_ModelShadow_Rtx_Pass_Ptr->InitRtx(map_size, 
				1U, false, vk::Format::eR32G32B32A32Sfloat))
			{
				AddGenericPass(m_ModelShadow_Rtx_Pass_Ptr);
				m_Loaded = true;
			}
		}
	}
	return m_Loaded;
}

//////////////////////////////////////////////////////////////
//// OVERRIDES ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////

bool RtxModelShadowModule::ExecuteAllTime(const uint32_t& vCurrentFrame, vk::CommandBuffer* vCmd, BaseNodeState* vBaseNodeState)
{
	ZoneScoped;

	BaseRenderer::Render("Rtx Model Shadow", vCmd);

	return true;
}

bool RtxModelShadowModule::DrawWidgets(const uint32_t& vCurrentFrame, ImGuiContext* vContext)
{
	assert(vContext); ImGui::SetCurrentContext(vContext);

	if (m_LastExecutedFrame == vCurrentFrame)
	{
		if (ImGui::CollapsingHeader_CheckBox("Rtx Model Shadow", -1.0f, true, true, &m_CanWeRender))
		{
			if (m_ModelShadow_Rtx_Pass_Ptr)
			{
				return m_ModelShadow_Rtx_Pass_Ptr->DrawWidgets(vCurrentFrame, vContext);
			}
		}
	}

	return false;
}

void RtxModelShadowModule::DrawOverlays(const uint32_t& vCurrentFrame, const ct::frect& vRect, ImGuiContext* vContext)
{
	assert(vContext); ImGui::SetCurrentContext(vContext);

	if (m_LastExecutedFrame == vCurrentFrame)
	{

	}
}

void RtxModelShadowModule::DisplayDialogsAndPopups(const uint32_t& vCurrentFrame, const ct::ivec2& vMaxSize, ImGuiContext* vContext)
{
	assert(vContext); ImGui::SetCurrentContext(vContext);

	if (m_LastExecutedFrame == vCurrentFrame)
	{

	}
}

void RtxModelShadowModule::SetAccelStructure(SceneAccelStructureWeak vSceneAccelStructure)
{
	if (m_ModelShadow_Rtx_Pass_Ptr)
	{
		m_ModelShadow_Rtx_Pass_Ptr->SetAccelStructure(vSceneAccelStructure);
	}
}

vk::DescriptorImageInfo* RtxModelShadowModule::GetDescriptorImageInfo(const uint32_t& vBindingPoint, ct::fvec2* vOutSize)
{
	if (m_ModelShadow_Rtx_Pass_Ptr)
	{
		return m_ModelShadow_Rtx_Pass_Ptr->GetDescriptorImageInfo(vBindingPoint, vOutSize);
	}

	return nullptr;
}

void RtxModelShadowModule::SetLightGroup(SceneLightGroupWeak vSceneLightGroup)
{
	if (m_ModelShadow_Rtx_Pass_Ptr)
	{
		m_ModelShadow_Rtx_Pass_Ptr->SetLightGroup(vSceneLightGroup);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// CONFIGURATION /////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::string RtxModelShadowModule::getXml(const std::string& vOffset, const std::string& vUserDatas)
{
	std::string str;

	str += vOffset + "<rtx_model_shadow_module>\n";

	str += vOffset + "\t<can_we_render>" + (m_CanWeRender ? "true" : "false") + "</can_we_render>\n";

	if (m_ModelShadow_Rtx_Pass_Ptr)
	{
		str += m_ModelShadow_Rtx_Pass_Ptr->getXml(vOffset + "\t", vUserDatas);
	}

	str += vOffset + "</rtx_model_shadow_module>\n";

	return str;
}

bool RtxModelShadowModule::setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& vUserDatas)
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

	if (strParentName == "rtx_model_shadow_module")
	{
		if (strName == "can_we_render")
			m_CanWeRender = ct::ivariant(strValue).GetB();
	}

	if (m_ModelShadow_Rtx_Pass_Ptr)
	{
		m_ModelShadow_Rtx_Pass_Ptr->setFromXml(vElem, vParent, vUserDatas);
	}

	return true;
}