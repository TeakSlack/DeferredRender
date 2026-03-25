#include <cassert>
#include <fstream>

#include <spdlog/spdlog.h>
#include "pipeline_builder.h"

// Constructor: initializes device, physical device, and queries device properties/features.
PipelineBuilder::PipelineBuilder(PipelineType pipelineType)
	: m_PipelineType(pipelineType)
{
	
}

PipelineBuilder::~PipelineBuilder()
{

}

// Add a shader stage from a SPIR-V file.
PipelineBuilder& PipelineBuilder::add_shader_stage(const std::string& shaderPath, vk::ShaderStageFlagBits shaderStage)
{
	std::ifstream file(shaderPath, std::ifstream::ate | std::ifstream::binary);
	if (!file.is_open())
	{
		spdlog::error("Failed to open file " + shaderPath);
		exit(EXIT_FAILURE);
	}

	size_t fileSize = file.tellg();
	std::vector<char> shaderCode(fileSize);
	file.seekg(0);
	file.read(shaderCode.data(), fileSize);
	file.close();

	ShaderInfo shader = { .shaderCode = std::move(shaderCode), .shaderStage = shaderStage};
	m_ShaderInfo.push_back(shader);

	return *this;
}

// Add a shader stage from an existing shader module.
PipelineBuilder& PipelineBuilder::add_shader_stage(vk::ShaderModule shaderModule, vk::ShaderStageFlagBits shaderStage)
{
	ShaderModule shader = { .shaderModule = shaderModule, .shaderStage = shaderStage };
	m_ShaderModules.push_back(shader);
	return *this;
}

// Set input assembly primitive topology.
PipelineBuilder& PipelineBuilder::set_primitive_topology(vk::PrimitiveTopology topology)
{
	state.inputAssembly.topology = topology;
	return *this;
}

// Enable/disable primitive restart.
PipelineBuilder& PipelineBuilder::set_primitive_restart(bool enable)
{
	state.inputAssembly.primitiveRestartEnable = enable ? vk::True : vk::False;
	return *this;
}

// Enable/disable dynamic primitive topology.
PipelineBuilder& PipelineBuilder::set_dynamic_topology(bool enable)
{
	toggle_dynamic_state(enable, vk::DynamicState::ePrimitiveTopology);
	return *this;
}

// Set number of patch control points for tessellation.
PipelineBuilder& PipelineBuilder::set_patch_control_points(uint32_t points)
{
	state.tessellationState.patchControlPoints = points;
	return *this;
}

// Add a dynamic state.
PipelineBuilder& PipelineBuilder::add_dynamic_state(vk::DynamicState dynamicState)
{
	state.dynamicStates.push_back(dynamicState);
	return *this;
}

// Add a viewport.
PipelineBuilder& PipelineBuilder::add_viewport(vk::Viewport viewport)
{
	state.viewportState.viewports.push_back(viewport);
	return *this;
}

// Add a viewport by parameters.
PipelineBuilder& PipelineBuilder::add_viewport(float x, float y, float width, float height, float minDepth, float maxDepth)
{
	vk::Viewport viewport(x, y, width, height, minDepth, maxDepth);
	state.viewportState.viewports.push_back(viewport);
	return *this;
}

// Add a scissor rectangle.
PipelineBuilder& PipelineBuilder::add_scissor(vk::Rect2D scissor)
{
	state.viewportState.scissors.push_back(scissor);
	return *this;
}

// Add a scissor rectangle by parameters.
PipelineBuilder& PipelineBuilder::add_scissor(int32_t x, int32_t y, uint32_t width, uint32_t height)
{
	vk::Rect2D scissor({ x, y }, { width, height });
	state.viewportState.scissors.push_back(scissor);
	return *this;
}

// Enable/disable depth clamp.
PipelineBuilder& PipelineBuilder::set_depth_clamp(bool enable)
{
	state.rasterizationState.depthClamp = enable ? vk::True : vk::False;
	return *this;
}

// Enable/disable rasterizer discard.
PipelineBuilder& PipelineBuilder::set_rasterizer_discard(bool enable)
{
	state.rasterizationState.rasterizerDiscard = enable ? vk::True : vk::False;
	return *this;
}

