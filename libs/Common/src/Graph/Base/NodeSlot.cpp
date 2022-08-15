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

#include "NodeSlot.h"

#include <utility>
#include "BaseNode.h"
static const float slotIconSize = 15.0f;

SlotColor::SlotColor()
{
	AddSlotColor("NONE", ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
	AddSlotColor("MESH", ImVec4(0.5f, 0.5f, 0.9f, 1.0f));
	AddSlotColor("MESH_GROUP", ImVec4(0.1f, 0.1f, 0.8f, 1.0f));
	AddSlotColor("LIGHT_GROUP", ImVec4(0.9f, 0.9f, 0.1f, 1.0f));
	AddSlotColor("ENVIRONMENT", ImVec4(0.1f, 0.9f, 0.1f, 1.0f));
	AddSlotColor("MERGED", ImVec4(0.1f, 0.5f, 0.9f, 1.0f));
	AddSlotColor("TEXTURE_2D", ImVec4(0.9f, 0.5f, 0.1f, 1.0f));
	AddSlotColor("TEXTURE_2D_GROUP", ImVec4(0.2f, 0.9f, 0.2f, 1.0f));
	AddSlotColor("TEXTURE_3D", ImVec4(0.9f, 0.8f, 0.3f, 1.0f));
	AddSlotColor("MIXED", ImVec4(0.3f, 0.5f, 0.1f, 1.0f));
	AddSlotColor("WIDGET_BOOLEAN", ImVec4(0.8f, 0.7f, 0.6f, 1.0f));
	AddSlotColor("WIDGET_UINT", ImVec4(0.8f, 0.7f, 0.6f, 1.0f));
	AddSlotColor("WIDGET_INT", ImVec4(0.8f, 0.7f, 0.6f, 1.0f));
	AddSlotColor("WIDGET_FLOAT", ImVec4(0.8f, 0.7f, 0.6f, 1.0f));
	AddSlotColor("DEPTH", ImVec4(0.2f, 0.7f, 0.6f, 1.0f));
}

ImVec4 SlotColor::GetSlotColor(const std::string& vNodeSlotType)
{
	ImVec4 res = ImVec4(0.8f, 0.8f, 0.0f, 1.0f);

	if (m_ColorSlots.find(vNodeSlotType) != m_ColorSlots.end())
	{
		res = m_ColorSlots.at(vNodeSlotType);
	}

	return res;
}

void SlotColor::AddSlotColor(const std::string& vNodeSlotType, const ImVec4& vSlotColor)
{
	m_ColorSlots[vNodeSlotType] = vSlotColor;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//// STATIC //////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

NodeSlotWeak NodeSlot::sSlotGraphOutputMouseLeft; 
ImVec4 NodeSlot::sSlotGraphOutputMouseLeftColor;
NodeSlotWeak NodeSlot::sSlotGraphOutputMouseMiddle;
ImVec4 NodeSlot::sSlotGraphOutputMouseMiddleColor;
NodeSlotWeak NodeSlot::sSlotGraphOutputMouseRight;
ImVec4 NodeSlot::sSlotGraphOutputMouseRightColor;

std::string NodeSlot::sGetStringFromNodeSlotPlaceEnum(const NodeSlot::PlaceEnum& vPlaceEnum)
{
	static std::array<std::string, (uint32_t)NodeSlot::PlaceEnum::Count> NodeSlotPlaceString = {
		"NONE",
		"INPUT",
		"OUTPUT",
	};
	if (vPlaceEnum != NodeSlot::PlaceEnum::Count)
		return NodeSlotPlaceString[(int)vPlaceEnum];
	LogVarDebug("Error, one NodeSlotvNodeSlot::PlaceEnumEnum have no corresponding string, return \"None\"");
	return "NONE";
}

NodeSlot::PlaceEnum NodeSlot::sGetNodeSlotPlaceEnumFromString(const std::string& vNodeSlotPlaceString)
{
	if (vNodeSlotPlaceString == "NONE") return NodeSlot::PlaceEnum::NONE;
	else if (vNodeSlotPlaceString == "INPUT") return NodeSlot::PlaceEnum::INPUT;
	else if (vNodeSlotPlaceString == "OUTPUT") return NodeSlot::PlaceEnum::OUTPUT;
	return NodeSlot::PlaceEnum::NONE;
}

size_t NodeSlot::sGetNewSlotId()
{
	//#define SLOT_ID_OFFSET 100000
	//return SLOT_ID_OFFSET + (++BaseNode::freeNodeId);
	return ++BaseNode::freeNodeId;
}

// static are null when a plugin is loaded
SlotColor* NodeSlot::sGetSlotColors(SlotColor* vCopy, bool vForce)
{
	static SlotColor _SlotColor;
	static SlotColor* _SlotColor_copy = nullptr;
	if (vCopy || vForce)
		_SlotColor_copy = vCopy;
	if (_SlotColor_copy)
		return _SlotColor_copy;
	return &_SlotColor;
}

NodeSlotPtr NodeSlot::Create(NodeSlot vSlot)
{
	auto res = std::make_shared<NodeSlot>(vSlot);
	res->m_This = res;
	return res;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//// NODESLOT CLASS //////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

NodeSlot::NodeSlot()
{
	pinID = sGetNewSlotId();
}

NodeSlot::NodeSlot(std::string vName)
{
	pinID = sGetNewSlotId();
	name = vName;
}

NodeSlot::NodeSlot(std::string vName, std::string vType)
{
	pinID = sGetNewSlotId();
	name = vName;
	slotType = vType;
	color = sGetSlotColors()->GetSlotColor(slotType);
	colorIsSet = true;
	//stamp.typeStamp = ConvertUniformsTypeEnumToString(type);
}

NodeSlot::NodeSlot(std::string vName, std::string vType, bool vHideName)
{
	pinID = sGetNewSlotId();
	name = vName;
	slotType = vType;
	color = sGetSlotColors()->GetSlotColor(slotType);
	colorIsSet = true;
	//stamp.typeStamp = ConvertUniformsTypeEnumToString(type);
	hideName = vHideName;
}

NodeSlot::NodeSlot(std::string vName, std::string vType, bool vHideName, bool vShowWidget)
{
	pinID = sGetNewSlotId();
	name = vName;
	slotType = vType;
	color = sGetSlotColors()->GetSlotColor(slotType);
	colorIsSet = true;
	//stamp.typeStamp = ConvertUniformsTypeEnumToString(type);
	hideName = vHideName;
	showWidget = vShowWidget;
}

NodeSlot::~NodeSlot() = default;

void NodeSlot::Init()
{
	
}

void NodeSlot::Unit()
{
	// ici pas besoin du assert sur le m_This 
	// car NodeSlot peut etre isntancié à l'ancienne en copie local donc sans shared_ptr
	// donc pour gagner du temps on va checker le this, si expiré on va pas plus loins
	if (!m_This.expired())
	{
		if (!parentNode.expired())
		{
			auto parentNodePtr = parentNode.lock();
			if (parentNodePtr)
			{
				auto graph = parentNodePtr->m_ParentNode;
				if (!graph.expired())
				{
					auto graphPtr = graph.lock();
					if (graphPtr)
					{
						graphPtr->DisConnectSlot(m_This);
					}
				}
			}
		}
	}
}

// name : toto, stamp : vec3(vec4) => result : vec3 toto(vec4)
std::string NodeSlot::GetFullStamp()
{
	std::string res;

	if (!name.empty() && !stamp.typeStamp.empty())
	{
		size_t par = stamp.typeStamp.find('(');
		if (par != std::string::npos)
		{
			res = stamp.typeStamp;
			res.insert(par, " " + name);
		}
	}

	return res;
}

void NodeSlot::NotifyConnectionChangeToParent(bool vConnected) // va contacter le parent pour lui dire que ce slot est connecté a un autre
{
	assert(!m_This.expired());
	if (!parentNode.expired())
	{
		auto parentNodePtr = parentNode.lock();
		if (parentNodePtr)
		{
			parentNodePtr->NotifyConnectionChangeOfThisSlot(m_This, vConnected);
		}
	}
}

bool NodeSlot::CanWeConnectToSlot(NodeSlotWeak vSlot)
{
	if (!parentNode.expired())
	{
		auto parentNodePtr = parentNode.lock();
		if (parentNodePtr)
		{
			assert(!m_This.expired());
			return parentNodePtr->CanWeConnectSlots(m_This, vSlot);
		}
	}

	return false;
}

std::vector<NodeSlotWeak> NodeSlot::InjectTypeInSlot(uType::uTypeEnum vType)
{
	std::vector<NodeSlotWeak> res;

	if (!parentNode.expired())
	{
		auto parentNodePtr = parentNode.lock();
		if (parentNodePtr)
		{
			assert(!m_This.expired());
			res = parentNodePtr->InjectTypeInSlot(m_This, vType);
		}
	}

	return res;
}

void NodeSlot::DrawContent(BaseNodeState *vBaseNodeState)
{
	if (vBaseNodeState && !hidden)
	{
		if (slotPlace == NodeSlot::PlaceEnum::INPUT)
		{
			nd::BeginPin(pinID, nd::PinKind::Input);
			{
				ImGui::BeginHorizontal(pinID.AsPointer());
				
				nd::PinPivotAlignment(ImVec2(0.0f, 0.5f));
				nd::PinPivotSize(ImVec2(0, 0));
				
				DrawSlot(vBaseNodeState, ImVec2(slotIconSize, slotIconSize));

				if (showWidget)
				{
					DrawInputWidget(vBaseNodeState);
				}
				if (!hideName)
				{
					ImGui::TextUnformatted(name.c_str());
				}

				ImGui::Spring(1);

				ImGui::EndHorizontal();
			}
			nd::EndPin();
		}
		else if (slotPlace == NodeSlot::PlaceEnum::OUTPUT)
		{
			nd::BeginPin(pinID, nd::PinKind::Output);
			{
				ImGui::BeginHorizontal(pinID.AsPointer());

				ImGui::Spring(1);

				if (!hideName)
				{
					ImGui::TextUnformatted(name.c_str());
				}
				if (showWidget)
				{
					DrawOutputWidget(vBaseNodeState);
				}

				nd::PinPivotAlignment(ImVec2(0.0f, 0.5f));
				nd::PinPivotSize(ImVec2(0, 0));

				DrawSlot(vBaseNodeState, ImVec2(slotIconSize, slotIconSize));
				
				ImGui::EndHorizontal();
			}
			nd::EndPin();
		}
	}
}

void NodeSlot::DrawSlot(BaseNodeState *vBaseNodeState, ImVec2 vSlotSize, ImVec2 vSlotOffset)
{
	if (vBaseNodeState)
	{
		ImGui::Dummy(vSlotSize);

		ImRect slotRect = GImGui->LastItemData.Rect;
		slotRect.Min += vSlotOffset;
		slotRect.Max += vSlotOffset;

		ImVec2 slotCenter = slotRect.GetCenter();
		pos = slotCenter;

		nd::PinPivotRect(slotCenter, slotCenter);
		nd::PinRect(slotRect.Min, slotRect.Max);

		highLighted = false;

		if (ImGui::IsRectVisible(vSlotSize))
		{
			if (!colorIsSet)
			{
				color = sGetSlotColors()->GetSlotColor(slotType);
				colorIsSet = true;
			}

			auto u_color = ImGui::GetColorU32(color);
			
			auto draw_list = ImGui::GetWindowDrawList();
			if (draw_list)
			{
				DrawNodeSlot(draw_list, slotCenter, vBaseNodeState, connected, u_color, u_color);
			}

			if (ImGui::IsItemHovered())
			{
				highLighted = true;

				if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					MouseDoubleClickedOnSlot(ImGuiMouseButton_Left);
					BaseNode::SelectSlot_Callback(m_This, ImGuiMouseButton_Left);
				}
				else if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Right))
				{
					MouseDoubleClickedOnSlot(ImGuiMouseButton_Right);
					BaseNode::SelectSlot_Callback(m_This, ImGuiMouseButton_Right);
				}
				else if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Middle))
				{
					MouseDoubleClickedOnSlot(ImGuiMouseButton_Middle);
					BaseNode::SelectSlot_Callback(m_This, ImGuiMouseButton_Middle);
				}

				DrawSlotText(draw_list, slotCenter, vBaseNodeState, connected, u_color, u_color);
			}
		}
	}
}

