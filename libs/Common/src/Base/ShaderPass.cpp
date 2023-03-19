﻿/*
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

// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "ShaderPass.h"

#include <functional>
#include <ctools/Logger.h>
#include <ctools/FileHelper.h>
#include <ImWidgets/ImWidgets.h>
#include <vkFramework/VulkanSubmitter.h>
#include <glm/gtc/type_ptr.hpp>
#include <Base/FrameBuffer.h>

#define TRACE_MEMORY
#include <vkProfiler/Profiler.h>

using namespace vkApi;

//#define VERBOSE_DEBUG
//#define BLEND_ENABLED

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// PUBLIC / CONSTRUCTOR //////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

ShaderPass::ShaderPass(vkApi::VulkanCorePtr vVulkanCorePtr)
{
	ZoneScoped;

	m_RendererType = GenericType::NONE;
	m_VulkanCorePtr = vVulkanCorePtr;
	m_Device = m_VulkanCorePtr->getDevice();
}

ShaderPass::ShaderPass(vkApi::VulkanCorePtr vVulkanCorePtr, const GenericType& vRendererTypeEnum)
{
	ZoneScoped;

	m_RendererType = vRendererTypeEnum;
	m_VulkanCorePtr = vVulkanCorePtr;
	m_Device = m_VulkanCorePtr->getDevice();
}

ShaderPass::ShaderPass(vkApi::VulkanCorePtr vVulkanCorePtr, vk::CommandPool* vCommandPool, vk::DescriptorPool* vDescriptorPool)
{
	ZoneScoped;

	m_RendererType = GenericType::NONE;
	m_VulkanCorePtr = vVulkanCorePtr;
	m_Device = m_VulkanCorePtr->getDevice();
	m_CommandPool = *vCommandPool;
	m_DescriptorPool = *vDescriptorPool;
}

ShaderPass::ShaderPass(vkApi::VulkanCorePtr vVulkanCorePtr, const GenericType& vRendererTypeEnum, vk::CommandPool* vCommandPool, vk::DescriptorPool* vDescriptorPool)
{
	ZoneScoped;

	m_RendererType = vRendererTypeEnum;
	m_VulkanCorePtr = vVulkanCorePtr;
	m_Device = m_VulkanCorePtr->getDevice();
	m_CommandPool = *vCommandPool;
	m_DescriptorPool = *vDescriptorPool;
}

ShaderPass::~ShaderPass()
{
	ZoneScoped;

	Unit();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// PUBLIC / DURING INIT //////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ShaderPass::ActionBeforeInit()
{

}

void ShaderPass::ActionAfterInitSucceed()
{
	if (IsCompute2DRenderer() && 
		m_ComputeBufferPtr)
	{
		SetDispatchSize2D(m_ComputeBufferPtr->GetOutputSize());
	}
}

void ShaderPass::ActionAfterInitFail()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// PUBLIC / INIT/UNIT ////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool ShaderPass::InitPixelWithoutFBO(
	const ct::uvec2& vSize,
	const uint32_t& vCountColorBuffers,
	vk::RenderPass* vRenderPassPtr,
	const vk::SampleCountFlagBits& vSampleCount)
{
	m_RendererType = GenericType::PIXEL;

	Resize(vSize); // will update m_RenderArea and m_Viewport

	ActionBeforeInit();

	m_Loaded = false;

	m_Device = m_VulkanCorePtr->getDevice();
	m_Queue = m_VulkanCorePtr->getQueue(vk::QueueFlagBits::eGraphics);
	m_DescriptorPool = m_VulkanCorePtr->getDescriptorPool();
	m_CommandPool = m_Queue.cmdPools;

	m_CountColorBuffers = vCountColorBuffers;
	m_SampleCount = vSampleCount;
	m_RenderPassPtr = vRenderPassPtr;

	// ca peut ne pas compiler, masi c'est plus bloquant
	// on va plutot mettre un cadre rouge, avec le message d'erreur au survol
	AddShaderEntryPoints(vk::ShaderStageFlagBits::eVertex, "main");
	AddShaderEntryPoints(vk::ShaderStageFlagBits::eFragment, "main");
	CompilPixel();

	if (BuildModel()) {
		if (CreateSBO()) {
			if (CreateUBO()) {
				if (CreateRessourceDescriptor()) {
					// si ca compile pas
					// c'est pas bon mais on renvoi true car on va afficher 
					// l'erreur dans le node et on pourra le corriger en editant le shader
					CreatePixelPipeline();
					m_Loaded = true;
				}
			}
		}
	}

	if (m_Loaded)
	{
		ActionAfterInitSucceed();
	}
	else
	{
		ActionAfterInitFail();
	}

	return m_Loaded;
}

bool ShaderPass::InitPixel(
	const ct::uvec2& vSize,
	const uint32_t& vCountColorBuffers,
	const bool& vUseDepth,
	const bool& vNeedToClear,
	const ct::fvec4& vClearColor,
	const bool& vMultiPassMode,
	const bool& vTesselated,
	const vk::Format& vFormat,
	const vk::SampleCountFlagBits& vSampleCount)
{
	m_RendererType = GenericType::PIXEL;

	Resize(vSize); // will update m_RenderArea and m_Viewport

	ActionBeforeInit();

	m_Loaded = false;

	m_Device = m_VulkanCorePtr->getDevice();
	m_Queue = m_VulkanCorePtr->getQueue(vk::QueueFlagBits::eGraphics);
	m_DescriptorPool = m_VulkanCorePtr->getDescriptorPool();
	m_CommandPool = m_Queue.cmdPools;

	m_CountColorBuffers = vCountColorBuffers;
	m_SampleCount = vSampleCount;
	m_Tesselated = vTesselated;

	// ca peut ne pas compiler, masi c'est plus bloquant
	// on va plutot mettre un cadre rouge, avec le message d'erreur au survol
	AddShaderEntryPoints(vk::ShaderStageFlagBits::eVertex, "main");
	AddShaderEntryPoints(vk::ShaderStageFlagBits::eFragment, "main");

	if (vTesselated)
	{
		AddShaderEntryPoints(vk::ShaderStageFlagBits::eTessellationControl, "main");
		AddShaderEntryPoints(vk::ShaderStageFlagBits::eTessellationEvaluation, "main");
	}

	CompilPixel();

	m_FrameBufferPtr = FrameBuffer::Create(m_VulkanCorePtr);
	if (m_FrameBufferPtr && m_FrameBufferPtr->Init(
		vSize, vCountColorBuffers, vUseDepth, vNeedToClear,
		vClearColor, vMultiPassMode, vFormat, vSampleCount)) {
		m_RenderPassPtr = m_FrameBufferPtr->GetRenderPass();
		if (BuildModel()) {
			if (CreateSBO()) {
				if (CreateUBO()) {
					if (CreateRessourceDescriptor()) {
						// si ca compile pas
						// c'est pas bon mais on renvoi true car on va afficher 
						// l'erreur dans le node et on pourra le corriger en editant le shader
						CreatePixelPipeline();

						m_OutputSize = m_FrameBufferPtr->GetOutputSize();
						WasJustResized();

						m_Loaded = true;
					}
				}
			}
		}
	}

	if (m_Loaded)
	{
		ActionAfterInitSucceed();
	}
	else
	{
		ActionAfterInitFail();
	}

	return m_Loaded;
}

bool ShaderPass::InitCompute1D(
	const uint32_t& vDispatchSize)
{
	m_RendererType = GenericType::COMPUTE_1D;

	Resize(vDispatchSize); // will update m_RenderArea and m_Viewport

	ActionBeforeInit();

	m_Loaded = false;

	m_Device = m_VulkanCorePtr->getDevice();
	m_Queue = m_VulkanCorePtr->getQueue(vk::QueueFlagBits::eGraphics);
	m_DescriptorPool = m_VulkanCorePtr->getDescriptorPool();
	m_CommandPool = m_Queue.cmdPools;

	// ca peut ne pas compiler, masi c'est plus bloquant
	// on va plutot mettre un cadre rouge, avec le message d'erreur au survol
	AddShaderEntryPoints(vk::ShaderStageFlagBits::eCompute, "main");
	CompilCompute();

	SetDispatchSize1D(vDispatchSize);

	if (BuildModel()) {
		if (CreateSBO()) {
			if (CreateUBO()) {
				if (CreateRessourceDescriptor()) {
					// si ca compile pas
					// c'est pas bon mais on renvoi true car on va afficher 
					// l'erreur dans le node et on pourra le corriger en editant le shader
					CreateComputePipeline();

					WasJustResized();

					m_Loaded = true;
				}
			}
		}
	}

	if (m_Loaded)
	{
		ActionAfterInitSucceed();
	}
	else
	{
		ActionAfterInitFail();
	}

	return m_Loaded;
}

bool ShaderPass::InitCompute2D(
	const ct::uvec2& vDispatchSize,
	const uint32_t& vCountColorBuffers,
	const bool& vMultiPassMode,
	const vk::Format& vFormat)
{
	m_RendererType = GenericType::COMPUTE_2D;

	Resize(vDispatchSize); // will update m_RenderArea and m_Viewport

	ActionBeforeInit();

	m_Loaded = false;

	m_Device = m_VulkanCorePtr->getDevice();
	m_Queue = m_VulkanCorePtr->getQueue(vk::QueueFlagBits::eGraphics);
	m_DescriptorPool = m_VulkanCorePtr->getDescriptorPool();
	m_CommandPool = m_Queue.cmdPools;

	m_CountColorBuffers = vCountColorBuffers;

	// ca peut ne pas compiler, masi c'est plus bloquant
	// on va plutot mettre un cadre rouge, avec le message d'erreur au survol
	AddShaderEntryPoints(vk::ShaderStageFlagBits::eCompute, "main");
	CompilCompute();

	SetDispatchSize2D(vDispatchSize);

	m_ComputeBufferPtr = ComputeBuffer::Create(m_VulkanCorePtr);
	if (m_ComputeBufferPtr && 
		m_ComputeBufferPtr->Init(
		vDispatchSize, vCountColorBuffers, 
		vMultiPassMode, vFormat)) {
		if (BuildModel()) {
			if (CreateSBO()) {
				if (CreateUBO()) {
					if (CreateRessourceDescriptor()) {
						// si ca compile pas
						// c'est pas bon mais on renvoi true car on va afficher 
						// l'erreur dans le node et on pourra le corriger en editant le shader
						CreateComputePipeline();

						m_OutputSize = m_ComputeBufferPtr->GetOutputSize();
						WasJustResized();

						m_Loaded = true;
					}
				}
			}
		}
	}

	if (m_Loaded)
	{
		ActionAfterInitSucceed();
	}
	else
	{
		ActionAfterInitFail();
	}

	return m_Loaded;
}

bool ShaderPass::InitCompute3D(const ct::uvec3& vDispatchSize)
{
	m_RendererType = GenericType::COMPUTE_3D;

	ActionBeforeInit();

	m_Loaded = false;

	m_Device = m_VulkanCorePtr->getDevice();
	m_Queue = m_VulkanCorePtr->getQueue(vk::QueueFlagBits::eGraphics);
	m_DescriptorPool = m_VulkanCorePtr->getDescriptorPool();
	m_CommandPool = m_Queue.cmdPools;

	// ca peut ne pas compiler, masi c'est plus bloquant
	// on va plutot mettre un cadre rouge, avec le message d'erreur au survol
	AddShaderEntryPoints(vk::ShaderStageFlagBits::eCompute, "main");
	CompilCompute();

	SetDispatchSize3D(vDispatchSize);

	if (BuildModel()) {
		if (CreateSBO()) {
			if (CreateUBO()) {
				if (CreateRessourceDescriptor()) {
					// si ca compile pas
					// c'est pas bon mais on renvoi true car on va afficher 
					// l'erreur dans le node et on pourra le corriger en editant le shader
					CreateComputePipeline();
					m_Loaded = true;
				}
			}
		}
	}

	if (m_Loaded)
	{
		ActionAfterInitSucceed();
	}
	else
	{
		ActionAfterInitFail();
	}

	return m_Loaded;
}

bool ShaderPass::InitRtx(
	const ct::uvec2& vDispatchSize,
	const uint32_t& vCountColorBuffers,
	const bool& vMultiPassMode,
	const vk::Format& vFormat)
{
	m_RendererType = GenericType::RTX;

	Resize(vDispatchSize); // will update m_RenderArea and m_Viewport

	ActionBeforeInit();

	m_Loaded = false;

	m_Device = m_VulkanCorePtr->getDevice();
	m_Queue = m_VulkanCorePtr->getQueue(vk::QueueFlagBits::eGraphics);
	m_DescriptorPool = m_VulkanCorePtr->getDescriptorPool();
	m_CommandPool = m_Queue.cmdPools;

	m_CountColorBuffers = vCountColorBuffers;

	// ca peut ne pas compiler, mais c'est plus bloquant
	// on va plutot mettre un cadre rouge, avec le message d'erreur au survol
	//AddShaderEntryPoints(vk::ShaderStageFlagBits::e, "main");
	CompilRtx();

	SetDispatchSize2D(vDispatchSize);

	m_ComputeBufferPtr = ComputeBuffer::Create(m_VulkanCorePtr);
	if (m_ComputeBufferPtr &&
		m_ComputeBufferPtr->Init(
		vDispatchSize, vCountColorBuffers,
		vMultiPassMode, vFormat)) {
		if (BuildModel()) {
			if (CreateSBO()) {
				if (CreateUBO()) {
					// ca peut foirer, mais ici, si on a pas de mode
					// on a pas d'accel struct et ca va planter
					// on finira d'updater les descriptors quand on aura bindé un model
					CreateRessourceDescriptor();
					// si ca compile pas
					// c'est pas bon mais on renvoi true car on va afficher 
					// l'erreur dans le node et on pourra le corriger en editant le shader
					CreateRtxPipeline();
					m_Loaded = true;
					
				}
			}
		}
	}

	if (m_Loaded)
	{
		ActionAfterInitSucceed();
	}
	else
	{
		ActionAfterInitFail();
	}

	return m_Loaded;
}

void ShaderPass::Unit()
{
	ZoneScoped;

	m_Device.waitIdle();
	DestroyPipeline();
	DestroyRessourceDescriptor();
	DestroyUBO();
	DestroySBO();
	DestroyModel(true);
	m_FrameBufferPtr.reset();
	m_FrameBufferWeak.reset();
	m_ComputeBufferPtr.reset();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// PUBLIC ////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////


void ShaderPass::SetFrameBuffer(FrameBufferWeak vFrameBufferWeak)
{
	m_FrameBufferWeak = vFrameBufferWeak;
}

void ShaderPass::SetRenderDocDebugName(const char* vLabel, ct::fvec4 vColor)
{
#ifdef VULKAN_DEBUG
	m_RenderDocDebugName = vLabel;
	m_RenderDocDebugColor = vColor;
#else
	UNUSED(vLabel);
	UNUSED(vColor);
#endif
}


void ShaderPass::AllowResizeOnResizeEvents(const bool& vResizing)
{
	m_ResizingByResizeEventIsAllowed = vResizing;
}

void ShaderPass::AllowResizeByHandOrByInputs(const bool& vResizing)
{
	m_ResizingByHandIsAllowed = vResizing;
}

void ShaderPass::NeedResizeByResizeEvent(ct::ivec2* vNewSize, const uint32_t* vCountColorBuffers)
{
	if (m_ResizingByResizeEventIsAllowed)
	{
		NeedResize(vNewSize, vCountColorBuffers);
	}
}

bool ShaderPass::NeedResizeByResizeEventIfChanged(ct::ivec2 vNewSize)
{
	// need resize ?
	if (IS_FLOAT_DIFFERENT((float)vNewSize.x, m_OutputSize.x) &&
		IS_FLOAT_DIFFERENT((float)vNewSize.y, m_OutputSize.y))
	{
		// yes !
		NeedResizeByResizeEvent(&vNewSize);

		return true;
	}

	return false;
}

void ShaderPass::NeedResizeByHand(ct::ivec2* vNewSize, const uint32_t* vCountColorBuffers)
{
	if (m_ResizingByHandIsAllowed)
	{
		NeedResize(vNewSize, vCountColorBuffers);
	}
}

bool ShaderPass::NeedResizeByHandIfChanged(ct::ivec2 vNewSize)
{
	// need resize ?
	if (IS_FLOAT_DIFFERENT((float)vNewSize.x, m_OutputSize.x) &&
		IS_FLOAT_DIFFERENT((float)vNewSize.y, m_OutputSize.y))
	{
		// yes !
		NeedResizeByHand(&vNewSize);

		return true;
	}

	return false;
}

bool ShaderPass::ResizeIfNeeded()
{
	ZoneScoped;

	if (m_ResizingByHandIsAllowed ||
		m_ResizingByResizeEventIsAllowed)
	{
		if (m_FrameBufferPtr && 
			m_FrameBufferPtr->ResizeIfNeeded())
		{
			Resize(m_FrameBufferPtr->GetOutputSize());
			return true;
		}
		else if (
			m_ComputeBufferPtr && 
			m_ComputeBufferPtr->ResizeIfNeeded())
		{
			UpdateBufferInfoInRessourceDescriptor();
			Resize(m_ComputeBufferPtr->GetOutputSize());
			return true;
		}
		else
		{
			auto fboPtr = m_FrameBufferWeak.getValidShared();
			if (fboPtr &&
				fboPtr->ResizeIfNeeded())
			{
				Resize(fboPtr->GetOutputSize());
				return true;
			}
		}
	}

	return false;
}

void ShaderPass::WasJustResized()
{

}

void ShaderPass::NeedResize(ct::ivec2* vNewSize, const uint32_t* vCountColorBuffers)
{
	if (m_ResizingByHandIsAllowed ||
		m_ResizingByResizeEventIsAllowed)
	{
		if (m_FrameBufferPtr)
		{
			m_FrameBufferPtr->NeedResize(vNewSize, vCountColorBuffers);
		}
		else if (m_ComputeBufferPtr)
		{
			m_ComputeBufferPtr->NeedResize(vNewSize, vCountColorBuffers);
		}
		else
		{
			auto fboPtr = m_FrameBufferWeak.getValidShared();
			if (fboPtr)
			{
				fboPtr->NeedResize(vNewSize, vCountColorBuffers);
			}
		}
	}
}

void ShaderPass::Resize(const ct::uvec2& vNewSize)
{
	ZoneScoped;

	if (m_ResizingByHandIsAllowed || 
		m_ResizingByResizeEventIsAllowed)
	{
		if (m_FrameBufferPtr || !m_FrameBufferWeak.expired())
		{
			m_RenderArea = vk::Rect2D(vk::Offset2D(), vk::Extent2D(vNewSize.x, vNewSize.y));
			m_Viewport = vk::Viewport(0.0f, 0.0f, static_cast<float>(vNewSize.x), static_cast<float>(vNewSize.y), 0, 1.0f);
		}
		else if (m_ComputeBufferPtr)
		{
			SetDispatchSize2D(vNewSize);
		}

		m_OutputSize = ct::fvec2((float)vNewSize.x, (float)vNewSize.y);
		m_OutputRatio = m_OutputSize.ratioXY<float>();

		WasJustResized();
	}
}

void ShaderPass::SwapMultiPassFrontBackDescriptors()
{
	
}

void ShaderPass::DrawPass(vk::CommandBuffer* vCmdBuffer, const int& vIterationNumber)
{
	if (m_IsShaderCompiled && 
		m_Pipelines[0].m_Pipeline && // one pipeline at least
		m_Pipelines[0].m_PipelineLayout &&
		m_DescriptorWasUpdated && 
		CanRender())
	{
		auto devicePtr = m_VulkanCorePtr->getFrameworkDevice().getValidShared();
		if (devicePtr)
		{
			devicePtr->BeginDebugLabel(vCmdBuffer, m_RenderDocDebugName, m_RenderDocDebugColor);
		}

		if (!m_Tesselated) // tesselated so no other topology than patch_list can be used
		{
			if (m_CanDynamicallyChangePrimitiveTopology)
			{
				vCmdBuffer->setPrimitiveTopologyEXT(m_DynamicPrimitiveTopology);
				if (GetPrimitiveTopologyFamily(m_DynamicPrimitiveTopology) == vk::PrimitiveTopology::eLineList)
				{
					vCmdBuffer->setLineWidth(m_LineWidth.w);
				}
			}
			else if (GetPrimitiveTopologyFamily(m_BasePrimitiveTopology) == vk::PrimitiveTopology::eLineList)
			{
				vCmdBuffer->setLineWidth(m_LineWidth.w);
			}
		}

		if (IsPixelRenderer())
		{
			if (m_FrameBufferPtr && 
				m_FrameBufferPtr->Begin(vCmdBuffer))
			{
				m_FrameBufferPtr->ClearAttachmentsIfNeeded(vCmdBuffer, m_ForceFBOClearing);
				m_ForceFBOClearing = false;

				DrawModel(vCmdBuffer, vIterationNumber);

				m_FrameBufferPtr->End(vCmdBuffer);
			}
			else if (!m_FrameBufferWeak.expired())
			{
				auto fboPtr = m_FrameBufferWeak.getValidShared();
				if (fboPtr)
				{
					if (fboPtr->Begin(vCmdBuffer))
					{
						fboPtr->ClearAttachmentsIfNeeded(vCmdBuffer, m_ForceFBOClearing);
						m_ForceFBOClearing = false;

						DrawModel(vCmdBuffer, vIterationNumber);

						fboPtr->End(vCmdBuffer);
					}
				}
			}
		}
		else if (IsCompute2DRenderer())
		{
			if (m_ComputeBufferPtr &&
				m_ComputeBufferPtr->Begin(vCmdBuffer))
			{
				Compute(vCmdBuffer, vIterationNumber);

				m_ComputeBufferPtr->End(vCmdBuffer);
			}
		}
		else if (IsCompute3DRenderer())
		{
			Compute(vCmdBuffer, vIterationNumber);
		}
		else if (IsRtxRenderer())
		{
			TraceRays(vCmdBuffer, vIterationNumber);
		}

		if (devicePtr)
		{
			devicePtr->EndDebugLabel(vCmdBuffer);
		}
	}
}

// dynamic change is possible only if the next topolgy is int eh same class than
// the original provided to the CreatePipeline
bool ShaderPass::ChangeDynamicPrimitiveTopology(const vk::PrimitiveTopology& vPrimitiveTopology, const bool& vForce)
{
	if (!m_Tesselated && // tesselated so no other topology than patch_list can be used
		m_CanDynamicallyChangePrimitiveTopology)
	{
		// we will check if this is the same familly of m_BasePrimitiveTopology
		// https://vulkan.lunarg.com/doc/view/1.2.182.0/windows/1.2-extensions/vkspec.html#drawing-primitive-topology-class

		if (GetPrimitiveTopologyFamily(vPrimitiveTopology) == GetPrimitiveTopologyFamily(m_BasePrimitiveTopology))
		{
			m_DynamicPrimitiveTopology = vPrimitiveTopology;
			return true;
		}
		else if (vForce)
		{
			m_BasePrimitiveTopology = vPrimitiveTopology;
			m_DynamicPrimitiveTopology = vPrimitiveTopology;
			DestroyPipeline();
			CreatePixelPipeline();
		}
	}

	return false;
}

void ShaderPass::SetDynamicallyChangePrimitiveTopology(const bool& vFlag)
{
	m_CanDynamicallyChangePrimitiveTopology = vFlag;
}

void ShaderPass::SetPrimitveTopology(const vk::PrimitiveTopology& vPrimitiveTopology)
{
	if (!m_Tesselated) // tesselated so no other topology than patch_list can be used
	{
		m_BasePrimitiveTopology = vPrimitiveTopology;
	}
}

vk::PrimitiveTopology ShaderPass::GetPrimitiveTopologyFamily(const vk::PrimitiveTopology& vPrimitiveTopology)
{
	switch (vPrimitiveTopology)
	{
	case vk::PrimitiveTopology::ePointList:
		return vk::PrimitiveTopology::ePointList;
	case vk::PrimitiveTopology::eLineList:
	case vk::PrimitiveTopology::eLineStrip:
	case vk::PrimitiveTopology::eLineListWithAdjacency:
	case vk::PrimitiveTopology::eLineStripWithAdjacency:
		return vk::PrimitiveTopology::eLineList;
	case vk::PrimitiveTopology::eTriangleList:
	case vk::PrimitiveTopology::eTriangleStrip:
	case vk::PrimitiveTopology::eTriangleFan:
	case vk::PrimitiveTopology::eTriangleListWithAdjacency:
	case vk::PrimitiveTopology::eTriangleStripWithAdjacency:
		return vk::PrimitiveTopology::eTriangleList;
	//case vk::PrimitiveTopology::ePatchList:
	//	return vk::PrimitiveTopology::ePatchList;
	}

	return vk::PrimitiveTopology::ePatchList;
}

bool ShaderPass::CanRender()
{
	return true;
}

void ShaderPass::DrawModel(vk::CommandBuffer* vCmdBuffer, const int& vIterationNumber)
{
	UNUSED(vCmdBuffer);
	UNUSED(vIterationNumber);

	CTOOL_DEBUG_BREAK;
}

void ShaderPass::Compute(vk::CommandBuffer* vCmdBuffer, const int& vIterationNumber)
{
	UNUSED(vCmdBuffer);
	UNUSED(vIterationNumber);

	CTOOL_DEBUG_BREAK;
}

void ShaderPass::TraceRays(vk::CommandBuffer* vCmdBuffer, const int& vIterationNumber)
{
	UNUSED(vCmdBuffer);
	UNUSED(vIterationNumber);

	CTOOL_DEBUG_BREAK;
}

bool ShaderPass::BuildModel()
{
	ZoneScoped;

	return true;
}

void ShaderPass::NeedNewModelUpdate()
{
	m_NeedNewModelUpdate = true;
}

void ShaderPass::DestroyModel(const bool& vReleaseDatas)
{
	ZoneScoped;

	if (vReleaseDatas)
	{
		
	}
}

void ShaderPass::UpdateModel(const bool& vLoaded)
{
	ZoneScoped;

	if (m_NeedNewModelUpdate)
	{
		if (vLoaded)
		{
			DestroyModel();
		}

		BuildModel();

		UpdateBufferInfoInRessourceDescriptor();

		m_NeedNewModelUpdate = false;
	}
}

void ShaderPass::EnableTextureUse(const uint32_t& vBindingPoint, const uint32_t& vTextureSLot, float& vTextureUseVar)
{
	if (vBindingPoint == vTextureSLot &&
		vTextureUseVar < 0.5f)
	{
		vTextureUseVar = 1.0f;
		NeedNewUBOUpload();
	}
}

void ShaderPass::DisableTextureUse(const uint32_t& vBindingPoint, const uint32_t& vTextureSLot, float& vTextureUseVar)
{
	if (vBindingPoint == vTextureSLot &&
		vTextureUseVar > 0.5f)
	{
		vTextureUseVar = 0.0f;
		NeedNewUBOUpload();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// PUBLIC / COMPUTE //////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ShaderPass::SetLocalGroupSize(const ct::uvec3& vLocalGroupSize)
{
	ZoneScoped;

	auto max_group = m_VulkanCorePtr->getPhysicalDevice().getProperties().limits.maxComputeWorkGroupSize;
	ct::uvec3 min = 1U;
	ct::uvec3 max = ct::uvec3(max_group[0], max_group[1], max_group[2]);
	m_LocalGroupSize = ct::clamp(vLocalGroupSize, min, max);
}

void ShaderPass::SetDispatchSize1D(const uint32_t& vDispatchSize)
{
	ZoneScoped;

	SetDispatchSize3D(ct::uvec3(vDispatchSize, 1U, 1U));
}

void ShaderPass::SetDispatchSize2D(const ct::uvec2& vDispatchSize)
{
	ZoneScoped;

	SetDispatchSize3D(ct::uvec3(vDispatchSize, 1U));
}

void ShaderPass::SetDispatchSize3D(const ct::uvec3& vDispatchSize)
{
	ZoneScoped;

	m_DispatchSize = vDispatchSize / m_LocalGroupSize;

	auto max_dispatch = m_VulkanCorePtr->getPhysicalDevice().getProperties().limits.maxComputeWorkGroupCount;
	ct::uvec3 min = 1U;
	ct::uvec3 max = ct::uvec3(max_dispatch[0], max_dispatch[1], max_dispatch[2]);
	m_DispatchSize = ct::clamp(m_DispatchSize, min, max);
}

const ct::uvec3& ShaderPass::GetDispatchSize()
{
	ZoneScoped;

	return m_DispatchSize;
}

void ShaderPass::Dispatch(vk::CommandBuffer* vCmdBuffer)
{
	if (vCmdBuffer)
	{
		vCmdBuffer->dispatch(m_DispatchSize.x, m_DispatchSize.y, m_DispatchSize.z);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// PUBLIC / VERTEX ///////////////////////////////////////////////////////////////////////////////////////
//// count point to draw ///////////////////////////////////////////////////////////////////////////////////
//// count instances ///////////////////////////////////////////////////////////////////////////////////////
//// etc.. /////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ShaderPass::SetCountVertexs(const uint32_t& vCountVertexs)
{
	ZoneScoped;

	m_CountVertexs.w = ct::maxi<uint32_t>(1u, vCountVertexs);
}

void ShaderPass::SetCountInstances(const uint32_t& vCountInstances)
{
	ZoneScoped;

	m_CountInstances.w = ct::maxi<uint32_t>(1u, vCountInstances);
}

void ShaderPass::SetCountIterations(const uint32_t& vCountIterations)
{
	ZoneScoped;

	m_CountIterations.w = ct::maxi<uint32_t>(1u, vCountIterations);
}

void ShaderPass::SetLineWidth(const float& vLineWidth)
{
	ZoneScoped;

	m_LineWidth.w = vLineWidth;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// PUBLIC / SHADER ///////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ShaderPass::ClearShaderEntryPoints()
{
	m_ShaderEntryPoints.clear();
}

void ShaderPass::AddShaderEntryPoints(const vk::ShaderStageFlagBits& vShaderId, const ShaderEntryPoint& vEntryPoint)
{
	m_ShaderEntryPoints[vShaderId].emplace(vEntryPoint);

	m_Pipelines.resize(m_ShaderEntryPoints[vShaderId].size());
	m_DescriptorSets.resize(m_ShaderEntryPoints[vShaderId].size());
}

// will set if a shader is used or not
// must be called before the compilation
void ShaderPass::SetShaderUse(const vk::ShaderStageFlagBits& vShaderId, const ShaderEntryPoint& vEntryPoint, const bool& vUsed)
{
	if (m_ShaderCodes.find(vShaderId) != m_ShaderCodes.end())
	{
		if (m_ShaderCodes.at(vShaderId).find(vEntryPoint) != m_ShaderCodes.at(vShaderId).end())
		{
			for (auto& shader : m_ShaderCodes.at(vShaderId).at(vEntryPoint))
			{
				shader.m_Used = vUsed;
			}
		}
	}
}

std::string ShaderPass::GetVertexShaderCode(std::string& vOutShaderName)
{
	vOutShaderName = "ShaderPass_Vertex";
	return m_VertexCode;
}

std::string ShaderPass::GetFragmentShaderCode(std::string& vOutShaderName)
{
	vOutShaderName = "ShaderPass_Fragment";
	return m_FragmentCode;
}

std::string ShaderPass::GetGeometryShaderCode(std::string& vOutShaderName)
{
	vOutShaderName = "ShaderPass_Vertex";
	return m_GeometryCode;
}

std::string ShaderPass::GetTesselationEvaluationShaderCode(std::string& vOutShaderName)
{
	vOutShaderName = "ShaderPass_Tesselation_Evaluation";
	return m_TesselationEvaluationCode;
}

std::string ShaderPass::GetTesselationControlShaderCode(std::string& vOutShaderName)
{
	vOutShaderName = "ShaderPass_Tesselation_Control";
	return m_TesselationControlCode;
}

std::string ShaderPass::GetComputeShaderCode(std::string& vOutShaderName)
{
	vOutShaderName = "ShaderPass_Compute";
	return m_ComputeCode;
}

std::string ShaderPass::GetRayGenerationShaderCode(std::string& vOutShaderName)
{

	vOutShaderName = "ShaderPass_Ray_Generation";
	return m_RayGenerationCode;
}

std::string ShaderPass::GetRayIntersectionShaderCode(std::string& vOutShaderName)
{
	vOutShaderName = "ShaderPass_Ray_Intersection";
	return m_RayIntersectionCode;
}

std::string ShaderPass::GetRayMissShaderCode(std::string& vOutShaderName)
{
	vOutShaderName = "ShaderPass_Ray_Miss";
	return m_RayMissCode;
}

std::string ShaderPass::GetRayAnyHitShaderCode(std::string& vOutShaderName)
{
	vOutShaderName = "ShaderPass_Ray_AnyHit";
	return m_RayAnyHitCode;
}

std::string ShaderPass::GetRayClosestHitShaderCode(std::string& vOutShaderName)
{
	vOutShaderName = "ShaderPass_Ray_ClosestHit";
	return m_RayClosestHitCode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// PRIVATE / RENDERER TYPE ///////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool ShaderPass::IsPixelRenderer()
{
	return (m_RendererType == GenericType::PIXEL);
}

bool ShaderPass::IsCompute2DRenderer()
{
	return (m_RendererType == GenericType::COMPUTE_2D);
}

bool ShaderPass::IsCompute3DRenderer()
{
	return (m_RendererType == GenericType::COMPUTE_3D);
}

bool ShaderPass::IsRtxRenderer()
{
	return (m_RendererType == GenericType::RTX);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// PRIVATE / SPECIFIC UPDATE CODE ////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

ShaderPass::ShaderCode ShaderPass::CompilShaderCode(
	const vk::ShaderStageFlagBits& vShaderType, 
	const std::string& vCode, 
	const std::string& vShaderName,
	const std::string& vEntryPoint)
{
	ShaderCode shaderCode;
	shaderCode.m_ShaderId = vShaderType;
	shaderCode.m_Code = vCode;
	std::string shader_name = vShaderName;
	std::string ext;
	switch (vShaderType)
	{
	case vk::ShaderStageFlagBits::eVertex:
		ext = "vert";
		break;
	case vk::ShaderStageFlagBits::eFragment:
		ext = "frag";
		break;
	case vk::ShaderStageFlagBits::eGeometry:
		ext = "geom";
		break;
	case vk::ShaderStageFlagBits::eTessellationEvaluation:
		ext = "eval";
		break;
	case vk::ShaderStageFlagBits::eTessellationControl:
		ext = "ctrl";
		break;
	case vk::ShaderStageFlagBits::eCompute:
		ext = "comp";
		break;
	case vk::ShaderStageFlagBits::eRaygenKHR:
		ext = "rgen";
		break;
	case vk::ShaderStageFlagBits::eIntersectionKHR:
		ext = "rint";
		break;
	case vk::ShaderStageFlagBits::eMissKHR:
		ext = "miss";
		break;
	case vk::ShaderStageFlagBits::eAnyHitKHR:
		ext = "ahit";
		break;
	case vk::ShaderStageFlagBits::eClosestHitKHR:
		ext = "chit";
		break;
	default:
		break;
	}
	assert(!shader_name.empty());

	shaderCode.m_Used = !shaderCode.m_Code.empty();
	if (shaderCode.m_Used)
	{
		if (!m_DontUseShaderFilesOnDisk)
		{
			shaderCode.m_FilePathName = "shaders/" + shader_name + "." + ext;
			auto shader_path = FileHelper::Instance()->GetAppPath() + "/" + shaderCode.m_FilePathName;
			if (FileHelper::Instance()->IsFileExist(shader_path, true))
			{
				shaderCode.m_Code = FileHelper::Instance()->LoadFileToString(shader_path, true);
			}
			else
			{
				FileHelper::Instance()->SaveStringToFile(shaderCode.m_Code, shader_path);
			}
		}

		if (vkApi::VulkanCore::sVulkanShader)
		{
			shaderCode.m_SPIRV = vkApi::VulkanCore::sVulkanShader->CompileGLSLString(shaderCode.m_Code, ext, shader_name, vEntryPoint);
		}
	}

	return shaderCode;
}

ShaderPass::ShaderCode ShaderPass::CompilShaderCode(
	const vk::ShaderStageFlagBits& vShaderType,
	const std::string& vEntryPoint)
{
	ShaderCode shaderCode;
	shaderCode.m_ShaderId = vShaderType;
	shaderCode.m_EntryPoint = vEntryPoint;
	std::string shader_name;
	std::string ext;
	switch (vShaderType)
	{
	case vk::ShaderStageFlagBits::eVertex:
		shaderCode.m_Code = GetVertexShaderCode(shader_name);
		ext = "vert";
		break;
	case vk::ShaderStageFlagBits::eFragment:
		shaderCode.m_Code = GetFragmentShaderCode(shader_name);
		ext = "frag";
		break;
	case vk::ShaderStageFlagBits::eGeometry:
		shaderCode.m_Code = GetGeometryShaderCode(shader_name);
		ext = "geom";
		break;
	case vk::ShaderStageFlagBits::eTessellationEvaluation:
		shaderCode.m_Code = GetTesselationEvaluationShaderCode(shader_name);
		ext = "eval";
		break;
	case vk::ShaderStageFlagBits::eTessellationControl:
		shaderCode.m_Code = GetTesselationControlShaderCode(shader_name);
		ext = "ctrl";
		break;
	case vk::ShaderStageFlagBits::eCompute:
		shaderCode.m_Code = GetComputeShaderCode(shader_name);
		ext = "comp";
		break;
	case vk::ShaderStageFlagBits::eRaygenKHR:
		shaderCode.m_Code = GetRayGenerationShaderCode(shader_name);
		ext = "rgen";
		break;
	case vk::ShaderStageFlagBits::eIntersectionKHR:
		shaderCode.m_Code = GetRayIntersectionShaderCode(shader_name);
		ext = "rint";
		break;
	case vk::ShaderStageFlagBits::eMissKHR:
		shaderCode.m_Code = GetRayMissShaderCode(shader_name);
		ext = "miss";
		break;
	case vk::ShaderStageFlagBits::eAnyHitKHR:
		shaderCode.m_Code = GetRayAnyHitShaderCode(shader_name);
		ext = "ahit";
		break;
	case vk::ShaderStageFlagBits::eClosestHitKHR:
		shaderCode.m_Code = GetRayClosestHitShaderCode(shader_name);
		ext = "chit";
		break;
	default:
		break;
	}
	assert(!shader_name.empty());

	shaderCode.m_Used = !shaderCode.m_Code.empty();

	if (shaderCode.m_Used)
	{
		if (!m_DontUseShaderFilesOnDisk)
		{
			shaderCode.m_FilePathName = "shaders/" + shader_name + "." + ext;
			auto shader_path = FileHelper::Instance()->GetAppPath() + "/" + shaderCode.m_FilePathName;
			if (FileHelper::Instance()->IsFileExist(shader_path, true))
			{
				shaderCode.m_Code = FileHelper::Instance()->LoadFileToString(shader_path, true);
			}
			else
			{
				FileHelper::Instance()->SaveStringToFile(shaderCode.m_Code, shader_path);
			}
		}

		if (vkApi::VulkanCore::sVulkanShader)
		{
			shaderCode.m_SPIRV = vkApi::VulkanCore::sVulkanShader->CompileGLSLString(shaderCode.m_Code, ext, shader_name, vEntryPoint);
		}
	}

	return shaderCode;
}

ShaderPass::ShaderCode& ShaderPass::AddShaderCode(const ShaderCode& vShaderCode, const ShaderEntryPoint& vEntryPoint)
{
	auto count = m_ShaderCodes[vShaderCode.m_ShaderId][vEntryPoint].size();
	m_ShaderCodes[vShaderCode.m_ShaderId][vEntryPoint].push_back(vShaderCode);
	return m_ShaderCodes.at(vShaderCode.m_ShaderId).at(vEntryPoint).at(count);
}

bool ShaderPass::CompilPixel()
{
	ZoneScoped;

	bool res = false;

	ActionBeforeCompilation();

	m_UsedUniforms.clear();
	m_ShaderCodes.clear();

	m_IsShaderCompiled = true;

	for (const auto& shader : m_ShaderEntryPoints)
	{
		for (const auto& entryPoint : shader.second)
		{
			auto code = AddShaderCode(CompilShaderCode(shader.first, entryPoint), entryPoint);
			if (code.m_Code.empty())
			{
				m_IsShaderCompiled = false;
				break;
			}
		}
	}
	
	if (m_IsShaderCompiled)
	{
		if (vkApi::VulkanCore::sVulkanShader)
		{
			if (!m_Loaded)
			{
				res = true;
			}
			else if (m_IsShaderCompiled)
			{
				m_Device.waitIdle();
				DestroyPipeline();
				DestroyRessourceDescriptor();
				DestroyUBO();
				DestroySBO();
				CreateSBO();
				CreateUBO();
				CreateRessourceDescriptor();
				CreatePixelPipeline();
				res = true;
			}
		}
	}

	ActionAfterCompilation();

	return res;
}

bool ShaderPass::CompilCompute()
{
	ZoneScoped;

	bool res = false;

	ActionBeforeCompilation();

	m_UsedUniforms.clear();
	m_ShaderCodes.clear();

	m_IsShaderCompiled = true;

	for (const auto& shader : m_ShaderEntryPoints)
	{
		for (const auto& entryPoint : shader.second)
		{
			auto code = AddShaderCode(CompilShaderCode(shader.first, entryPoint), entryPoint);
			if (code.m_Code.empty())
			{
				m_IsShaderCompiled = false;
				break;
			}
		}
	}

	if (m_IsShaderCompiled)
	{
		if (vkApi::VulkanCore::sVulkanShader)
		{
			if (!m_Loaded)
			{
				res = true;
			}
			else if (m_IsShaderCompiled)
			{
				m_Device.waitIdle();
				DestroyPipeline();
				DestroyRessourceDescriptor();
				DestroyUBO();
				DestroySBO();
				CreateSBO();
				CreateUBO();
				CreateRessourceDescriptor();
				CreateComputePipeline();
				res = true;
			}
		}
	}

	ActionAfterCompilation();

	return res;
}

bool ShaderPass::CompilRtx()
{
	ZoneScoped;

	bool res = false;

	m_UsedUniforms.clear();
	m_ShaderCodes.clear();
	m_IsShaderCompiled = true;

	ActionBeforeCompilation();

	for (const auto& shaders : m_ShaderCodes)
	{
		for (auto& shaderEntryPoint : shaders.second)
		{
			for (auto& shader : shaderEntryPoint.second)
			{
				if (shader.m_Used &&
					shader.m_SPIRV.empty())
				{
					m_IsShaderCompiled = false;
				}
			}
		}
	}

	if (!m_Loaded)
	{
		res = true;
	}
	else if (m_IsShaderCompiled)
	{
		m_Device.waitIdle();
		DestroyPipeline();
		DestroyRessourceDescriptor();
		DestroyUBO();
		DestroySBO();
		CreateSBO();
		CreateUBO();
		CreateRessourceDescriptor();
		CreateRtxPipeline();
		res = true;
	}

	ActionAfterCompilation();

	return res;
}

bool ShaderPass::ReCompilCode()
{
	bool res = false;

	if (m_RendererType == GenericType::COMPUTE_2D ||
		m_RendererType == GenericType::COMPUTE_3D)
	{
		res = CompilCompute();
	}
	else if (m_RendererType == GenericType::PIXEL)
	{
		res = CompilPixel();
	}
	else if (m_RendererType == GenericType::RTX)
	{
		res = CompilRtx();
	}

	return res;
}

void ShaderPass::ActionBeforeCompilation()
{

}

void ShaderPass::ActionAfterCompilation()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::string ShaderPass::getXml(const std::string& vOffset, const std::string& vUserDatas)
{
	std::string str;

	//str += m_UniformWidgets.getXml(vOffset, vUserDatas);

	str += vOffset + "<resize_by_event>" + (m_ResizingByResizeEventIsAllowed ? "true" : "false") + "</resize_by_event>\n";
	str += vOffset + "<resize_by_hand>" + (m_ResizingByHandIsAllowed ? "true" : "false") + "</resize_by_hand>\n";
	str += vOffset + "<buffer_quality>" + ct::toStr(m_BufferQuality) + "</buffer_quality>\n";

	return str;
}

// return true for continue xml parsing of childs in this node or false for interupt the child exploration
bool ShaderPass::setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& vUserDatas)
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

	//m_UniformWidgets.setFromXml(vElem, vParent, vUserDatas);

	if (strName == "resize_by_event")
		m_ResizingByResizeEventIsAllowed = ct::ivariant(strValue).GetB();
	else if (strName == "resize_by_hand")
		m_ResizingByHandIsAllowed = ct::ivariant(strValue).GetB();
	else if (strName == "buffer_quality")
		m_BufferQuality = ct::fvariant(strValue).GetF();

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// PRIVATE / UBO /////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool ShaderPass::CreateUBO()
{
	ZoneScoped;

	return true;
}

void ShaderPass::NeedNewUBOUpload()
{
	m_NeedNewUBOUpload = true;
}

void ShaderPass::UploadUBO()
{
	ZoneScoped;
}

void ShaderPass::DestroyUBO()
{
	ZoneScoped;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// PRIVATE / SBO /////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool ShaderPass::CreateSBO()
{
	ZoneScoped;

	return true;
}

void ShaderPass::NeedNewSBOUpload()
{
	m_NeedNewSBOUpload = true;
}

void ShaderPass::UploadSBO()
{
	ZoneScoped;

	CTOOL_DEBUG_BREAK;
}

void ShaderPass::DestroySBO()
{
	ZoneScoped;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// PRIVATE / RESSOURCE DESCRIPTORS ///////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool ShaderPass::UpdateRessourceDescriptors()
{
	return UpdateBufferInfoInRessourceDescriptor() && UpdateLayoutBindingInRessourceDescriptor();
}

bool ShaderPass::UpdateBufferInfoInRessourceDescriptor()
{
	ZoneScoped;

	bool res = true;

	for (auto& descriptor : m_DescriptorSets)
	{
		descriptor.m_WriteDescriptorSets.clear();
	}

	return res;
}

bool ShaderPass::UpdateLayoutBindingInRessourceDescriptor()
{
	ZoneScoped;

	bool res = true;

	for (auto& descriptor : m_DescriptorSets)
	{
		descriptor.m_LayoutBindings.clear();
	}

	return res;
}

/////////////////////////////////////////////////////////////////////
//// WRITE DESCRIPTORS //////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

void ShaderPass::ClearWriteDescriptors()
{
	for (uint32_t idx = 0U; idx < m_DescriptorSets.size(); ++idx)
	{
		ClearWriteDescriptors(idx);
	}
}

void ShaderPass::ClearWriteDescriptors(const uint32_t& DescriptorSetIndex)
{
	if (DescriptorSetIndex < (uint32_t)m_DescriptorSets.size())
	{
		m_DescriptorSets[DescriptorSetIndex].m_WriteDescriptorSets.clear();
	}
}

bool ShaderPass::AddOrSetWriteDescriptorImage(
	const uint32_t& vBindingPoint, 
	const vk::DescriptorType& vType, 
	const vk::DescriptorImageInfo* vImageInfo,
	const uint32_t& vCount, 
	const uint32_t& DescriptorSetIndex)
{
	if (DescriptorSetIndex < (uint32_t)m_DescriptorSets.size())
	{
		if (vImageInfo && vImageInfo->imageView)
		{
			bool _needUpdate = false;
			uint32_t indexToUpdate = 0U;
			for (const auto& desc : m_DescriptorSets[DescriptorSetIndex].m_WriteDescriptorSets)
			{
				if (desc.dstBinding == vBindingPoint)
				{
					_needUpdate = true;
					break;
				}
				++indexToUpdate;
			}

			// update
			if (_needUpdate)
			{
				m_DescriptorSets[DescriptorSetIndex].m_WriteDescriptorSets[indexToUpdate] = vk::WriteDescriptorSet(
					m_DescriptorSets[DescriptorSetIndex].m_DescriptorSet, vBindingPoint, 0, vCount,
					vType, vImageInfo);
			}
			else // add
			{
				m_DescriptorSets[DescriptorSetIndex].m_WriteDescriptorSets.emplace_back(
					m_DescriptorSets[DescriptorSetIndex].m_DescriptorSet, vBindingPoint, 0, vCount,
					vType, vImageInfo);
			}
		}

		CheckWriteDescriptors(DescriptorSetIndex);

		return true;
	}

	return false;
}

bool ShaderPass::AddOrSetWriteDescriptorBuffer(
	const uint32_t& vBindingPoint,
	const vk::DescriptorType& vType,
	const vk::DescriptorBufferInfo* vBufferInfo,
	const uint32_t& vCount,
	const uint32_t& DescriptorSetIndex)
{
	if (DescriptorSetIndex < (uint32_t)m_DescriptorSets.size())
	{
		bool _needUpdate = false;
		uint32_t indexToUpdate = 0U;
		for (const auto& desc : m_DescriptorSets[DescriptorSetIndex].m_WriteDescriptorSets)
		{
			if (desc.dstBinding == vBindingPoint)
			{
				_needUpdate = true;
				break;
			}
			++indexToUpdate;
		}

		// update
		if (_needUpdate)
		{
			if (vBufferInfo && vBufferInfo->buffer)
			{
				m_DescriptorSets[DescriptorSetIndex].m_WriteDescriptorSets[indexToUpdate] = vk::WriteDescriptorSet(
					m_DescriptorSets[DescriptorSetIndex].m_DescriptorSet, vBindingPoint, 0, vCount,
					vType, nullptr, vBufferInfo);
			}
			else
			{
				m_DescriptorSets[DescriptorSetIndex].m_WriteDescriptorSets[indexToUpdate] = vk::WriteDescriptorSet(
					m_DescriptorSets[DescriptorSetIndex].m_DescriptorSet, vBindingPoint, 0, vCount,
					vType, nullptr, m_VulkanCorePtr->getEmptyDescriptorBufferInfo());
			}
		}
		else // add
		{
			if (vBufferInfo && vBufferInfo->buffer)
			{
				m_DescriptorSets[DescriptorSetIndex].m_WriteDescriptorSets.emplace_back(
					m_DescriptorSets[DescriptorSetIndex].m_DescriptorSet, vBindingPoint, 0, vCount,
					vType, nullptr, vBufferInfo);
			}
			else
			{
				m_DescriptorSets[DescriptorSetIndex].m_WriteDescriptorSets.emplace_back(
					m_DescriptorSets[DescriptorSetIndex].m_DescriptorSet, vBindingPoint, 0, vCount,
					vType, nullptr, m_VulkanCorePtr->getEmptyDescriptorBufferInfo());
			}
		}

		CheckWriteDescriptors(DescriptorSetIndex);

		return true;
	}

	return false;
}

bool ShaderPass::AddOrSetWriteDescriptorBufferView(
	const uint32_t& vBindingPoint,
	const vk::DescriptorType& vType,
	const vk::BufferView* vBufferView,
	const uint32_t& vCount,
	const uint32_t& DescriptorSetIndex)
{
	if (DescriptorSetIndex < (uint32_t)m_DescriptorSets.size())
	{
		bool _needUpdate = false;
		uint32_t indexToUpdate = 0U;
		for (const auto& desc : m_DescriptorSets[DescriptorSetIndex].m_WriteDescriptorSets)
		{
			if (desc.dstBinding == vBindingPoint)
			{
				_needUpdate = true;
				break;
			}
			++indexToUpdate;
		}

		// update
		if (_needUpdate)
		{
			if (vBufferView)
			{
				m_DescriptorSets[DescriptorSetIndex].m_WriteDescriptorSets[indexToUpdate] = vk::WriteDescriptorSet(
					m_DescriptorSets[DescriptorSetIndex].m_DescriptorSet, vBindingPoint, 0, vCount,
					vType, nullptr, nullptr, vBufferView);
			}
			else
			{
				m_DescriptorSets[DescriptorSetIndex].m_WriteDescriptorSets[indexToUpdate] = vk::WriteDescriptorSet(
					m_DescriptorSets[DescriptorSetIndex].m_DescriptorSet, vBindingPoint, 0, vCount,
					vType, nullptr, nullptr, m_VulkanCorePtr->getEmptyBufferView());
			}
		}
		else // Add
		{
			if (vBufferView)
			{
				m_DescriptorSets[DescriptorSetIndex].m_WriteDescriptorSets.emplace_back(
					m_DescriptorSets[DescriptorSetIndex].m_DescriptorSet, vBindingPoint, 0, vCount,
					vType, nullptr, nullptr, vBufferView);
			}
			else
			{
				m_DescriptorSets[DescriptorSetIndex].m_WriteDescriptorSets.emplace_back(
					m_DescriptorSets[DescriptorSetIndex].m_DescriptorSet, vBindingPoint, 0, vCount,
					vType, nullptr, nullptr, m_VulkanCorePtr->getEmptyBufferView());
			}
		}

		CheckWriteDescriptors(DescriptorSetIndex);

		return true;
	}

	return false;
}

bool ShaderPass::AddOrSetWriteDescriptorNext(
	const uint32_t& vBindingPoint,
	const vk::DescriptorType& vType,
	const void* vNext,
	const uint32_t& vCount,
	const uint32_t& DescriptorSetIndex)
{
	if (DescriptorSetIndex < (uint32_t)m_DescriptorSets.size())
	{
		bool _needUpdate = false;
		uint32_t indexToUpdate = 0U;
		for (const auto& desc : m_DescriptorSets[DescriptorSetIndex].m_WriteDescriptorSets)
		{
			if (desc.dstBinding == vBindingPoint)
			{
				_needUpdate = true;
				break;
			}
			++indexToUpdate;
		}

		// update
		if (_needUpdate)
		{
			if (vNext)
			{
				m_DescriptorSets[DescriptorSetIndex].m_WriteDescriptorSets[indexToUpdate] = vk::WriteDescriptorSet(
					m_DescriptorSets[DescriptorSetIndex].m_DescriptorSet, vBindingPoint, 0, vCount,
					vType, nullptr, nullptr, nullptr, vNext);
			}
			else
			{
				m_DescriptorSets[DescriptorSetIndex].m_WriteDescriptorSets[indexToUpdate] = vk::WriteDescriptorSet(
					m_DescriptorSets[DescriptorSetIndex].m_DescriptorSet, vBindingPoint, 0, vCount,
					vType, nullptr, nullptr, nullptr, nullptr);

				return false;
			}
		}
		else // Add
		{
			if (vNext)
			{
				m_DescriptorSets[DescriptorSetIndex].m_WriteDescriptorSets.emplace_back(
					m_DescriptorSets[DescriptorSetIndex].m_DescriptorSet, vBindingPoint, 0, vCount,
					vType, nullptr, nullptr, nullptr, vNext);
			}
			else
			{
				m_DescriptorSets[DescriptorSetIndex].m_WriteDescriptorSets.emplace_back(
					m_DescriptorSets[DescriptorSetIndex].m_DescriptorSet, vBindingPoint, 0, vCount,
					vType, nullptr, nullptr, nullptr, nullptr);

				return false;
			}
		}

		CheckWriteDescriptors(DescriptorSetIndex);

		return true;
	}

	return false;
}

void ShaderPass::CheckWriteDescriptors(const uint32_t& DescriptorSetIndex)
{
#ifdef _DEBUG
	// en debug, au moment de la creation du node
	// il est interressant de verifier les collisions de binding points
	std::set<uint32_t> vBindingPoints;
	for (const auto& desc : m_DescriptorSets[DescriptorSetIndex].m_WriteDescriptorSets)
	{
		if (vBindingPoints.find(desc.dstBinding) == vBindingPoints.end())
		{
			vBindingPoints.emplace(desc.dstBinding);
		}
		else
		{
			LogVarError("Duplicated bidning point %u found in the WriteDescriptorSet", desc.dstBinding);
			CTOOL_DEBUG_BREAK;
		}
	}
#endif
}

/////////////////////////////////////////////////////////////////////
//// LAYOUT DESCRIPTORS /////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

void ShaderPass::ClearLayoutDescriptors()
{
	for (uint32_t idx = 0U; idx < m_DescriptorSets.size(); ++idx)
	{
		ClearLayoutDescriptors(idx);
	}
}

void ShaderPass::ClearLayoutDescriptors(const uint32_t& DescriptorSetIndex)
{
	if (DescriptorSetIndex < (uint32_t)m_DescriptorSets.size())
	{
		m_DescriptorSets[DescriptorSetIndex].m_LayoutBindings.clear();
	}
}

bool ShaderPass::AddOrSetLayoutDescriptor(
	const uint32_t& vBindingPoint,
	const vk::DescriptorType& vType,
	const vk::ShaderStageFlags& vStage,
	const uint32_t& vCount,
	const uint32_t& DescriptorSetIndex)
{
	if (DescriptorSetIndex < (uint32_t)m_DescriptorSets.size())
	{
		bool _needUpdate = false;
		uint32_t indexToUpdate = 0U;
		for (const auto& desc : m_DescriptorSets[DescriptorSetIndex].m_LayoutBindings)
		{
			if (desc.binding == vBindingPoint)
			{
				_needUpdate = true;
				break;
			}
			++indexToUpdate;
		}

		// update
		if (_needUpdate)
		{
			m_DescriptorSets[DescriptorSetIndex].m_LayoutBindings[indexToUpdate] =
				vk::DescriptorSetLayoutBinding(vBindingPoint, vType, vCount, vStage);
		}
		else // Add
		{
			m_DescriptorSets[DescriptorSetIndex].m_LayoutBindings.emplace_back(
				vBindingPoint, vType, vCount, vStage);
		}

		CheckLayoutBinding(DescriptorSetIndex);

		return true;
	}

	return false;
}

void ShaderPass::CheckLayoutBinding(const uint32_t& DescriptorSetIndex)
{
#ifdef _DEBUG
	// en debug, au moment de la creation du node
	// il est interressant de verifier les collisions de binding points
	std::set<uint32_t> vBindingPoints;
	for (const auto& desc : m_DescriptorSets[DescriptorSetIndex].m_LayoutBindings)
	{
		if (vBindingPoints.find(desc.binding) == vBindingPoints.end())
		{
			vBindingPoints.emplace(desc.binding);
		}
		else
		{
			LogVarError("Duplicated bidning point %u found in the WriteDescriptorSet", desc.binding);
			CTOOL_DEBUG_BREAK;
		}
	}
#endif
}

/////////////////////////////////////////////////////////////////////
//// RESSOURCE DESCRIPTORS //////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

bool ShaderPass::CreateRessourceDescriptor()
{
	ZoneScoped;

	if (UpdateLayoutBindingInRessourceDescriptor())
	{
		for (auto& descriptor : m_DescriptorSets)
		{
			descriptor.m_DescriptorSetLayout =
				m_Device.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo(
					vk::DescriptorSetLayoutCreateFlags(),
					static_cast<uint32_t>(descriptor.m_LayoutBindings.size()),
					descriptor.m_LayoutBindings.data()
				));
			descriptor.m_DescriptorSet =
				m_Device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo(
					m_DescriptorPool, 1, &descriptor.m_DescriptorSetLayout))[0];
		}

		if (UpdateBufferInfoInRessourceDescriptor())
		{
			UpdateRessourceDescriptor();
			return true;
		}
	}

	return false;
}

bool ShaderPass::CanUpdateDescriptors()
{
	return true;
}

void ShaderPass::UpdateRessourceDescriptor()
{
	ZoneScoped;

	m_Device.waitIdle();

	// il s'agit de maj les buffer info
	// et ca doit ce faire en dehors d'un renderpass
	// ce qui peu etre faire dans un renderpass c'est juste de la maj de data,
	// mais pas des switch de buffer infos

	// update samplers outside of the command buffer
	//assert(!m_This.expired());
	/*m_UniformWidgets.UpdateSamplers(m_This);
	if (IsCompute2DRenderer())
	{
		m_UniformWidgets.UpdateImages(m_This);
	}
	m_UniformWidgets.UpdateBuffers(m_This);
	m_UniformWidgets.UpdateBlocks(m_This, true);*/

	UpdateModel(m_Loaded);

	if (m_NeedNewUBOUpload)
	{
		UploadUBO();
		m_NeedNewUBOUpload = false;
	}

	if (m_NeedNewSBOUpload)
	{
		UploadSBO();
		m_NeedNewSBOUpload = false;
	}

	if (m_ComputeBufferPtr &&
		m_ComputeBufferPtr->IsMultiPassMode())
	{
		SwapMultiPassFrontBackDescriptors();
	}

	m_DescriptorWasUpdated = false;
	if (CanUpdateDescriptors())
	{
		// update descriptor
		for (auto& descriptor : m_DescriptorSets)
		{
			m_Device.updateDescriptorSets(descriptor.m_WriteDescriptorSets, nullptr);
		}
		m_DescriptorWasUpdated = true;
	}

	// on le met la avant le rendu plutot qu'apres sinon au reload la 1ere
	// frame vaut 0 et ca peut reset le shader selon ce qu'on a mis dedans
	// par contre on incremente la frame apres le submit
	//m_UniformWidgets.SetFrame(m_Frame);
}