// Set polygon mode.
PipelineBuilder& PipelineBuilder::set_polygon_mode(vk::PolygonMode mode)
{
	state.rasterizationState.polygonMode = mode;
	return *this;
}

// Set cull mode.
PipelineBuilder& PipelineBuilder::set_cull_mode(vk::CullModeFlags mode)
{
	state.rasterizationState.cullMode = mode;
	return *this;
}

// Set front face winding order.
PipelineBuilder& PipelineBuilder::set_front_face(vk::FrontFace face)
{
	state.rasterizationState.frontFace = face;
	return *this;
}

// Enable/disable depth bias.
PipelineBuilder& PipelineBuilder::set_depth_bias(bool enable)
{
	state.rasterizationState.depthBias = enable ? vk::True : vk::False;
	return *this;
}

// Set line width.
PipelineBuilder& PipelineBuilder::set_line_width(float width)
{
	state.rasterizationState.lineWidth = width;
	return *this;
}

// Set depth bias constant.
PipelineBuilder& PipelineBuilder::set_depth_bias_constant(float constant)
{
	state.rasterizationState.depthBiasConstant = constant;
	return *this;
}

// Set depth bias clamp.
PipelineBuilder& PipelineBuilder::set_depth_bias_clamp(float clamp)
{
	state.rasterizationState.depthBiasClamp = clamp;
	return *this;
}

// Set depth bias slope.
PipelineBuilder& PipelineBuilder::set_depth_bias_slope(float slope)
{
	state.rasterizationState.depthBiasSlope = slope;
	return *this;
}

// Enable/disable dynamic line width.
PipelineBuilder& PipelineBuilder::set_dynamic_line_width(bool enable)
{
	toggle_dynamic_state(enable, vk::DynamicState::eLineWidth);
	return *this;
}

// Enable/disable dynamic depth bias.
PipelineBuilder& PipelineBuilder::set_dynamic_depth_bias(bool enable)
{
	toggle_dynamic_state(enable, vk::DynamicState::eDepthBias);
	return *this;
}

// Set multisample count.
PipelineBuilder& PipelineBuilder::set_sample_count(vk::SampleCountFlagBits count)
{
	state.multisampleState.rasterizationSamples = count;
	return *this;
}

// Enable/disable sample shading and set min sample shading.
PipelineBuilder& PipelineBuilder::set_sample_shading(bool enable, float minSampleShading)
{
	if (minSampleShading < 0.0f || minSampleShading > 1.0f)
	{
		spdlog::error("minSampleShading must be between 0.0 and 1.0.");
		throw std::runtime_error("Invalid minSampleShading value.");
	}
	state.multisampleState.sampleShadingEnable = enable ? vk::True : vk::False;
	state.multisampleState.minSampleShading = minSampleShading;
	return *this;
}

// Add a sample mask.
PipelineBuilder& PipelineBuilder::add_sample_mask(vk::SampleMask mask)
{
	state.multisampleState.sampleMasks.push_back(mask);
	return *this;
}

// Enable/disable alpha-to-coverage.
PipelineBuilder& PipelineBuilder::set_alpha_to_coverage(bool enable)
{
	state.multisampleState.alphaToCoverageEnable = enable ? vk::True : vk::False;
	return *this;
}

// Enable/disable alpha-to-one.
PipelineBuilder& PipelineBuilder::set_alpha_to_one(bool enable)
{
	state.multisampleState.alphaToOneEnable = enable ? vk::True : vk::False;
	return *this;
}

// Set render pass and subpass index.
PipelineBuilder& PipelineBuilder::set_render_pass(vk::RenderPass& renderPass, uint32_t subpassIndex)
{
	m_RenderPass = renderPass;
	m_SubpassIndex = subpassIndex;
	return *this;
}

PipelineBuilder& PipelineBuilder::set_render_pass(VkRenderPass& renderPass, uint32_t subpassIndex)
{
	m_RenderPass = renderPass;
	m_SubpassIndex = subpassIndex;
	return *this;
}

// Enable/disable depth test.
PipelineBuilder& PipelineBuilder::set_depth_test(bool enable)
{
	state.depthStencilState.depthTestEnable = enable ? vk::True : vk::False;
	return *this;
}

// Enable/disable depth write.
PipelineBuilder& PipelineBuilder::set_depth_write(bool enable)
{
	state.depthStencilState.depthWriteEnable = enable ? vk::True : vk::False;
	return *this;
}