bool NodeSlot::IsAnInput()
{
	return slotPlace == NodeSlot::PlaceEnum::INPUT;
}

bool NodeSlot::IsAnOutput()
{
	return slotPlace == NodeSlot::PlaceEnum::OUTPUT;
}

void NodeSlot::Notify(const NotifyEvent& vEvent, const NodeSlotWeak& vEmitterSlot, const NodeSlotWeak& /*vReceiverSlot*/)
{
	// one notification can be :
	// - from input to output : Front
	// - from output to input : Back
	// Front or Back will not been used in the same way

	if (vEmitterSlot.expired() || vEmitterSlot.getValidShared() == m_This.getValidShared()) // Front or Back
	{
		// we propagate the notification to the connected slots
		for (const auto& otherSlot : linkedSlots)
		{
			auto otherSlotPtr = otherSlot.getValidShared();
			if (otherSlotPtr)
			{
				otherSlotPtr->Notify(vEvent, m_This, otherSlot);
			}
		}

		// also to the LMR selected output slots

		// Left
		if (vEmitterSlot.getValidShared() == NodeSlot::sSlotGraphOutputMouseLeft.getValidShared())
		{
			BaseNode::SelectForGraphOutput_Callback(NodeSlot::sSlotGraphOutputMouseLeft, ImGuiMouseButton_Left);
		}

		// Middle
		if (vEmitterSlot.getValidShared() == NodeSlot::sSlotGraphOutputMouseMiddle.getValidShared())
		{
			BaseNode::SelectForGraphOutput_Callback(NodeSlot::sSlotGraphOutputMouseLeft, ImGuiMouseButton_Middle);
		}

		// Right
		if (vEmitterSlot.getValidShared() == NodeSlot::sSlotGraphOutputMouseRight.getValidShared())
		{
			BaseNode::SelectForGraphOutput_Callback(NodeSlot::sSlotGraphOutputMouseLeft, ImGuiMouseButton_Right);
		}
	}
	else // receiving notification from other slots
	{
		// we treat the notification in herited slots
		TreatNotification(vEvent, vEmitterSlot, m_This);

		auto parentPtr = parentNode.getValidShared();
		if (parentPtr)
		{
			// we treat the notification in parent node
			parentPtr->TreatNotification(vEvent, vEmitterSlot, m_This);
		}

		if (IsAnInput()) // Front
		{
			// front propagate some particular global events
			if (vEvent == GraphIsLoaded ||
				vEvent == NewFrameAvailable ||
				vEvent == SomeTasksWasUpdated)
			{
				parentPtr->PropagateFrontNotification(vEvent);
			}
		}
		else if (IsAnOutput()) // Back
		{
			// back propagate some particular global events
			if (vEvent == GraphIsLoaded ||
				vEvent == NewFrameAvailable ||
				vEvent == SomeTasksWasUpdated)
			{
				parentPtr->PropagateBackNotification(vEvent);
			}
		}
	}
}

