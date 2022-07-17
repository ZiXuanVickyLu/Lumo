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

#include "RtxPbrRenderer_Pass.h"

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
#include <vkFramework/VulkanCommandBuffer.h>
#include <utils/Mesh/VertexStruct.h>
#include <Base/FrameBuffer.h>

using namespace vkApi;

//////////////////////////////////////////////////////////////
//// CTOR / DTOR /////////////////////////////////////////////
//////////////////////////////////////////////////////////////

#define RTX_SHADER_PASS_DEBUG_COLOR ct::fvec4(0.6f, 0.2f, 0.9f, 0.5f)

RtxPbrRenderer_Pass::RtxPbrRenderer_Pass(vkApi::VulkanCorePtr vVulkanCorePtr)
	: RtxShaderPass(vVulkanCorePtr)
{
	SetRenderDocDebugName("Rtx Pass : PBR", RTX_SHADER_PASS_DEBUG_COLOR);

	//m_DontUseShaderFilesOnDisk = true;
}

RtxPbrRenderer_Pass::~RtxPbrRenderer_Pass()
{
	Unit();
}

//////////////////////////////////////////////////////////////
//// OVERRIDES ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////

bool RtxPbrRenderer_Pass::DrawWidgets(const uint32_t& vCurrentFrame, ImGuiContext* vContext)
{
	assert(vContext);

	return false;
}

void RtxPbrRenderer_Pass::DrawOverlays(const uint32_t& vCurrentFrame, const ct::frect& vRect, ImGuiContext* vContext)
{
	assert(vContext);

}

void RtxPbrRenderer_Pass::DisplayDialogsAndPopups(const uint32_t& vCurrentFrame, const ct::ivec2& vMaxSize, ImGuiContext* vContext)
{
	assert(vContext);

}