// Set depth compare operation.
PipelineBuilder& PipelineBuilder::set_depth_compare_op(vk::CompareOp compareOp)
{
	state.depthStencilState.depthCompareOp = compareOp;
	return *this;
}

// Enable/disable depth bounds test.
PipelineBuilder& PipelineBuilder::set_depth_bounds_test(bool enable)
{
	state.depthStencilState.depthBoundsTestEnable = enable ? vk::True : vk::False;
	return *this;
}

// Enable/disable stencil test.
PipelineBuilder& PipelineBuilder::set_stencil_test(bool enable)
{
	state.depthStencilState.stencilTestEnable = enable ? vk::True : vk::False;
	return *this;
}

// Set stencil front state.
PipelineBuilder& PipelineBuilder::set_stencil_front(vk::StencilOpState front)
{
	state.depthStencilState.front = front;
	return *this;
}

// Set stencil back state.
PipelineBuilder& PipelineBuilder::set_stencil_back(vk::StencilOpState back)
{
	state.depthStencilState.back = back;
	return *this;
}

// Set minimum depth bounds.
PipelineBuilder& PipelineBuilder::set_min_depth_bounds(float minDepthBounds)
{
	state.depthStencilState.minDepthBounds = minDepthBounds;
	return *this;
}

// Set maximum depth bounds.
PipelineBuilder& PipelineBuilder::set_max_depth_bounds(float maxDepthBounds)
{
	state.depthStencilState.maxDepthBounds = maxDepthBounds;
	return *this;
}

// Enable/disable logic op and set logic op.
PipelineBuilder& PipelineBuilder::set_logic_op(bool enable, vk::LogicOp logicOp)
{
	state.colorBlendState.logicOpEnable = enable ? vk::True : vk::False;
	state.colorBlendState.logicOp = logicOp;
	return *this;
}

// Add a color blend attachment.
PipelineBuilder& PipelineBuilder::add_color_blend_attachment(vk::PipelineColorBlendAttachmentState& attachment)
{
	state.colorBlendState.attachments.push_back(attachment);
	return *this;
}

// Set blend constants.
PipelineBuilder& PipelineBuilder::set_blend_constants(const std::array<float, 4>& constants)
{
	state.colorBlendState.blendConstants = constants;
	return *this;
}

// Enable writing to all color channels in the blend state.
PipelineBuilder& PipelineBuilder::color_blend_write_all()
{
	vk::PipelineColorBlendAttachmentState attachment{};
	attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
	state.colorBlendState.attachments.push_back(attachment);
	return *this;
}


// Add a descriptor set layout.
PipelineBuilder& PipelineBuilder::add_descriptor_set_layout(vk::DescriptorSetLayout descriptorSetLayout)
{
	state.pipelineLayout.descriptorSetLayouts.push_back(descriptorSetLayout);
	return *this;
}

// Add a push constant range.
PipelineBuilder& PipelineBuilder::add_push_constant_range(vk::PushConstantRange pushConstantRange)
{
	state.pipelineLayout.pushConstantRanges.push_back(pushConstantRange);
	return *this;
}

PipelineBuilder& PipelineBuilder::add_binding_description(uint32_t binding, uint32_t stride, vk::VertexInputRate rate)
{
	state.vertexInput.add_binding_descriptions(binding, stride, rate);
	return *this;
}

PipelineBuilder& PipelineBuilder::add_binding_description(vk::VertexInputBindingDescription bindingDescription)
{
	state.vertexInput.bindingDescriptions.push_back(bindingDescription);
	return *this;
}

PipelineBuilder& PipelineBuilder::add_attribute_description(uint32_t binding, uint32_t location, vk::Format format, uint32_t offset)
{
	vk::VertexInputAttributeDescription attributeDescription(binding, location, format, offset);
	state.vertexInput.attributeDescriptions.push_back(attributeDescription);
	return *this;
}

PipelineBuilder& PipelineBuilder::add_attribute_description(vk::VertexInputAttributeDescription attributeDescription)
{
	state.vertexInput.attributeDescriptions.push_back(attributeDescription);
	return *this;
}