void NodeSlot::SendNotification(const std::string& vSlotType, const NotifyEvent& vEvent)
{
	auto nodePtr = parentNode.getValidShared();
	if (nodePtr)
	{
		nodePtr->SendFrontNotification(vSlotType, vEvent);
	}
}

void NodeSlot::Connect(NodeSlotWeak /*vOtherSlot*/)
{
#ifdef _DEBUG
	LogVarInfo("NodeSlot::Connect catched by the slot \"%s\", some class not implement it. maybe its wanted", name.c_str());
#endif
}

void NodeSlot::DisConnect(NodeSlotWeak /*vOtherSlot*/)
{
#ifdef _DEBUG
	LogVarInfo("NodeSlot::DisConnect catched by the slot \"%s\", some class not implement it. maybe its wanted", name.c_str());
#endif
}

void NodeSlot::TreatNotification(
	const NotifyEvent& /*vEvent*/,
	const NodeSlotWeak& /*vEmitterSlot*/,
	const NodeSlotWeak& /*vReceiverSlot*/)
{
#ifdef _DEBUG
	LogVarInfo("NodeSlot::TreatNotification catched by the slot \"%s\", some class not implement it. maybe its wanted", name.c_str());
#endif
}

void NodeSlot::SendFrontNotification(const NotifyEvent& /*vEvent*/)
{
#ifdef _DEBUG
	LogVarInfo("NodeSlot::SendFrontNotification catched by the slot \"%s\", some class not implement it. maybe its wanted", name.c_str());
#endif
}

