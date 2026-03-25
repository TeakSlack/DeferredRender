#ifndef PIPELINE_BUILDER_H
#define PIPELINE_BUILDER_H

#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <type_traits>
#include <array>

#include <vulkan/vulkan.hpp>

// VertexFormat CRTP base class
// Provides a static interface for derived vertex types to define their
// Vulkan binding and attribute descriptions.
//template<typename Derived>
//class VertexFormat
//{
//	// Returns the name of the vertex format (must be implemented by Derived)
//	static consteval std::string_view name() { return Derived::name(); }
//
//	// Returns the Vulkan binding description for this vertex format
//	static consteval vk::VertexInputBindingDescription get_binding_description();
//
//	// Returns the Vulkan attribute descriptions for this vertex format
//	static consteval vk::VertexInputAttributeDescription get_attribute_description();
//};

// TODO: Raytracing pipeline support
enum class PipelineType
{
	Graphics,
	Compute,
};

// Helper class for building Vulkan graphics pipelines with a fluent interface.
class PipelineBuilder
{
public:
	PipelineBuilder(PipelineType pipelineType);
	~PipelineBuilder();

	// Add a shader stage from a SPIR-V file.
	PipelineBuilder& add_shader_stage(const std::string& shaderPath, vk::ShaderStageFlagBits shaderStage);
	PipelineBuilder& add_shader_stage(vk::ShaderModule shaderModule, vk::ShaderStageFlagBits shaderStage);

	// Set input assembly and primitive topology.
	PipelineBuilder& set_primitive_topology(vk::PrimitiveTopology topology);
	PipelineBuilder& set_primitive_restart(bool enable);
	PipelineBuilder& set_dynamic_topology(bool enable);
	PipelineBuilder& set_patch_control_points(uint32_t points);

	// Add dynamic state, viewport, and scissor rectangles.
	PipelineBuilder& add_dynamic_state(vk::DynamicState dynamicState);
	PipelineBuilder& add_viewport(vk::Viewport viewport);
	PipelineBuilder& add_viewport(float x, float y, float width, float height, float minDepth, float maxDepth);
	PipelineBuilder& add_scissor(vk::Rect2D scissor);
	PipelineBuilder& add_scissor(int32_t x, int32_t y, uint32_t width, uint32_t height);

	// Rasterization state.
	PipelineBuilder& set_depth_clamp(bool enable);
	PipelineBuilder& set_rasterizer_discard(bool enable);
	PipelineBuilder& set_polygon_mode(vk::PolygonMode mode);
	PipelineBuilder& set_cull_mode(vk::CullModeFlags mode);
	PipelineBuilder& set_front_face(vk::FrontFace face);
	PipelineBuilder& set_depth_bias(bool enable);
	PipelineBuilder& set_line_width(float width);
	PipelineBuilder& set_depth_bias_constant(float constant);
	PipelineBuilder& set_depth_bias_clamp(float clamp);
	PipelineBuilder& set_depth_bias_slope(float slope);
	PipelineBuilder& set_dynamic_line_width(bool enable);
	PipelineBuilder& set_dynamic_depth_bias(bool enable);

	// Multisample state.
	PipelineBuilder& set_sample_count(vk::SampleCountFlagBits count);
	PipelineBuilder& set_sample_shading(bool enable, float minSampleShading);
	PipelineBuilder& add_sample_mask(vk::SampleMask mask);
	PipelineBuilder& set_alpha_to_coverage(bool enable);
	PipelineBuilder& set_alpha_to_one(bool enable);

	// Render pass and subpass.
	PipelineBuilder& set_render_pass(vk::RenderPass& renderPass, uint32_t subpassIndex);
	PipelineBuilder& set_render_pass(VkRenderPass& renderPass, uint32_t subpassIndex);

	// Depth/stencil state.
	PipelineBuilder& set_depth_test(bool enable);
	PipelineBuilder& set_depth_write(bool enable);
	PipelineBuilder& set_depth_compare_op(vk::CompareOp compareOp);
	PipelineBuilder& set_depth_bounds_test(bool enable);
	PipelineBuilder& set_stencil_test(bool enable);
	PipelineBuilder& set_stencil_front(vk::StencilOpState front);
	PipelineBuilder& set_stencil_back(vk::StencilOpState back);
	PipelineBuilder& set_min_depth_bounds(float minDepthBounds);
	PipelineBuilder& set_max_depth_bounds(float maxDepthBounds);

	// Color blend state.
	PipelineBuilder& set_logic_op(bool enable, vk::LogicOp logicOp);
	PipelineBuilder& add_color_blend_attachment(vk::PipelineColorBlendAttachmentState& attachment);
	PipelineBuilder& set_blend_constants(const std::array<float, 4>& constants);
	PipelineBuilder& color_blend_write_all();

	// Pipeline layout.
	PipelineBuilder& add_descriptor_set_layout(vk::DescriptorSetLayout descriptorSetLayout);
	PipelineBuilder& add_push_constant_range(vk::PushConstantRange pushConstantRange);

	// Vertex input state.
	PipelineBuilder& add_binding_description(uint32_t binding, uint32_t stride, vk::VertexInputRate rate);
	PipelineBuilder& add_binding_description(vk::VertexInputBindingDescription);
	PipelineBuilder& add_attribute_description(uint32_t binding, uint32_t location, vk::Format format, uint32_t offset);
	PipelineBuilder& add_attribute_description(vk::VertexInputAttributeDescription);
	PipelineBuilder& add_attribute_descriptions(std::vector<vk::VertexInputAttributeDescription> attribute_descriptions);


