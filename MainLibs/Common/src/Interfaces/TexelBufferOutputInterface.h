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

#pragma once

#include <map>
#include <Graph/Graph.h>
#include <vulkan/vulkan.hpp>
#include <vkFramework/vkFramework.h>

//todo : split the interface in one input interface and one output interface
// can clarify some code who jsut need output and not input 
class TexelBufferOutputInterface
{
public:
	virtual vk::Buffer* GetTexelBuffer(const uint32_t& vBindingPoint, ct::uvec2* vOutSize = nullptr) = 0;
	virtual vk::BufferView* GetTexelBufferView(const uint32_t& vBindingPoint, ct::uvec2* vOutSize = nullptr) = 0;
};