void NodeSlot::SendBackNotification(const NotifyEvent& /*vEvent*/)
{
#ifdef _DEBUG
	LogVarInfo("NodeSlot::SendBackNotification catched by the slot \"%s\", some class not implement it. maybe its wanted", name.c_str());
#endif
}

void NodeSlot::MouseDoubleClickedOnSlot(const ImGuiMouseButton& /*vMouseButton*/)
{
#ifdef _DEBUG
	LogVarInfo("NodeSlot::MouseDoubleClickedOnSlot catched by the slot \"%s\", some class not implement it. maybe its wanted", name.c_str());
#endif
}

void NodeSlot::DrawDebugInfos()
{
	ImGui::Text("--------------------");
	ImGui::Text("Slot %s", name.c_str());
	ImGui::Text(IsAnInput() ? "Input" : "Output");
	ImGui::Text("Count connections : %u", (uint32_t)linkedSlots.size());
}

//////////////////////////////////////////////////////////////
//// PRIVATE /////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void NodeSlot::DrawInputWidget(BaseNodeState *vBaseNodeState)
{
	if (vBaseNodeState && !parentNode.expired())
	{
		assert(!m_This.expired());
		auto ptr = parentNode.lock();
		if (ptr)
		{
			ptr->DrawInputWidget(vBaseNodeState, m_This);
		}
	}
}