void ShaderPass::DestroyRessourceDescriptor()
{
	ZoneScoped;

	m_Device.waitIdle();

	for (auto& descriptor : m_DescriptorSets)
	{
		if (m_DescriptorPool && descriptor.m_DescriptorSet)
			m_Device.freeDescriptorSets(m_DescriptorPool, descriptor.m_DescriptorSet);
		if (descriptor.m_DescriptorSetLayout)
			m_Device.destroyDescriptorSetLayout(descriptor.m_DescriptorSetLayout);

		descriptor.m_DescriptorSetLayout = vk::DescriptorSetLayout{};
		descriptor.m_DescriptorSet = vk::DescriptorSet{};
	}
	
	m_DescriptorSets[0] = DescriptorSetStruct{};
}

/////////////////////////////////////////////////////////////////////
//// RESSOURCE DESCRIPTORS //////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

void ShaderPass::UpdateShaders(const std::set<std::string>& vFiles)
{
	bool needReCompil = false;

	// tofix : c'est nimp cette focntion, on utilise meme pas vFiles...

	for (const auto& shaderCodes : m_ShaderCodes)
	{
		for (const auto& shaderEntryPoint : shaderCodes.second)
		{
			for (const auto& shader : shaderEntryPoint.second)
			{
				if (shader.m_Used &&
					!shader.m_FilePathName.empty())
				{
					for (auto filePathNameToUpdate : vFiles)
					{
						if (filePathNameToUpdate == shader.m_FilePathName)
						{
							auto shader_path = FileHelper::Instance()->GetAppPath() + "/" + shader.m_FilePathName;
							needReCompil |= FileHelper::Instance()->IsFileExist(shader_path);
						}
					}
				}
			}
		}
	}

	if (needReCompil)
	{
		ReCompilCode();
	}
}

