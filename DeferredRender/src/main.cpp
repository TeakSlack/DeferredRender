#include "vk_context.h"
#include "vk_swapchain.h"
#include "vk_render_pass.h"
#include "logger.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>
#include <cstdio>   // only for the --help printf before logger is up

static constexpr u32 INITIAL_WIDTH  = 1280;
static constexpr u32 INITIAL_HEIGHT = 720;

// -------------------------------------------------------------------------
// AppConfig — populated from argv before anything else runs
// -------------------------------------------------------------------------
struct AppConfig {
    bool        verbose  = false;
    std::string log_file = "";
};

static AppConfig parse_args(int argc, char** argv)
{
    AppConfig cfg;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--verbose" || arg == "-v") {
            cfg.verbose = true;

        } else if ((arg == "--log-file" || arg == "-l") && i + 1 < argc) {
            cfg.log_file = argv[++i];

        } else if (arg == "--help" || arg == "-h") {
            printf(
                "Usage: vk_renderer [options]\n"
                "\n"
                "Options:\n"
                "  -v, --verbose           Show trace/debug messages and source locations\n"
                "  -l, --log-file <path>   Mirror all output to a log file (no color codes)\n"
                "  -h, --help              Print this message\n"
            );
            exit(0);

        } else {
            // Logger not up yet — use fprintf for this one early error
            fprintf(stderr, "Unknown argument: %s  (use --help)\n", arg.c_str());
            exit(1);
        }
    }
#ifdef DEBUG
	cfg.verbose = true; // force verbose in debug builds, regardless of args
#endif

    return cfg;
}

// -------------------------------------------------------------------------
// Command infrastructure — one pool + one primary cmd buffer per frame slot
// -------------------------------------------------------------------------
struct FrameData {
    VkCommandPool   command_pool   = VK_NULL_HANDLE;
    VkCommandBuffer command_buffer = VK_NULL_HANDLE;
};

static FrameData frame_data[MAX_FRAMES_IN_FLIGHT] = {};

static void create_command_infrastructure(const VkContext& ctx)
{
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VkCommandPoolCreateInfo pool_info = {};
        pool_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.queueFamilyIndex = ctx.queue_families.graphics;
        pool_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        VK_CHECK(vkCreateCommandPool(ctx.device, &pool_info, nullptr,
                                     &frame_data[i].command_pool));

        VkCommandBufferAllocateInfo alloc_info = {};
        alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool        = frame_data[i].command_pool;
        alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount = 1;
        VK_CHECK(vkAllocateCommandBuffers(ctx.device, &alloc_info,
                                          &frame_data[i].command_buffer));

        LOG_DEBUG_TO("render", "Frame slot {} — command pool + buffer allocated", i);
    }
}

static void destroy_command_infrastructure(const VkContext& ctx)
{
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        vkDestroyCommandPool(ctx.device, frame_data[i].command_pool, nullptr);
    LOG_DEBUG_TO("render", "Command infrastructure destroyed");
}

// -------------------------------------------------------------------------
// Record — Phase 1: clear the swapchain image, transition to present layout.
// Replaced in Phase 2 by a proper render pass.
// -------------------------------------------------------------------------
static void record_commands(VkCommandBuffer cmd, VkImage swapchain_image, u32 frame_slot)
{
    LOG_VERBOSE("Recording commands for frame slot {}", frame_slot);

    VkCommandBufferBeginInfo begin = {};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK(vkBeginCommandBuffer(cmd, &begin));

    // Transition: UNDEFINED -> TRANSFER_DST_OPTIMAL
    VkImageMemoryBarrier barrier_to_clear        = {};
    barrier_to_clear.sType                       = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier_to_clear.oldLayout                   = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier_to_clear.newLayout                   = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier_to_clear.srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
    barrier_to_clear.dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
    barrier_to_clear.image                       = swapchain_image;
    barrier_to_clear.subresourceRange            = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    barrier_to_clear.srcAccessMask               = 0;
    barrier_to_clear.dstAccessMask               = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier_to_clear);

    // Dark teal — distinguishable from a crash black at a glance
    VkClearColorValue clear_color = {};
    clear_color.float32[0] = 0.05f;
    clear_color.float32[1] = 0.10f;
    clear_color.float32[2] = 0.12f;
    clear_color.float32[3] = 1.00f;
    VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    vkCmdClearColorImage(cmd, swapchain_image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color, 1, &range);

    // Transition: TRANSFER_DST_OPTIMAL -> PRESENT_SRC_KHR
    VkImageMemoryBarrier barrier_to_present = barrier_to_clear;
    barrier_to_present.oldLayout            = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier_to_present.newLayout            = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    barrier_to_present.srcAccessMask        = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier_to_present.dstAccessMask        = 0;

    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier_to_present);

    VK_CHECK(vkEndCommandBuffer(cmd));
}