void NodeSlot::DrawOutputWidget(BaseNodeState *vBaseNodeState)
{
	if (vBaseNodeState && !parentNode.expired())
	{
		assert(!m_This.expired());
		auto ptr = parentNode.lock();
		if (ptr)
		{
			ptr->DrawOutputWidget(vBaseNodeState, m_This);
		}
	}
}

void NodeSlot::DrawSlotText(ImDrawList* /*vDrawList*/, ImVec2 /*vCenter*/, BaseNodeState* vBaseNodeState, bool /*vConnected*/, ImU32 /*vColor*/, ImU32 /*vInnerColor*/)
{
	if (vBaseNodeState)
	{
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		if (draw_list)
		{
			std::string slotUType;
			std::string slotName;

			if (slotPlace == NodeSlot::PlaceEnum::INPUT)
			{
				slotName = slotType;

				if (vBaseNodeState->debug_mode)
				{
					slotName = "in " + slotUType + " links(";

					int idx = 0;
					for (auto link : linkedSlots)
					{
						auto linkPtr = link.getValidShared();
						if (linkPtr)
						{
							if (idx > 0)
								slotName += ", ";

							if (!linkPtr->parentNode.expired())
							{
								auto parentNodePtr = linkPtr->parentNode.lock();
								if (parentNodePtr)
								{
									slotName += parentNodePtr->name;
								}
							}
						}

						idx++;
					}

					slotName += ")";
				}
				size_t len = slotName.length();
				if (len > 0)
				{
					const char *beg = slotName.c_str();
					ImVec2 txtSize = ImGui::CalcTextSize(beg);
					ImVec2 min = ImVec2(pos.x - slotIconSize * 0.5f - txtSize.x, pos.y - slotIconSize * 0.5f);
					ImVec2 max = min + ImVec2(txtSize.x, slotIconSize);
					ImGui::RenderFrame(min, max, ImGui::GetColorU32(ImVec4(0.1f, 0.1f, 0.1f, 1.0f)));
					draw_list->AddText(ImVec2(min.x, pos.y - txtSize.y * 0.55f), ImColor(200, 200, 200, 255), beg);
				}
			}
			else if (slotPlace == NodeSlot::PlaceEnum::OUTPUT)
			{
				slotName = slotType;

				if (vBaseNodeState->debug_mode)
				{
					slotName = "links(" + ct::toStr(linkedSlots.size()) + ")" + slotUType + " out";
				}
				size_t len = slotName.length();
				if (len > 0)
				{
					const char *beg = slotName.c_str();
					ImVec2 txtSize = ImGui::CalcTextSize(beg);
					ImVec2 min = ImVec2(pos.x + slotIconSize * 0.5f, pos.y - slotIconSize * 0.5f);
					ImVec2 max = min + ImVec2(txtSize.x, slotIconSize);
					ImGui::RenderFrame(min, max, ImGui::GetColorU32(ImVec4(0.1f, 0.1f, 0.1f, 1.0f)));
					draw_list->AddText(ImVec2(min.x, pos.y - txtSize.y * 0.55f), ImColor(200, 200, 200, 255), beg);
				}
			}
		}
	}
}
void NodeSlot::DrawNodeSlot(ImDrawList *vDrawList, ImVec2 vCenter, BaseNodeState* vBaseNodeState, bool vConnected, ImU32 vColor, ImU32 vInnerColor)
{
	UNUSED(vInnerColor);
	UNUSED(vConnected);

	if (vDrawList)
	{
		const auto slotRadius = vBaseNodeState->graphStyle.SLOT_RADIUS;

		vDrawList->AddNgonFilled(vCenter, slotRadius, vColor, 24);
		
		if (ImGui::IsItemHovered())
		{
			ImVec4 _color = ImGui::ColorConvertU32ToFloat4(vColor); _color.w = 0.5f;
			vDrawList->AddNgon(vCenter, slotRadius + 2.0f, ImGui::GetColorU32(_color), 24, 2.5f);
		}

		auto slotOutputMouseLeftPtr = NodeSlot::sSlotGraphOutputMouseLeft.getValidShared(); // blue
		if (slotOutputMouseLeftPtr && slotOutputMouseLeftPtr == m_This.getValidShared())
		{
			vDrawList->AddNgon(vCenter, slotRadius + 2.0f, ImGui::GetColorU32(NodeSlot::sSlotGraphOutputMouseLeftColor), 24, 2.5f);
		}

		auto slotOutputMouseMiddlePtr = NodeSlot::sSlotGraphOutputMouseMiddle.getValidShared(); // green
		if (slotOutputMouseMiddlePtr && slotOutputMouseMiddlePtr == m_This.getValidShared())
		{
			vDrawList->AddNgon(vCenter, slotRadius + 2.0f, ImGui::GetColorU32(NodeSlot::sSlotGraphOutputMouseMiddleColor), 24, 2.5f);
		}

		auto slotOutputMouseRightPtr = NodeSlot::sSlotGraphOutputMouseRight.getValidShared(); // red
		if (slotOutputMouseRightPtr && slotOutputMouseRightPtr == m_This.getValidShared())
		{
			vDrawList->AddNgon(vCenter, slotRadius + 2.0f, ImGui::GetColorU32(NodeSlot::sSlotGraphOutputMouseRightColor), 24, 2.5f);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////
//// CONFIGURATION ///////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

std::string NodeSlot::getXml(const std::string& vOffset, const std::string& /*vUserDatas*/)
{
	std::string res;

	res += vOffset + ct::toStr("<slot index=\"%u\" name=\"%s\" type=\"%s\" place=\"%s\" id=\"%u\"/>\n",
		index,
		name.c_str(),
		slotType.c_str(),
		sGetStringFromNodeSlotPlaceEnum(slotPlace).c_str(),
		(uint32_t)pinID.Get());

	return res;
}

bool NodeSlot::setFromXml(tinyxml2::XMLElement* vElem, tinyxml2::XMLElement* vParent, const std::string& /*vUserDatas*/)
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

	if (strName == "slot" && strParentName == "node")
	{
		uint32_t _index = 0U;
		std::string _name;
		std::string _type = "NONE";
		NodeSlot::PlaceEnum _place = NodeSlot::PlaceEnum::NONE;
		uint32_t _pinId = 0U;

		for (const tinyxml2::XMLAttribute* attr = vElem->FirstAttribute(); attr != nullptr; attr = attr->Next())
		{
			std::string attName = attr->Name();
			std::string attValue = attr->Value();

			if (attName == "index")
				_index = ct::ivariant(attValue).GetU();
			else if (attName == "name")
				_name = attValue;
			else if (attName == "type")
				_type = attValue;
			else if (attName == "place")
				_place = sGetNodeSlotPlaceEnumFromString(attValue);
			else if (attName == "id")
				_pinId = ct::ivariant(attValue).GetU();
		}

		if (index == _index &&
			slotType == _type && 
			slotPlace == _place && 
			!idAlreadySetbyXml)
		{

			pinID = _pinId;
			idAlreadySetbyXml = true;

			// pour eviter que des slots aient le meme id qu'un nodePtr
			BaseNode::freeNodeId = ct::maxi<uint32_t>(BaseNode::freeNodeId, (uint32_t)pinID.Get()) + 1U;

			return false;
		}
	}	

	return true;
}