/////////////////////////////////////////////////////////////////////
//// SHADER CODE ////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

void ShaderPass::SetHeaderCode(const std::string& vHeaderCode)
{
	m_HeaderCode = vHeaderCode;
}

void ShaderPass::SetVertexShaderCode(const std::string& vShaderCode)
{
	m_VertexCode = vShaderCode;
}

void ShaderPass::SetFragmentShaderCode(const std::string& vShaderCode)
{
	m_FragmentCode = vShaderCode;
}

void ShaderPass::SetGeometryShaderCode(const std::string& vShaderCode)
{
	m_GeometryCode = vShaderCode;
}

void ShaderPass::SetTesselationEvaluationShaderCode(const std::string& vShaderCode)
{
	m_TesselationEvaluationCode = vShaderCode;
}

void ShaderPass::SetTesselationControlShaderCode(const std::string& vShaderCode)
{
	m_TesselationControlCode = vShaderCode;
}

void ShaderPass::SetComputeShaderCode(const std::string& vShaderCode)
{
	m_ComputeCode = vShaderCode;
}

void ShaderPass::SetRayGenerationShaderCode(const std::string& vShaderCode)
{
	m_RayGenerationCode = vShaderCode;
}

void ShaderPass::SetRayIntersectionShaderCode(const std::string& vShaderCode)
{
	m_RayIntersectionCode = vShaderCode;
}