PipelineBuilder& PipelineBuilder::add_attribute_descriptions(std::vector<vk::VertexInputAttributeDescription> attribute_descriptions)
{
	for(const auto& desc : attribute_descriptions)
		state.vertexInput.attributeDescriptions.push_back(desc);
	return *this;
}

// Build and return the Vulkan graphics pipeline.
vk::UniquePipeline PipelineBuilder::build(vk::Device device)
{
	std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;

	for (auto shaderInfo : m_ShaderInfo)
	{
		vk::ShaderModuleCreateInfo stageModuleInfo(
			{},
			shaderInfo.shaderCode.size(),
			reinterpret_cast<const uint32_t*>(shaderInfo.shaderCode.data())
		);
		vk::ShaderModule module = device.createShaderModule(stageModuleInfo);
		m_ShaderModules.push_back({module, shaderInfo.shaderStage});
	}

	for (const auto& shaderModule : m_ShaderModules)
	{
		vk::PipelineShaderStageCreateInfo stageInfo{};
		stageInfo.stage = shaderModule.shaderStage;
		stageInfo.module = shaderModule.shaderModule;
		stageInfo.pName = "main";
		shaderStages.push_back(stageInfo);
	}

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo(
		{},
		state.vertexInput.bindingDescriptions,
		state.vertexInput.attributeDescriptions
	);

	vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo(
		{},
		state.inputAssembly.topology,
		state.inputAssembly.primitiveRestartEnable
	);

	vk::PipelineTessellationStateCreateInfo tessellationInfo({}, state.tessellationState.patchControlPoints);

	vk::PipelineViewportStateCreateInfo viewportStateInfo(
		{},
		state.viewportState.viewports,
		state.viewportState.scissors
	);

	vk::PipelineRasterizationStateCreateInfo rasterizerInfo(
		{},
		state.rasterizationState.depthClamp,
		state.rasterizationState.rasterizerDiscard,
		state.rasterizationState.polygonMode,
		state.rasterizationState.cullMode,
		state.rasterizationState.frontFace,
		state.rasterizationState.depthBias,
		state.rasterizationState.depthBiasConstant,
		state.rasterizationState.depthBiasClamp,
		state.rasterizationState.depthBiasSlope,
		state.rasterizationState.lineWidth
	);

	vk::PipelineMultisampleStateCreateInfo multisamplingInfo(
		{},
		state.multisampleState.rasterizationSamples,
		state.multisampleState.sampleShadingEnable,
		state.multisampleState.minSampleShading,
		state.multisampleState.sampleMasks.data(),
		state.multisampleState.alphaToCoverageEnable,
		state.multisampleState.alphaToOneEnable
	);

	vk::PipelineDepthStencilStateCreateInfo depthStencilInfo(
		{},
		state.depthStencilState.depthTestEnable,
		state.depthStencilState.depthWriteEnable,
		state.depthStencilState.depthCompareOp,
		state.depthStencilState.depthBoundsTestEnable,
		state.depthStencilState.stencilTestEnable,
		state.depthStencilState.front,
		state.depthStencilState.back,
		state.depthStencilState.minDepthBounds,
		state.depthStencilState.maxDepthBounds
	);

	vk::PipelineColorBlendStateCreateInfo colorBlendInfo(
		{},
		state.colorBlendState.logicOpEnable,
		state.colorBlendState.logicOp,
		state.colorBlendState.attachments,
		state.colorBlendState.blendConstants
	);

	vk::PipelineDynamicStateCreateInfo dynamicStateInfo({}, state.dynamicStates);

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo({}, state.pipelineLayout.descriptorSetLayouts, state.pipelineLayout.pushConstantRanges);
	vk::PipelineLayout pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);

	vk::UniquePipeline pipeline;

	if (m_PipelineType == PipelineType::Graphics)
	{
		// Create the graphics pipeline.
		vk::GraphicsPipelineCreateInfo pipelineInfo(
			{},
			shaderStages,
			&vertexInputInfo,
			&inputAssemblyInfo,
			&tessellationInfo,
			&viewportStateInfo,
			&rasterizerInfo,
			&multisamplingInfo,
			&depthStencilInfo,
			&colorBlendInfo,
			&dynamicStateInfo,
			pipelineLayout,
			m_RenderPass,
			m_SubpassIndex
		);

		vk::ResultValue result = device.createGraphicsPipelineUnique(VK_NULL_HANDLE, pipelineInfo);

		if (result.result != vk::Result::eSuccess)
		{
			spdlog::error("Failed to create graphics pipeline: {}", vk::to_string(result.result));
			throw std::runtime_error("Failed to create graphics pipeline.");
		}

		pipeline = std::move(result.value);
	}
	else if (m_PipelineType == PipelineType::Compute)
	{
		// Create the compute pipeline.
		vk::ComputePipelineCreateInfo pipelineInfo(
			{},
			shaderStages[0], // Only one shader stage for compute
			pipelineLayout
		);

		vk::ResultValue result = device.createComputePipelineUnique(VK_NULL_HANDLE, pipelineInfo);

		if (result.result != vk::Result::eSuccess)
		{
			spdlog::error("Failed to create compute pipeline: {}", vk::to_string(result.result));
			throw std::runtime_error("Failed to create compute pipeline.");
		}

		pipeline = std::move(result.value);
	}

	for (auto& shaderModule : m_ShaderModules)
	{
		device.destroyShaderModule(shaderModule.shaderModule);
	}

	device.destroyPipelineLayout(pipelineLayout);

	if (pipeline.get() == VK_NULL_HANDLE)
	{
		spdlog::error("Failed to create pipeline, returned null handle.");
		throw std::runtime_error("Pipeline creation failed.");
	}

	return std::move(pipeline);
}

