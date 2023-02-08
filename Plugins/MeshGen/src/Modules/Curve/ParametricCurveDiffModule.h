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


#pragma once

#include <set>
#include <array>
#include <string>
#include <memory>

#include <Headers/Globals.h>

#include <ctools/cTools.h>
#include <ctools/ConfigAbstract.h>

#include <Base/BaseRenderer.h>
#include <Base/QuadShaderPass.h>

#include <vulkan/vulkan.hpp>
#include <vkFramework/Texture2D.h>
#include <vkFramework/VulkanCore.h>
#include <vkFramework/VulkanDevice.h>
#include <vkFramework/vk_mem_alloc.h>
#include <vkFramework/VulkanShader.h>
#include <vkFramework/ImGuiTexture.h>
#include <vkFramework/VulkanRessource.h>
#include <vkFramework/VulkanFrameBuffer.h>

#include <Interfaces/GuiInterface.h>
#include <Interfaces/NodeInterface.h>
#include <Interfaces/TaskInterface.h>
#include <Interfaces/NodeInterface.h>
#include <Interfaces/ResizerInterface.h>

#include <Interfaces/VariableInputInterface.h>
#include <Interfaces/ModelOutputInterface.h>

#include <tinyexpr/tinyexpr.h>

class ParametricCurveDiffModule :
	public NodeInterface,
	public conf::ConfigAbstract,
	public VariableInputInterface<0U>,
	public ModelOutputInterface,
	public GuiInterface
{
private:
	static constexpr size_t s_EXPR_MAX_LEN = 1024;

public:
	static std::shared_ptr<ParametricCurveDiffModule> Create(vkApi::VulkanCorePtr vVulkanCorePtr, BaseNodeWeak vParentNode);

private:
	ct::cWeak<ParametricCurveDiffModule> m_This;
	vkApi::VulkanCorePtr m_VulkanCorePtr = nullptr;
	SceneModelPtr m_SceneModelPtr = nullptr;

private: // curve
	int m_Err_x = 0, m_Err_y = 0, m_Err_z = 0;
	char m_ExprX[s_EXPR_MAX_LEN + 1] = "10.0*(y-x)";
	char m_ExprY[s_EXPR_MAX_LEN + 1] = "28.0*x-y-x*z";
	char m_ExprZ[s_EXPR_MAX_LEN + 1] = "x*y-2.66667*z";
	ct::dvec3 m_StartLocation = 0.001;
	uint32_t m_StepCount = 10000U;
	double m_StepSize = 0.01;
	std::map<std::string, double> m_VarNameValues;
	std::vector<te_variable> m_Vars;
	char m_VarToAddBuffer[s_EXPR_MAX_LEN + 1] = "";
	bool m_CloseCurve = false;
	ct::dvec3 m_CenterPoint;

public:
	ParametricCurveDiffModule(vkApi::VulkanCorePtr vVulkanCorePtr);
	~ParametricCurveDiffModule();

	bool Init();
	void Unit();

	bool DrawWidgets(const uint32_t& vCurrentFrame, ImGuiContext* vContext = nullptr) override;
	void DrawOverlays(const uint32_t& vCurrentFrame, const ct::frect& vRect, ImGuiContext* vContext = nullptr) override;
	void DisplayDialogsAndPopups(const uint32_t& vCurrentFrame, const ct::ivec2& vMaxSize, ImGuiContext* vContext = nullptr) override;

	// Interfaces Setters
	void SetVariable(const uint32_t& vVarIndex, SceneVariableWeak vSceneVariable = SceneVariableWeak()) override;

	// Interfaces Getters
	SceneModelWeak GetModel() override;

	std::string getXml(const std::string& vOffset, const std::string& vUserDatas = "") override;
	bool setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& vUserDatas = "") override;
	void AfterNodeXmlLoading() override;

private:
	void prDrawWidgets();
	void prUpdateMesh();

	void prAddVar(const std::string& vName, const double& vValue);
	void prDelVar(const std::string& vName);

	bool prDrawInputExpr(
		const char* vLabel, 
		const char* vBufferLabel, 
		char* vBuffer, 
		size_t vBufferSize, 
		const int& vError,
		const char* vDdefaultValue);
	bool prDrawVars();
};