vk::DescriptorImageInfo* RtxPbrRenderer_Pass::GetDescriptorImageInfo(const uint32_t& vBindingPoint, ct::fvec2* vOutSize)
{
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

void RtxPbrRenderer_Pass::SetModel(SceneModelWeak vSceneModel)
{
	ZoneScoped;

	m_SceneModel = vSceneModel;

	NeedNewModelUpdate();
}

void RtxPbrRenderer_Pass::SetLightGroup(SceneLightGroupWeak vSceneLightGroup)
{
	ZoneScoped;

	m_SceneLightGroup = vSceneLightGroup;

	m_SceneLightGroupDescriptorInfoPtr = &m_SceneLightGroupDescriptorInfo;

	auto lightGroupPtr = m_SceneLightGroup.getValidShared();
	if (lightGroupPtr &&
		lightGroupPtr->GetBufferInfo())
	{
		m_SceneLightGroupDescriptorInfoPtr = lightGroupPtr->GetBufferInfo();
	}

	UpdateBufferInfoInRessourceDescriptor();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// CONFIGURATION /////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::string RtxPbrRenderer_Pass::getXml(const std::string& vOffset, const std::string& vUserDatas)
{
	std::string str;

	return str;
}

bool RtxPbrRenderer_Pass::setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& vUserDatas)
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

	if (strParentName == "rtx_pbr_renderer_module")
	{
		

		NeedNewUBOUpload();
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool RtxPbrRenderer_Pass::CanUpdateDescriptors()
{
	if (!m_SceneModel.expired() &&
		m_AccelStructure_Top_Ptr)
	{
		return true;
	}

	return false;
}

bool RtxPbrRenderer_Pass::BuildModel()
{
	m_ModelAdressesBufferInfo = vk::DescriptorBufferInfo { VK_NULL_HANDLE, 0U, VK_WHOLE_SIZE };

	auto modelPtr = m_SceneModel.getValidShared();
	if (modelPtr)
	{
		std::vector<SceneMesh::SceneMeshBuffers> modelBufferAddresses;

		for (auto meshPtr : *modelPtr)
		{
			SceneMesh::SceneMeshBuffers buffer;
			buffer.vertices_address = meshPtr->GetVerticesDeviceAddress();
			buffer.indices_address = meshPtr->GetIndiceDeviceAddress();
			modelBufferAddresses.push_back(buffer);

			CreateBottomLevelAccelerationStructureForMesh(meshPtr);
		}

		vk::BufferUsageFlags bufferUsageFlags = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress;

		auto sizeInBytes = modelPtr->size() * sizeof(SceneMesh::SceneMeshBuffers);
		m_ModelAdressesPtr = VulkanRessource::createStorageBufferObject(
			m_VulkanCorePtr, sizeInBytes,
			bufferUsageFlags, VMA_MEMORY_USAGE_CPU_TO_GPU);
		VulkanRessource::upload(m_VulkanCorePtr, *m_ModelAdressesPtr, modelBufferAddresses.data(), sizeInBytes);

		m_ModelAdressesBufferInfo.buffer = m_ModelAdressesPtr->buffer;
		m_ModelAdressesBufferInfo.offset = 0U;
		m_ModelAdressesBufferInfo.range = sizeInBytes;

		vk::DescriptorBufferInfo m_DescriptorBufferInfo_Vert;
		glm::mat4 m_model_pos = glm::mat4(1.0f);

		std::vector<vk::AccelerationStructureInstanceKHR> blas_instances;
		blas_instances.push_back(CreateBlasInstance(0, m_model_pos));

		CreateTopLevelAccelerationStructure(blas_instances);

		return true;
	}
	else
	{
		return true;
	}

	return false;
}

void RtxPbrRenderer_Pass::DestroyModel(const bool& vReleaseDatas)
{
	DestroyBottomLevelAccelerationStructureForMesh();
	DestroyTopLevelAccelerationStructure();
	m_ModelAdressesPtr.reset();
	m_ModelAdressesBufferInfo = vk::DescriptorBufferInfo{ VK_NULL_HANDLE, 0U, VK_WHOLE_SIZE };

}

bool RtxPbrRenderer_Pass::UpdateLayoutBindingInRessourceDescriptor()
{
	ZoneScoped;

	m_LayoutBindings.clear();
	m_LayoutBindings.emplace_back(0U, vk::DescriptorType::eStorageImage, 1, 
		vk::ShaderStageFlagBits::eRaygenKHR); // output
	m_LayoutBindings.emplace_back(1U, vk::DescriptorType::eAccelerationStructureKHR, 1, 
		vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR); // accel struct
	m_LayoutBindings.emplace_back(2U, vk::DescriptorType::eUniformBuffer, 1, 
		vk::ShaderStageFlagBits::eRaygenKHR); // camera
	m_LayoutBindings.emplace_back(3U, vk::DescriptorType::eStorageBuffer, 1,
		vk::ShaderStageFlagBits::eClosestHitKHR); // model device address

	return true;
}

bool RtxPbrRenderer_Pass::UpdateBufferInfoInRessourceDescriptor()
{
	ZoneScoped;

	writeDescriptorSets.clear();
	writeDescriptorSets.emplace_back(m_DescriptorSet, 0U, 0, 1, vk::DescriptorType::eStorageImage,
		m_ComputeBufferPtr->GetFrontDescriptorImageInfo(0U), nullptr); // output
	// The acceleration structure descriptor has to be chained via pNext
	writeDescriptorSets.emplace_back(m_DescriptorSet, 1U, 0, 1, vk::DescriptorType::eAccelerationStructureKHR,
		nullptr, nullptr, nullptr, &m_AccelStructureTopDescriptorInfo); // accel struct
	writeDescriptorSets.emplace_back(m_DescriptorSet, 2U, 0, 1, vk::DescriptorType::eUniformBuffer, 
		nullptr, CommonSystem::Instance()->GetBufferInfo()); // camera
	writeDescriptorSets.emplace_back(m_DescriptorSet, 3U, 0, 1, vk::DescriptorType::eStorageBuffer,
		nullptr, &m_ModelAdressesBufferInfo); // model device address

	if (!m_SceneModel.expired() && m_AccelStructure_Top_Ptr)
	{
		return true; // pas de maj si pas de structure acceleratrice
	}
	
	return false;
}

std::string RtxPbrRenderer_Pass::GetRayGenerationShaderCode(std::string& vOutShaderName)
{
	vOutShaderName = "RtxPbrRenderer_Pass";
	return u8R"(
#version 460
#extension GL_EXT_ray_tracing : enable

layout(binding = 0, rgba32f) uniform writeonly image2D out_color;
layout(binding = 1) uniform accelerationStructureEXT tlas;
)"
+ CommonSystem::GetBufferObjectStructureHeader(2U) +
u8R"(
struct hitPayload
{
	vec4 color;
	vec3 ro;
	vec3 rd;
};

layout(location = 0) rayPayloadEXT hitPayload prd;

vec3 getRayOrigin()
{
	vec3 ro = view[3].xyz + model[3].xyz;
	ro *= mat3(model);
	return -ro;
}

vec3 getRayDirection(vec2 uv)
{
	uv = uv * 2.0 - 1.0;
	vec4 ray_clip = vec4(uv.x, uv.y, -1.0, 0.0);
	vec4 ray_eye = inverse(proj) * ray_clip;
	vec3 rd = normalize(vec3(ray_eye.x, ray_eye.y, -1.0));
	rd *= mat3(view * model);
	return rd;
}

void main()
{
	const vec2 p = vec2(gl_LaunchIDEXT.xy);
	const vec2 s = vec2(gl_LaunchSizeEXT.xy);

	const vec2 pc = p + 0.5; // pixel center
	const vec2 uv = pc / s;
	const vec2 uvc = uv * 2.0 - 1.0;
	
	mat4 imv = inverse(model * view);
	mat4 ip = inverse(proj);

	vec4 origin    = imv * vec4(0, 0, 0, 1);
	vec4 target    = ip * vec4(uvc.x, uvc.y, 1, 1);
	vec4 direction = imv * vec4(normalize(target.xyz), 0);

	origin = getRayOrigin();
	direction = getRayDirection(uv);

	float tmin = 0.001;
	float tmax = 1e32;

	prd.ro = origin.xyz;
	prd.rd = direction.xyz;
	prd.color = vec4(0.0);

	traceRayEXT(tlas, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, prd.rayOrigin, tmin, prd.rayDir, tmax, 0);
	
	imageStore(out_color, ivec2(gl_LaunchIDEXT.xy), prd.color);
}
)";
}