void ShaderPass::SetRayMissShaderCode(const std::string& vShaderCode)
{
	m_RayMissCode = vShaderCode;
}

void ShaderPass::SetRayAnyHitShaderCode(const std::string& vShaderCode)
{
	m_RayAnyHitCode = vShaderCode;
}

void ShaderPass::SetRayClosestHitShaderCode(const std::string& vShaderCode)
{
	m_RayClosestHitCode = vShaderCode;
}

/////////////////////////////////////////////////////////////////////
//// FBO CLEARING ///////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

void ShaderPass::NeedToClearFBOThisFrame()
{
	m_ForceFBOClearing = true;
}

/////////////////////////////////////////////////////////////////////
//// RESIZING ///////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

bool ShaderPass::DrawResizeWidget()
{
	bool resize = false;

	if (ImGui::CollapsingHeader("Resizing"))
	{
		ImGui::Indent();

		ImGui::CheckBoxBoolDefault("Auto Resize by 3D viewport", &m_ResizingByResizeEventIsAllowed, true);
		ImGui::CheckBoxBoolDefault("Resize by Hand", &m_ResizingByHandIsAllowed, true);

		if (m_ResizingByHandIsAllowed)
		{
			const float aw = ImGui::GetContentRegionAvail().x;

			if (ImGui::SliderFloatDefaultCompact(aw, "Quality (smaller value is better)", &m_BufferQuality, 0.5f, 4.0f, 1.0f, 0.25f))
			{
				m_BufferQuality = ct::clamp(m_BufferQuality, 0.25f, 4.0f);
				resize = true;
			}

			ct::uvec2 size = m_OutputSize;
			resize |= ImGui::SliderUIntDefaultCompact(aw, "Width", &size.x, 1U, 2048U, 512U);
			resize |= ImGui::SliderUIntDefaultCompact(aw, "Height", &size.y, 1U, 2048U, 512U);
			if (resize)
			{
				ct::fvec2 new_quality_size = ct::clamp(ct::fvec2(size) / m_BufferQuality, 1.0f, 4096.0f); // 2048 / 0.5 => 4096
				ct::ivec2 new_size = ct::clamp(ct::ivec2(new_quality_size), 1, 2048);
				NeedResizeByHand(&new_size);
			}
		}

		ImGui::Unindent();
	}

	return resize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// PRIVATE / PUSH CONSTANTS //////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

// to call before create pipeline
void ShaderPass::SetPushConstantRange(const vk::PushConstantRange& vPushConstantRange)
{
	m_Internal_PushConstants = vPushConstantRange;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// PRIVATE / PIPELINE ////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool ShaderPass::CreateComputePipeline()
{
	ZoneScoped;

	if (m_ShaderCodes[vk::ShaderStageFlagBits::eCompute].empty()) return false;
	if (m_ShaderCodes[vk::ShaderStageFlagBits::eCompute]["main"][0].m_SPIRV.empty()) return false;

	std::vector<vk::PushConstantRange> push_constants;
	if (m_Internal_PushConstants.size)
	{
		push_constants.push_back(m_Internal_PushConstants);
	}

	m_Pipelines[0].m_PipelineLayout =
		m_Device.createPipelineLayout(vk::PipelineLayoutCreateInfo(
			vk::PipelineLayoutCreateFlags(),
			1, &m_DescriptorSets[0].m_DescriptorSetLayout,
			(uint32_t)push_constants.size(), push_constants.data()
		));
	
	auto cs = vkApi::VulkanCore::sVulkanShader->CreateShaderModule(
		(VkDevice)m_Device, m_ShaderCodes[vk::ShaderStageFlagBits::eCompute]["main"][0].m_SPIRV);

	m_ShaderCreateInfos = {
	   vk::PipelineShaderStageCreateInfo(
		   vk::PipelineShaderStageCreateFlags(),
		   vk::ShaderStageFlagBits::eCompute,
		   cs, "main"
	   )
	};

	vk::ComputePipelineCreateInfo computePipeInfo = vk::ComputePipelineCreateInfo()
		.setStage(m_ShaderCreateInfos[0]).setLayout(m_Pipelines[0].m_PipelineLayout);
	m_Pipelines[0].m_Pipeline = m_Device.createComputePipeline(nullptr, computePipeInfo).value;

	vkApi::VulkanCore::sVulkanShader->DestroyShaderModule((VkDevice)m_Device, cs);

	return true;
}

void ShaderPass::SetInputStateBeforePipelineCreation()
{
	//for a mesh:
	//VertexStruct::P2_T2::GetInputState(m_InputState);

	//for a vertex
	//m_InputState = auto vertexInputState = vk::PipelineVertexInputStateCreateInfo(
	// vk::PipelineVertexInputStateCreateFlags(), // flags
	//	0, nullptr, // vertexBindingDescriptionCount
	//	0, nullptr // vertexAttributeDescriptions
	//	);
}

bool ShaderPass::CreatePixelPipeline()
{
	ZoneScoped;

	if (!m_RenderPassPtr) return false;
	if (m_ShaderCodes[vk::ShaderStageFlagBits::eVertex]["main"].empty()) return false;
	if (m_ShaderCodes[vk::ShaderStageFlagBits::eVertex]["main"][0].m_SPIRV.empty()) return false;
	if (m_ShaderCodes[vk::ShaderStageFlagBits::eFragment]["main"].empty()) return false;
	if (m_ShaderCodes[vk::ShaderStageFlagBits::eFragment]["main"][0].m_SPIRV.empty()) return false;

	if (m_Tesselated)
	{
		if (m_ShaderCodes[vk::ShaderStageFlagBits::eTessellationControl]["main"].empty()) return false;
		if (m_ShaderCodes[vk::ShaderStageFlagBits::eTessellationControl]["main"][0].m_SPIRV.empty()) return false;
		if (m_ShaderCodes[vk::ShaderStageFlagBits::eTessellationEvaluation]["main"].empty()) return false;
		if (m_ShaderCodes[vk::ShaderStageFlagBits::eTessellationEvaluation]["main"][0].m_SPIRV.empty()) return false;
	}

	std::vector<vk::PushConstantRange> push_constants;
	if (m_Internal_PushConstants.size)
	{
		push_constants.push_back(m_Internal_PushConstants);
	}

	m_Pipelines[0].m_PipelineLayout =
		m_Device.createPipelineLayout(vk::PipelineLayoutCreateInfo(
			vk::PipelineLayoutCreateFlags(),
			1, &m_DescriptorSets[0].m_DescriptorSetLayout,
			(uint32_t)push_constants.size(), push_constants.data()
		));

	auto vs = vkApi::VulkanCore::sVulkanShader->CreateShaderModule(
		(VkDevice)m_Device, m_ShaderCodes[vk::ShaderStageFlagBits::eVertex]["main"][0].m_SPIRV);
	auto fs = vkApi::VulkanCore::sVulkanShader->CreateShaderModule(
		(VkDevice)m_Device, m_ShaderCodes[vk::ShaderStageFlagBits::eFragment]["main"][0].m_SPIRV);

	m_ShaderCreateInfos = {
		vk::PipelineShaderStageCreateInfo(
			vk::PipelineShaderStageCreateFlags(),
			vk::ShaderStageFlagBits::eVertex,
			vs, "main"
		),
		vk::PipelineShaderStageCreateInfo(
			vk::PipelineShaderStageCreateFlags(),
			vk::ShaderStageFlagBits::eFragment,
			fs, "main"
		)
	};

	vk::ShaderModule tc, te;
	if (m_Tesselated)
	{
		tc = vkApi::VulkanCore::sVulkanShader->CreateShaderModule(
			(VkDevice)m_Device, m_ShaderCodes[vk::ShaderStageFlagBits::eTessellationControl]["main"][0].m_SPIRV);
		te = vkApi::VulkanCore::sVulkanShader->CreateShaderModule(
			(VkDevice)m_Device, m_ShaderCodes[vk::ShaderStageFlagBits::eTessellationEvaluation]["main"][0].m_SPIRV);

		m_ShaderCreateInfos.push_back(
			vk::PipelineShaderStageCreateInfo(
				vk::PipelineShaderStageCreateFlags(),
				vk::ShaderStageFlagBits::eTessellationControl,
				tc, "main"
			));

		m_ShaderCreateInfos.push_back(
			vk::PipelineShaderStageCreateInfo(
				vk::PipelineShaderStageCreateFlags(),
				vk::ShaderStageFlagBits::eTessellationEvaluation,
				te, "main"
			));
	}

	// setup fix functions
	if (m_Tesselated)
	{
		m_BasePrimitiveTopology = vk::PrimitiveTopology::ePatchList;
	}

	auto assemblyState = vk::PipelineInputAssemblyStateCreateInfo(
		vk::PipelineInputAssemblyStateCreateFlags(),
		m_BasePrimitiveTopology
	);

	vk::PipelineTessellationStateCreateInfo* tesselationStatePtr = nullptr;
	auto tesselationState = vk::PipelineTessellationStateCreateInfo(
		vk::PipelineTessellationStateCreateFlags(),
		m_PatchControlPoints
	);
	if (m_Tesselated)
	{
		tesselationStatePtr = &tesselationState;
	}

	auto viewportState = vk::PipelineViewportStateCreateInfo(
		vk::PipelineViewportStateCreateFlags(),
		1, &m_Viewport,
		1, &m_RenderArea
	);

	auto rasterState = vk::PipelineRasterizationStateCreateInfo(
		vk::PipelineRasterizationStateCreateFlags(),
		VK_FALSE,
		VK_FALSE,
		m_PolygonMode,
		m_CullMode,
		m_FrontFaceMode,
		VK_FALSE,
		0,
		0,
		0,
		m_LineWidth.w
	);

	auto multisampleState = vk::PipelineMultisampleStateCreateInfo(vk::PipelineMultisampleStateCreateFlags());
	multisampleState.rasterizationSamples = m_SampleCount;
	//multisampleState.sampleShadingEnable = VK_TRUE;
	//multisampleState.minSampleShading = 0.2f;

	m_BlendAttachmentStates.clear();
	vk::PipelineDepthStencilStateCreateInfo depthStencilState;
	vk::PipelineColorBlendStateCreateInfo colorBlendState;
	if (m_BlendingEnabled)
	{
		for (uint32_t i = 0; i < m_CountColorBuffers; ++i)
		{
			m_BlendAttachmentStates.emplace_back(
				VK_TRUE,
				vk::BlendFactor::eOne,
				vk::BlendFactor::eOne,
				vk::BlendOp::eAdd,
				vk::BlendFactor::eSrcAlpha,
				vk::BlendFactor::eDstAlpha,
				vk::BlendOp::eAdd,
				vk::ColorComponentFlags(
					vk::ColorComponentFlagBits::eR |
					vk::ColorComponentFlagBits::eG |
					vk::ColorComponentFlagBits::eB |
					vk::ColorComponentFlagBits::eA));
		}

		colorBlendState = vk::PipelineColorBlendStateCreateInfo(
			vk::PipelineColorBlendStateCreateFlags(),
			VK_FALSE,
			vk::LogicOp::eCopy,
			static_cast<uint32_t>(m_BlendAttachmentStates.size()),
			m_BlendAttachmentStates.data()
		);

		depthStencilState = vk::PipelineDepthStencilStateCreateInfo(
			vk::PipelineDepthStencilStateCreateFlags(),
			VK_FALSE,
			VK_FALSE,
			vk::CompareOp::eAlways
		);
	}
	
	else
	{
		for (uint32_t i = 0; i < m_CountColorBuffers; ++i)
		{
			m_BlendAttachmentStates.emplace_back(
				VK_FALSE,
				vk::BlendFactor::eOne,
				vk::BlendFactor::eZero,
				vk::BlendOp::eAdd,
				vk::BlendFactor::eOne,
				vk::BlendFactor::eZero,
				vk::BlendOp::eAdd,
				vk::ColorComponentFlags(
					vk::ColorComponentFlagBits::eR |
					vk::ColorComponentFlagBits::eG |
					vk::ColorComponentFlagBits::eB |
					vk::ColorComponentFlagBits::eA));
		}

		colorBlendState = vk::PipelineColorBlendStateCreateInfo(
			vk::PipelineColorBlendStateCreateFlags(),
			VK_FALSE,
			vk::LogicOp::eClear,
			static_cast<uint32_t>(m_BlendAttachmentStates.size()),
			m_BlendAttachmentStates.data()
		);

		depthStencilState = vk::PipelineDepthStencilStateCreateInfo(
			vk::PipelineDepthStencilStateCreateFlags(),
			VK_TRUE,
			VK_TRUE,
			vk::CompareOp::eLessOrEqual,
			VK_FALSE,
			VK_FALSE,
			vk::StencilOpState(),
			vk::StencilOpState(),
			0.0f,
			0.0f
		);
	}

	auto dynamicStateList = std::vector<vk::DynamicState>{
		vk::DynamicState::eViewport
		,vk::DynamicState::eScissor
		,vk::DynamicState::eLineWidth
		//,vk::DynamicState::eCullMode
		//,vk::DynamicState::eFrontFace
	};

	if (!m_Tesselated && // tesselated so no other topology than patch_list can be used
		m_CanDynamicallyChangePrimitiveTopology)
	{
		dynamicStateList.push_back(vk::DynamicState::ePrimitiveTopologyEXT);
	}

	auto dynamicState = vk::PipelineDynamicStateCreateInfo(
		vk::PipelineDynamicStateCreateFlags(),
		static_cast<uint32_t>(dynamicStateList.size()),
		dynamicStateList.data()
	);

	SetInputStateBeforePipelineCreation();

	m_Pipelines[0].m_Pipeline = m_Device.createGraphicsPipeline(
		m_PipelineCache,
		vk::GraphicsPipelineCreateInfo(
			vk::PipelineCreateFlags(),
			static_cast<uint32_t>(m_ShaderCreateInfos.size()),
			m_ShaderCreateInfos.data(),
			&m_InputState.state,
			&assemblyState,
			tesselationStatePtr,
			&viewportState,
			&rasterState,
			&multisampleState,
			&depthStencilState,
			&colorBlendState,
			&dynamicState,
			m_Pipelines[0].m_PipelineLayout,
			*m_RenderPassPtr,
			0
		)
	).value;

	vkApi::VulkanCore::sVulkanShader->DestroyShaderModule((VkDevice)m_Device, vs);
	vkApi::VulkanCore::sVulkanShader->DestroyShaderModule((VkDevice)m_Device, fs);

	if (m_Tesselated)
	{
		vkApi::VulkanCore::sVulkanShader->DestroyShaderModule((VkDevice)m_Device, tc);
		vkApi::VulkanCore::sVulkanShader->DestroyShaderModule((VkDevice)m_Device, te);
	}

	return true;
}

bool ShaderPass::CreateRtxPipeline()
{
	CTOOL_DEBUG_BREAK;

	return false;
}

void ShaderPass::DestroyPipeline()
{
	ZoneScoped;

	for (auto& pip : m_Pipelines)
	{
		if (pip.m_Pipeline)
			m_Device.destroyPipeline(pip.m_Pipeline);
		pip.m_Pipeline = vk::Pipeline{};
		if (pip.m_PipelineLayout)
			m_Device.destroyPipelineLayout(pip.m_PipelineLayout);
		pip.m_PipelineLayout = vk::PipelineLayout{};
	}
	if (m_PipelineCache)
		m_Device.destroyPipelineCache(m_PipelineCache);
	m_PipelineCache = vk::PipelineCache {};
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// PRIVATE / RTX /////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