	// Build the pipeline.
	vk::UniquePipeline build(vk::Device device);
	VkPipeline build(VkDevice device);
	VkPipeline build(VkDevice device, VkPipelineLayout& layout);

	// Set the vertex format using a CRTP vertex type.
	/*template<typename T>
	PipelineBuilder& set_vertex_format()
	{
		static_assert(std::is_base_of_v<T, VertexFormat>, "Must inherit from VertexFormat CRTP");
		state.vertexInput.add_binding_descriptions(T::get_binding_description());
		state.vertexInput.add_attribute_description(T::get_attribute_description());
	}*/

	// Holds all pipeline state for construction.
	struct State
	{
		// Vertex input state.
		struct VertexInput
		{
			std::vector<vk::VertexInputBindingDescription> bindingDescriptions;
			std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;

			VertexInput& add_binding_descriptions(uint32_t binding, uint32_t stride, vk::VertexInputRate rate)
			{
				bindingDescriptions.push_back({ binding, stride, rate });
				return *this;
			}

			VertexInput& add_binding_descriptions(vk::VertexInputBindingDescription description)
			{
				bindingDescriptions.push_back(description);
				return *this;
			}

			VertexInput& add_attribute_description(uint32_t binding, uint32_t location, vk::Format format, uint32_t offset)
			{
				attributeDescriptions.push_back({ binding, location, format, offset });
				return *this;
			}

			VertexInput& add_attribute_description(vk::VertexInputAttributeDescription description)
			{
				attributeDescriptions.push_back(description);
				return *this;
			}
		} vertexInput;

		// Input assembly state.
		struct InputAssembly
		{
			vk::PrimitiveTopology topology = vk::PrimitiveTopology::eTriangleList;
			vk::Bool32 primitiveRestartEnable = vk::False;
		} inputAssembly;

		// Tessellation state.
		struct TesselationState
		{
			uint32_t patchControlPoints = 0;
		} tessellationState;

		// Dynamic states.
		std::vector<vk::DynamicState> dynamicStates;

		// Viewport and scissor state.
		struct ViewportState
		{
			std::vector<vk::Viewport> viewports;
			std::vector<vk::Rect2D> scissors;
		} viewportState;

		// Rasterization state.
		struct RasterizationState
		{
			vk::Bool32 depthClamp = vk::False;
			vk::Bool32 rasterizerDiscard = vk::False;
			vk::PolygonMode polygonMode = vk::PolygonMode::eFill;
			vk::CullModeFlags cullMode = vk::CullModeFlagBits::eBack;
			vk::FrontFace frontFace = vk::FrontFace::eCounterClockwise;
			vk::Bool32 depthBias = vk::False;
			float lineWidth = 1.0f;
			float depthBiasConstant = 0.0f;
			float depthBiasClamp = 0.0f;
			float depthBiasSlope = 0.0f;
		} rasterizationState;

		// Multisample state.
		struct MultisampleState
		{
			vk::SampleCountFlagBits rasterizationSamples = vk::SampleCountFlagBits::e1;
			vk::Bool32 sampleShadingEnable = vk::False;
			float minSampleShading = 1.0f;
			std::vector<vk::SampleMask> sampleMasks;
			vk::Bool32 alphaToCoverageEnable = vk::False;
			vk::Bool32 alphaToOneEnable = vk::False;
		} multisampleState;

		// Depth/stencil state.
		struct DepthStencilState
		{
			vk::Bool32 depthTestEnable = vk::False;
			vk::Bool32 depthWriteEnable = vk::False;
			vk::CompareOp depthCompareOp = vk::CompareOp::eNever;
			vk::Bool32 depthBoundsTestEnable = vk::False;
			vk::Bool32 stencilTestEnable = vk::False;
			vk::StencilOpState front{};
			vk::StencilOpState back{};
			float minDepthBounds = 0.0f;
			float maxDepthBounds = 1.0f;
		} depthStencilState;

		// Color blend state.
		struct ColorBlendState
		{
			vk::Bool32 logicOpEnable = false;
			vk::LogicOp logicOp = vk::LogicOp::eNoOp;
			std::vector<vk::PipelineColorBlendAttachmentState> attachments;
			std::array<float, 4> blendConstants;
		} colorBlendState;

		// Pipeline layout state.
		struct PipelineLayout
		{
			std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
			std::vector<vk::PushConstantRange> pushConstantRanges;
		} pipelineLayout;
	} state;
	
private:
	struct ShaderInfo
	{
		std::vector<char> shaderCode;
		vk::ShaderStageFlagBits shaderStage;
	};

	struct ShaderModule
	{
		vk::ShaderModule shaderModule;
		vk::ShaderStageFlagBits shaderStage;
	};

	PipelineType m_PipelineType;
	vk::RenderPass m_RenderPass;
	uint32_t m_SubpassIndex = 0;

	std::vector<ShaderInfo> m_ShaderInfo;
	std::vector<ShaderModule> m_ShaderModules;
private:
	// Helper to enable/disable a dynamic state.
	void toggle_dynamic_state(bool enable, vk::DynamicState dynamicState);
};

#endif