// -------------------------------------------------------------------------
// Resize callback
// -------------------------------------------------------------------------
static bool g_framebuffer_resized = false;
static void framebuffer_resize_callback(GLFWwindow*, int w, int h)
{
    g_framebuffer_resized = true;
    LOG_DEBUG_TO("render", "Framebuffer resize detected: {}x{}", w, h);
}

// =========================================================================
// main
// =========================================================================
int main(int argc, char** argv)
{
    AppConfig cfg = parse_args(argc, argv);

    // Logger is the very first engine system initialised
    Logger::init(cfg.verbose, cfg.log_file);

    LOG_INFO("vk_renderer starting");
    if (cfg.verbose)
        LOG_DEBUG("Verbose mode on, trace/debug messages and source locations visible");
    if (!cfg.log_file.empty())
        LOG_INFO("Logging to file: {}", cfg.log_file);

    // -------- GLFW --------
    LOG_DEBUG("Initialising GLFW");
    if (!glfwInit()) {
        LOG_FATAL("glfwInit() failed");
        return 1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE,  GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(
        INITIAL_WIDTH, INITIAL_HEIGHT, "vk_renderer | phase 1", nullptr, nullptr);
    if (!window) {
        LOG_FATAL("glfwCreateWindow() failed");
        glfwTerminate();
        return 1;
    }
    glfwSetFramebufferSizeCallback(window, framebuffer_resize_callback);
    LOG_INFO("Window created ({}x{})", INITIAL_WIDTH, INITIAL_HEIGHT);

    // -------- Vulkan init --------
    VkContext   ctx = {};
    VkSwapchain sc  = {};

    vk_context_init(ctx, window);
    vk_swapchain_create(sc, ctx, INITIAL_WIDTH, INITIAL_HEIGHT);
    create_command_infrastructure(ctx);

    LOG_INFO("Initialisation complete, entering main loop");

    // -------- Main loop --------
    u64 frame_number = 0;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        int fb_w = 0, fb_h = 0;
        glfwGetFramebufferSize(window, &fb_w, &fb_h);
        if (fb_w == 0 || fb_h == 0) {
            LOG_VERBOSE("Window minimised — skipping frame {}", frame_number);
            continue;
        }

        u32 image_index = vk_swapchain_acquire(sc, ctx);
        if (image_index == UINT32_MAX || g_framebuffer_resized) {
            g_framebuffer_resized = false;
            LOG_INFO_TO("render", "Swapchain out of date — recreating");
            vk_swapchain_recreate(sc, ctx, (u32)fb_w, (u32)fb_h);
            continue;
        }

        LOG_VERBOSE("Frame {} — image index {}, frame slot {}",
                    frame_number, image_index, sc.current_frame);

        VkCommandBuffer cmd = frame_data[sc.current_frame].command_buffer;
        vkResetCommandBuffer(cmd, 0);
        record_commands(cmd, sc.images[image_index], sc.current_frame);

        bool ok = vk_swapchain_submit_and_present(sc, ctx, cmd, image_index);
        if (!ok) {
            glfwGetFramebufferSize(window, &fb_w, &fb_h);
            LOG_WARN_TO("render", "Present returned out-of-date — recreating swapchain");
            vk_swapchain_recreate(sc, ctx, (u32)fb_w, (u32)fb_h);
        }

        ++frame_number;
    }

    LOG_INFO("Main loop exited after {} frames", frame_number);

    // -------- Cleanup --------
    LOG_DEBUG("Waiting for GPU idle before shutdown");
    vkDeviceWaitIdle(ctx.device);

    destroy_command_infrastructure(ctx);
    vk_swapchain_destroy(sc, ctx);
    vk_context_destroy(ctx);

    glfwDestroyWindow(window);
    glfwTerminate();

    LOG_INFO("Shutdown complete");
    spdlog::shutdown();
    return 0;
}
