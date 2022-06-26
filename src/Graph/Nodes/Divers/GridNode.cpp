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

#include "GridNode.h"

#include <Modules/Divers/GridModule.h>

std::shared_ptr<GridNode> GridNode::Create(vkApi::VulkanCore* vVulkanCore)
{
	auto res = std::make_shared<GridNode>();
	res->m_This = res;
	if (!res->Init(vVulkanCore))
	{
		res.reset();
	}
	return res;
}

GridNode::GridNode() : BaseNode()
{
	m_NodeType = NodeTypeEnum::GRID_AXIS;
}

GridNode::~GridNode()
{

}

bool GridNode::Init(vkApi::VulkanCore* vVulkanCore)
{
	name = "Grid / Axis";

	NodeSlot slot;
	slot.slotType = NodeSlotTypeEnum::TEXTURE_2D;
	slot.name = "Output";
	AddOutput(slot, true, true);

	m_GridModulePtr = GridModule::Create(vVulkanCore);
	if (m_GridModulePtr)
	{
		return true;
	}
	
	return false;
}

void GridNode::Unit()
{
	m_GridModulePtr.reset();
}

bool GridNode::Execute(const uint32_t& vCurrentFrame, vk::CommandBuffer *vCmd)
{
	if (m_GridModulePtr)
	{
		return m_GridModulePtr->Execute(vCurrentFrame, vCmd);
	}
	return false;
}

bool GridNode::DrawWidgets(const uint32_t& vCurrentFrame, ImGuiContext* vContext)
{
	if (m_GridModulePtr)
	{
		return m_GridModulePtr->DrawWidgets(vCurrentFrame, vContext);
	}

	return false;
}

void GridNode::DisplayDialogsAndPopups(const uint32_t& vCurrentFrame, const ct::ivec2& vMaxSize, ImGuiContext* vContext)
{

}

void GridNode::DisplayInfosOnTopOfTheNode(BaseNodeStateStruct* vCanvasState)
{
	if (vCanvasState && vCanvasState->debug_mode)
	{
		auto drawList = nd::GetNodeBackgroundDrawList(nodeID);
		if (drawList)
		{
			char debugBuffer[255] = "\0";
			snprintf(debugBuffer, 254,
				"Used(%s)\nCell(%i, %i)"/*\nPos(%.1f, %.1f)\nSize(%.1f, %.1f)*/,
				(used ? "true" : "false"), cell.x, cell.y/*, pos.x, pos.y, size.x, size.y*/);
			ImVec2 txtSize = ImGui::CalcTextSize(debugBuffer);
			drawList->AddText(pos - ImVec2(0, txtSize.y), ImGui::GetColorU32(ImGuiCol_Text), debugBuffer);
		}
	}
}

void GridNode::NeedResize(ct::ivec2* vNewSize, const uint32_t* vCountColorBuffer)
{
	if (m_GridModulePtr)
	{
		m_GridModulePtr->NeedResize(vNewSize, vCountColorBuffer);
	}

	// on fait ca apres
	BaseNode::NeedResize(vNewSize, vCountColorBuffer);
}

vk::DescriptorImageInfo* GridNode::GetDescriptorImageInfo(const uint32_t& vBindingPoint)
{
	if (m_GridModulePtr)
	{
		return m_GridModulePtr->GetDescriptorImageInfo(vBindingPoint);
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//// CONFIGURATION ///////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

std::string GridNode::getXml(const std::string& vOffset, const std::string& vUserDatas)
{
	std::string res;

	if (!m_ChildNodes.empty())
	{
		res += BaseNode::getXml(vOffset, vUserDatas);
	}
	else
	{
		res += vOffset + ct::toStr("<node name=\"%s\" type=\"%s\" pos=\"%s\" id=\"%u\">\n",
			name.c_str(),
			Graph::GetStringFromNodeTypeEnum(m_NodeType).c_str(),
			ct::fvec2(pos.x, pos.y).string().c_str(),
			(uint32_t)nodeID.Get());

		for (auto slot : m_Inputs)
		{
			res += slot.second->getXml(vOffset + "\t", vUserDatas);
		}

		for (auto slot : m_Outputs)
		{
			res += slot.second->getXml(vOffset + "\t", vUserDatas);
		}

		if (m_GridModulePtr)
		{
			res += m_GridModulePtr->getXml(vOffset + "\t", vUserDatas);
		}

		res += vOffset + "</node>\n";
	}

	return res;
}

bool GridNode::setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& vUserDatas)
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

	BaseNode::setFromXml(vElem, vParent, vUserDatas);

	if (m_GridModulePtr)
	{
		m_GridModulePtr->setFromXml(vElem, vParent, vUserDatas);
	}

	return true;
}