std::string RtxPbrRenderer_Pass::GetRayIntersectionShaderCode(std::string& vOutShaderName)
{
	vOutShaderName = "RtxPbrRenderer_Pass";
	return u8R"()";
}

std::string RtxPbrRenderer_Pass::GetRayMissShaderCode(std::string& vOutShaderName)
{
	vOutShaderName = "RtxPbrRenderer_Pass";
	return u8R"(
#version 460
#extension GL_EXT_ray_tracing : enable

struct hitPayload
{
	vec4 color;
	vec3 ro;
	vec3 rd;
};

layout(location = 0) rayPayloadInEXT hitPayload prd;

void main()
{
	prd.color = vec4(0.0);
}
)";
}

std::string RtxPbrRenderer_Pass::GetRayAnyHitShaderCode(std::string& vOutShaderName)
{
	vOutShaderName = "RtxPbrRenderer_Pass";
	return u8R"()";
}

std::string RtxPbrRenderer_Pass::GetRayClosestHitShaderCode(std::string& vOutShaderName)
{
	vOutShaderName = "RtxPbrRenderer_Pass";
	return u8R"(
#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_nonuniform_qualifier : enable

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

struct hitPayload
{
	vec4 color;
	vec3 ro;
	vec3 rd;
};

layout(location = 0) rayPayloadInEXT hitPayload prd;

hitAttributeEXT vec3 attribs;

struct V3N3T3B3T2C4 
{
	float px, py, pz;
	float nx, ny, nz;
	float tax, tay, taz;
	float btax, btay, btaz;
	float tx, ty;
	float cx, cy, cz, cw;
};

layout(buffer_reference, , scalar) readonly buffer Vertices
{
	V3N3T3B3T2C4 vdatas[];
};

layout(buffer_reference, scalar) readonly buffer Indices
{
	uint idatas[];
};

layout(binding = 1) uniform accelerationStructureEXT tlas; // same as raygen shader

struct ModelAddresses
{
	uint64_t vertices;
	uint64_t indices;
};

layout(binding = 3) buffer ModelAddresses 
{ 
	SceneMeshBuffers datas[]; 
} sceneMeshBuffers;

void main()
{
	// When contructing the TLAS, we stored the model id in InstanceCustomIndexEXT, so the
	// the instance can quickly have access to the data

	// Object data
	SceneMeshBuffers meshRes = sceneMeshBuffers.datas[gl_InstanceCustomIndexEXT];
	Indices indices = Indices(objResource.indices);
	Vertices vertices = Vertices(objResource.vertices);

	// Indices of the triangle
	uint ind = indices.idatas[gl_PrimitiveID];

	// Vertex of the triangle
	Vertex v0 = vertices.vdatas[ind * 3 + 0];
	Vertex v1 = vertices.vdatas[ind * 3 + 1];
	Vertex v2 = vertices.vdatas[ind * 3 + 2];

	// Barycentric coordinates of the triangle
	const vec3 barycentrics = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);

	vec3 pos = v0.px * barycentrics.x + v1.py * barycentrics.y + v2.pz * barycentrics.z;
	vec3 normal = v0.nx * barycentrics.x + v1.ny * barycentrics.y + v2.nz * barycentrics.z;
    
	// Transforming the normal to world space
	normal = normalize(vec3(normal * gl_WorldToObjectEXT)); 

	//prd.color = vec4(pos, 1.0); // return pos
	//prd.color = vec4(normal * 0.5 + 0.5, 1.0); // return normal
	prd.color = vec4(0.5, 0.2, 0.8, 1.0); // return simple color for hit
}
)";
}