// Build and return the Vulkan graphics pipeline.
VkPipeline PipelineBuilder::build(VkDevice device)
{
	std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;

	for (auto shaderInfo : m_ShaderInfo)
	{
		vk::ShaderModuleCreateInfo stageModuleInfo(
			{},
			shaderInfo.shaderCode.size(),
			reinterpret_cast<const uint32_t*>(shaderInfo.shaderCode.data())
		);
		VkShaderModule module = VK_NULL_HANDLE;
		vkCreateShaderModule(device, reinterpret_cast<const VkShaderModuleCreateInfo*>(&stageModuleInfo), nullptr, &module);
		m_ShaderModules.push_back({ module, shaderInfo.shaderStage });
	}

	for (const auto& shaderModule : m_ShaderModules)
	{
		vk::PipelineShaderStageCreateInfo stageInfo{};
		stageInfo.stage = shaderModule.shaderStage;
		stageInfo.module = shaderModule.shaderModule;
		stageInfo.pName = "main";
		shaderStages.push_back(stageInfo);
	}

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo(
		{},
		state.vertexInput.bindingDescriptions,
		state.vertexInput.attributeDescriptions
	);

	vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo(
		{},
		state.inputAssembly.topology,
		state.inputAssembly.primitiveRestartEnable
	);

	vk::PipelineTessellationStateCreateInfo tessellationInfo({}, state.tessellationState.patchControlPoints);

	vk::PipelineViewportStateCreateInfo viewportStateInfo(
		{},
		state.viewportState.viewports,
		state.viewportState.scissors
	);

	vk::PipelineRasterizationStateCreateInfo rasterizerInfo(
		{},
		state.rasterizationState.depthClamp,
		state.rasterizationState.rasterizerDiscard,
		state.rasterizationState.polygonMode,
		state.rasterizationState.cullMode,
		state.rasterizationState.frontFace,
		state.rasterizationState.depthBias,
		state.rasterizationState.depthBiasConstant,
		state.rasterizationState.depthBiasClamp,
		state.rasterizationState.depthBiasSlope,
		state.rasterizationState.lineWidth
	);

	vk::PipelineMultisampleStateCreateInfo multisamplingInfo(
		{},
		state.multisampleState.rasterizationSamples,
		state.multisampleState.sampleShadingEnable,
		state.multisampleState.minSampleShading,
		state.multisampleState.sampleMasks.data(),
		state.multisampleState.alphaToCoverageEnable,
		state.multisampleState.alphaToOneEnable
	);

	vk::PipelineDepthStencilStateCreateInfo depthStencilInfo(
		{},
		state.depthStencilState.depthTestEnable,
		state.depthStencilState.depthWriteEnable,
		state.depthStencilState.depthCompareOp,
		state.depthStencilState.depthBoundsTestEnable,
		state.depthStencilState.stencilTestEnable,
		state.depthStencilState.front,
		state.depthStencilState.back,
		state.depthStencilState.minDepthBounds,
		state.depthStencilState.maxDepthBounds
	);

	vk::PipelineColorBlendStateCreateInfo colorBlendInfo(
		{},
		state.colorBlendState.logicOpEnable,
		state.colorBlendState.logicOp,
		state.colorBlendState.attachments,
		state.colorBlendState.blendConstants
	);

	vk::PipelineDynamicStateCreateInfo dynamicStateInfo({}, state.dynamicStates);

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo({}, state.pipelineLayout.descriptorSetLayouts, state.pipelineLayout.pushConstantRanges);
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
	vkCreatePipelineLayout(device, reinterpret_cast<const VkPipelineLayoutCreateInfo*>(&pipelineLayoutInfo), nullptr, &pipelineLayout);

	VkPipeline pipeline;

	if (m_PipelineType == PipelineType::Graphics)
	{
		// Create the graphics pipeline.
		vk::GraphicsPipelineCreateInfo pipelineInfo(
			{},
			shaderStages,
			&vertexInputInfo,
			&inputAssemblyInfo,
			&tessellationInfo,
			&viewportStateInfo,
			&rasterizerInfo,
			&multisamplingInfo,
			&depthStencilInfo,
			&colorBlendInfo,
			&dynamicStateInfo,
			pipelineLayout,
			m_RenderPass,
			m_SubpassIndex
		);

		vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, reinterpret_cast<const VkGraphicsPipelineCreateInfo*>(&pipelineInfo), nullptr, &pipeline);

		if (pipeline == VK_NULL_HANDLE)
		{
			throw std::runtime_error("Failed to create graphics pipeline.");
		}
	}
	else if (m_PipelineType == PipelineType::Compute)
	{
		// Create the compute pipeline.
		vk::ComputePipelineCreateInfo pipelineInfo(
			{},
			shaderStages[0], // Only one shader stage for compute
			pipelineLayout
		);

		vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, reinterpret_cast<const VkComputePipelineCreateInfo*>(&pipelineInfo), nullptr, &pipeline);
		if (pipeline == VK_NULL_HANDLE)
		{
			throw std::runtime_error("Failed to create compute pipeline.");
		}
	}

	for (auto& shaderModule : m_ShaderModules)
	{
		vkDestroyShaderModule(device, shaderModule.shaderModule, nullptr);
	}

	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

	if (pipeline == VK_NULL_HANDLE)
	{
		spdlog::error("Failed to create pipeline, returned null handle.");
		throw std::runtime_error("Pipeline creation failed.");
	}

	return pipeline;
}

