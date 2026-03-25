#include "vk_pipeline.h"
#include <glm/glm.hpp>
#include <fstream>
#include <vector>

// =========================================================================
// Shader loading
// =========================================================================

VkShaderModule vk_load_shader(VkDevice device, const char* spv_path)
{
    std::ifstream file(spv_path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        LOG_FATAL_TO("render", "Failed to open shader SPIR-V: {}", spv_path);
        abort();
    }
    size_t file_size = (size_t)file.tellg();
    std::vector<char> buf(file_size);
    file.seekg(0);
    file.read(buf.data(), (std::streamsize)file_size);

    VkShaderModuleCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = buf.size();
    create_info.pCode = reinterpret_cast<const u32*>(buf.data());

    VkShaderModule mod;
    VK_CHECK(vkCreateShaderModule(device, &create_info, nullptr, &mod));
    LOG_DEBUG_TO("render", "Shader loaded: {}", spv_path);
    return mod;
}

void vk_destroy_shader(VkDevice device, VkShaderModule mod)
{
    vkDestroyShaderModule(device, mod, nullptr);
}

// =========================================================================
// Pipeline layout
// =========================================================================

VkPipelineLayout vk_create_pipeline_layout(
    VkDevice              device,
    VkDescriptorSetLayout frame_set_layout,
    VkDescriptorSetLayout material_set_layout)
{
    VkDescriptorSetLayout set_layouts[] = {
        frame_set_layout,    // set 0 - camera / per-frame
        material_set_layout, // set 1 - material textures
    };

    // Two push constant ranges - vertex gets the model matrix,
    // fragment gets the material tint. They don't overlap.
    VkPushConstantRange push_ranges[2] = {};

    push_ranges[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push_ranges[0].offset = 0;
    push_ranges[0].size = sizeof(glm::mat4); // 64 bytes

    push_ranges[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    push_ranges[1].offset = sizeof(glm::mat4); // starts after the mat4
    push_ranges[1].size = sizeof(MaterialPushConstants); // 16 bytes

    VkPipelineLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount = 2;
    layout_info.pSetLayouts = set_layouts;
    layout_info.pushConstantRangeCount = 2;
    layout_info.pPushConstantRanges = push_ranges;

    VkPipelineLayout layout;
    VK_CHECK(vkCreatePipelineLayout(device, &layout_info, nullptr, &layout));
    return layout;
}

// =========================================================================
// Graphics pipeline
// =========================================================================

VkPipeline vk_create_graphics_pipeline(VkDevice device, const PipelineCreateInfo& info)
{
    // --- shader stages ---
    VkPipelineShaderStageCreateInfo stages[2] = {};

    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = info.vert_module;
    stages[0].pName = "main";

    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = info.frag_module;
    stages[1].pName = "main";

    // --- vertex input: driven by Vertex struct ---
    auto binding = Vertex::binding_desc();
    auto attrs = Vertex::attribute_descs();

    VkPipelineVertexInputStateCreateInfo vertex_input = {};
    vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input.vertexBindingDescriptionCount = 1;
    vertex_input.pVertexBindingDescriptions = &binding;
    vertex_input.vertexAttributeDescriptionCount = (u32)attrs.size();
    vertex_input.pVertexAttributeDescriptions = attrs.data();

    // --- input assembly ---
    VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    // --- viewport + scissor (static, matched to swapchain at creation time) ---
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (f32)info.viewport_extent.width;
    viewport.height = (f32)info.viewport_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = info.viewport_extent;

    // Mark viewport/scissor as dynamic so resize doesn't require pipeline rebuild
    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    VkPipelineDynamicStateCreateInfo dynamic_state = {};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = 2;
    dynamic_state.pDynamicStates = dynamic_states;

    VkPipelineViewportStateCreateInfo viewport_state = {};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;
    // pViewports / pScissors are null because they're dynamic

    // --- rasterizer ---
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode =
        (info.material_type == MaterialType::PBRMasked)
        ? VK_CULL_MODE_NONE  // masked: can't cull back faces (alpha may punch holes)
        : VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    // --- multisampling: off ---
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // --- depth/stencil ---
    VkPipelineDepthStencilStateCreateInfo depth_stencil = {};
    depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable = VK_TRUE;
    depth_stencil.depthWriteEnable = VK_TRUE;
    depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;

    // --- color blend: one attachment, no blending ---
    VkPipelineColorBlendAttachmentState blend_attachment = {};
    blend_attachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blend_attachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo color_blend = {};
    color_blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend.attachmentCount = 1;
    color_blend.pAttachments = &blend_attachment;

    // --- assemble ---
    VkGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = stages;
    pipeline_info.pVertexInputState = &vertex_input;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pDepthStencilState = &depth_stencil;
    pipeline_info.pColorBlendState = &color_blend;
    pipeline_info.pDynamicState = &dynamic_state;
    pipeline_info.layout = info.layout;
    pipeline_info.renderPass = info.render_pass;
    pipeline_info.subpass = 0;

    VkPipeline pipeline;
    VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1,
        &pipeline_info, nullptr, &pipeline));
    LOG_INFO_TO("render", "Graphics pipeline created (type={})",
        (int)info.material_type);
    return pipeline;
}