// Build and return the Vulkan graphics pipeline.
VkPipeline PipelineBuilder::build(VkDevice device, VkPipelineLayout& layout)
{
	std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;

	for (auto shaderInfo : m_ShaderInfo)
	{
		vk::ShaderModuleCreateInfo stageModuleInfo(
			{},
			shaderInfo.shaderCode.size(),
			reinterpret_cast<const uint32_t*>(shaderInfo.shaderCode.data())
		);
		VkShaderModule module = VK_NULL_HANDLE;
		vkCreateShaderModule(device, reinterpret_cast<const VkShaderModuleCreateInfo*>(&stageModuleInfo), nullptr, &module);
		m_ShaderModules.push_back({ module, shaderInfo.shaderStage });
	}

	for (const auto& shaderModule : m_ShaderModules)
	{
		vk::PipelineShaderStageCreateInfo stageInfo{};
		stageInfo.stage = shaderModule.shaderStage;
		stageInfo.module = shaderModule.shaderModule;
		stageInfo.pName = "main";
		shaderStages.push_back(stageInfo);
	}

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo(
		{},
		state.vertexInput.bindingDescriptions,
		state.vertexInput.attributeDescriptions
	);

	vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo(
		{},
		state.inputAssembly.topology,
		state.inputAssembly.primitiveRestartEnable
	);

	vk::PipelineTessellationStateCreateInfo tessellationInfo({}, state.tessellationState.patchControlPoints);

	vk::PipelineViewportStateCreateInfo viewportStateInfo(
		{},
		state.viewportState.viewports,
		state.viewportState.scissors
	);

	vk::PipelineRasterizationStateCreateInfo rasterizerInfo(
		{},
		state.rasterizationState.depthClamp,
		state.rasterizationState.rasterizerDiscard,
		state.rasterizationState.polygonMode,
		state.rasterizationState.cullMode,
		state.rasterizationState.frontFace,
		state.rasterizationState.depthBias,
		state.rasterizationState.depthBiasConstant,
		state.rasterizationState.depthBiasClamp,
		state.rasterizationState.depthBiasSlope,
		state.rasterizationState.lineWidth
	);

	vk::PipelineMultisampleStateCreateInfo multisamplingInfo(
		{},
		state.multisampleState.rasterizationSamples,
		state.multisampleState.sampleShadingEnable,
		state.multisampleState.minSampleShading,
		state.multisampleState.sampleMasks.data(),
		state.multisampleState.alphaToCoverageEnable,
		state.multisampleState.alphaToOneEnable
	);

	vk::PipelineDepthStencilStateCreateInfo depthStencilInfo(
		{},
		state.depthStencilState.depthTestEnable,
		state.depthStencilState.depthWriteEnable,
		state.depthStencilState.depthCompareOp,
		state.depthStencilState.depthBoundsTestEnable,
		state.depthStencilState.stencilTestEnable,
		state.depthStencilState.front,
		state.depthStencilState.back,
		state.depthStencilState.minDepthBounds,
		state.depthStencilState.maxDepthBounds
	);

	vk::PipelineColorBlendStateCreateInfo colorBlendInfo(
		{},
		state.colorBlendState.logicOpEnable,
		state.colorBlendState.logicOp,
		state.colorBlendState.attachments,
		state.colorBlendState.blendConstants
	);

	vk::PipelineDynamicStateCreateInfo dynamicStateInfo({}, state.dynamicStates);

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo({}, state.pipelineLayout.descriptorSetLayouts, state.pipelineLayout.pushConstantRanges);
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
	vkCreatePipelineLayout(device, reinterpret_cast<const VkPipelineLayoutCreateInfo*>(&pipelineLayoutInfo), nullptr, &pipelineLayout);

	VkPipeline pipeline;

	if (m_PipelineType == PipelineType::Graphics)
	{
		// Create the graphics pipeline.
		vk::GraphicsPipelineCreateInfo pipelineInfo(
			{},
			shaderStages,
			&vertexInputInfo,
			&inputAssemblyInfo,
			&tessellationInfo,
			&viewportStateInfo,
			&rasterizerInfo,
			&multisamplingInfo,
			&depthStencilInfo,
			&colorBlendInfo,
			&dynamicStateInfo,
			pipelineLayout,
			m_RenderPass,
			m_SubpassIndex
		);

		vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, reinterpret_cast<const VkGraphicsPipelineCreateInfo*>(&pipelineInfo), nullptr, &pipeline);

		if (pipeline == VK_NULL_HANDLE)
		{
			throw std::runtime_error("Failed to create graphics pipeline.");
		}
	}
	else if (m_PipelineType == PipelineType::Compute)
	{
		// Create the compute pipeline.
		vk::ComputePipelineCreateInfo pipelineInfo(
			{},
			shaderStages[0], // Only one shader stage for compute
			pipelineLayout
		);

		vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, reinterpret_cast<const VkComputePipelineCreateInfo*>(&pipelineInfo), nullptr, &pipeline);
		if (pipeline == VK_NULL_HANDLE)
		{
			throw std::runtime_error("Failed to create compute pipeline.");
		}
	}

	for (auto& shaderModule : m_ShaderModules)
	{
		vkDestroyShaderModule(device, shaderModule.shaderModule, nullptr);
	}

	if (pipeline == VK_NULL_HANDLE)
	{
		spdlog::error("Failed to create pipeline, returned null handle.");
		throw std::runtime_error("Pipeline creation failed.");
	}

	layout = pipelineLayout;
	return pipeline;
}

// Helper to add or remove a dynamic state.
void PipelineBuilder::toggle_dynamic_state(bool enable, vk::DynamicState dynamicState)
{
	auto it = std::find(state.dynamicStates.begin(),
		state.dynamicStates.end(),
		dynamicState
	);

	if (enable && it == state.dynamicStates.end())
	{
		state.dynamicStates.push_back(dynamicState);
	}
	else if (!enable && it != state.dynamicStates.end())
	{
		state.dynamicStates.erase(it);
	}
}