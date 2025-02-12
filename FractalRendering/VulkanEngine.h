#pragma once
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include "vulkan/vulkan_core.h"
#include <chrono>
#include <iterator>
#include <vulkan/vulkan.h>

//#define GLFW_INCLUDE_VULKAN

#define _USE_MATH_DEFINES
#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>

#include <cmath>

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <random>
#include <set>
#include <stdexcept>
#include <vector>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char *> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

const std::vector<const char *> validationLayers = {
	"VK_LAYER_KHRONOS_validation",
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

struct Vertex {
	float pos[4];
	float color[4];
	// float texCoord[2];

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};

		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 2>
	getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 2>
			attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		/*
		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, texCoord);
		*/

		return attributeDescriptions;
	}
};

/*
struct mat4x4 {
	float a;
	float b;
	float c;
	float d;
	float e;
	float f;
	float g;
	float h;
	float i;
	float j;
	float k;
	float l;
	float m;
	float n;
	float o;
	float p;
};*/
typedef std::array<std::array<float, 4>, 4> mat4x4;
mat4x4 transpose(mat4x4 a);
mat4x4 mul(mat4x4 a, mat4x4 b);

struct ComputeUniformBufferObject {
	/*
	float time = 1.0f;
	float deltaTime = 1.0f;
	*/
	float lo_xy[2] = {-1.0f, -1.0f};
	float hi_xy[2] = {1.0f, 1.0f};

	float julia_c[2] = {0.5f, 0.5f};
	int subdivisionCount = 1;

	/*
	float defaultLifetime;
	float lifetimeJiggle;
	int sphereBufferSize;
	*/
};

struct GraphicsUniformBufferObject {
	mat4x4 model;
	mat4x4 view;
	mat4x4 proj;
};

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsAndComputeFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete() {
		return graphicsAndComputeFamily.has_value() &&
			   presentFamily.has_value();
	}
};
struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

/*
struct VulkanBufferHandle {
	VkBuffer buf;
	VkDeviceMemory mem;
};

struct VulkanImageHandle {
	VkBuffer img;
	VkDeviceMemory mem;
	VkImageView imgView;
	VkFormat format;
	VkExtent2D extent;

	// Necessary for samplable images
	VkSampler sampler;
};

struct VulkanDescriptorSetHandle {
	VkDescriptorSet set;
	VkDescriptorSetLayout layout;
};

struct VulkanPipelineHandle {
	VkPipeline pipeline;
	VkPipelineLayout layout;

	// For Graphics
	VkRenderPass renderPass;
};

struct VulkanEngineInitInfo {
	std::vector<std::tuple<VulkanBufferHandle *, const void *, const size_t>>
		bufferData;
	std::vector<
		std::tuple<VulkanImageHandle *, const uint32_t, const uint32_t,
				   const VkFormat, const VkImageUsageFlags,
				   const VkMemoryPropertyFlags, const VkImageAspectFlags>>
		imgData;

	std::vector<std::vector<VkDescriptorSetLayoutBinding>>
		descriptorSetLayoutBindings;

	std::vector<std::tuple<VulkanDescriptorSetHandle *, size_t,
						   std::vector<VkWriteDescriptorSet>>>
		descriptorSets;

	std::vector<const char *> computeShaders;
};
*/

class VulkanEngine {
  public:
	void glfwWindowInit();
	void initVulkan();
	void initUI();
	void mainLoop();
	void cleanup();

  private:
	GLFWwindow *window;

	VkInstance vulkanInstance;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkSurfaceKHR surface;

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device;

	VkQueue graphicsQueue;
	VkQueue presentQueue;

	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkFramebuffer> swapChainFramebuffers;

	VkRenderPass graphicsRenderPass;
	VkDescriptorSetLayout graphicsDescriptorSetLayout;
	VkPipelineLayout graphicsPipelineLayout;
	VkPipeline graphicsPipeline;

	VkDescriptorSetLayout computeDescriptorSetLayout;
	VkDescriptorSetLayout computeBufferDescriptorSetLayout;
	VkPipelineLayout computePipelineLayout;
	VkPipelineLayout computeBufferPipelineLayout;

	VkPipeline computeSierpinskiTrianglePipeline;
	VkPipeline computeSierpinskiPyramidPipeline;
	VkPipeline computeSierpinskiCarpetPipeline;
	VkPipeline computeSierpinskiCubePipeline;
	VkPipeline computeMandelbrotPipeline;
	VkPipeline computeJuliaPipeline;
	// TODO: RAY TRACING
	// VkPipeline computeSierpinskiCubeRTPipeline;

	VkCommandPool commandPool;

	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
	VkSampler textureSampler;

	std::vector<VkBuffer> graphicsUniformBuffers;
	std::vector<VkDeviceMemory> graphicsUniformBuffersMemory;
	std::vector<void *> graphicsUniformBuffersMapped;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	const VkFormat computeImageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
	std::vector<VkImage> computeStorageImage;
	std::vector<VkDeviceMemory> computeStorageImageMemory;
	std::vector<VkImageView> computeStorageImageView;

	VkBuffer pointBuffer;
	VkDeviceMemory pointBufferMemory;

	std::array<float, 2> lo_xy;
	std::array<float, 2> hi_xy;
	std::array<float, 2> julia_c;
	int subdivisionCount = 0;
	bool autoRotate = false;
	float theta = 0;
	float phi = 0;
	/*
	float defaultLifetime = 1.0f;
	float lifetimeJiggle = 0.1f;
	*/
	std::vector<VkBuffer> computeUniformBuffers;
	std::vector<VkDeviceMemory> computeUniformBuffersMemory;
	std::vector<void *> computeUniformBuffersMapped;

	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> graphicsDescriptorSets;

	std::vector<VkDescriptorSet> computeDescriptorSets;

	std::vector<VkCommandBuffer> commandBuffers;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;

	uint32_t currentFrame = 0;
	bool framebufferResized = false;

	// NEW IMGUI RenderPass
	VkRenderPass imguiRenderPass;
	std::vector<VkFramebuffer> imguiFramebuffers;

	VkCommandPool imguiCommandPool;
	std::vector<VkCommandBuffer> imguiCommandBuffers;

	int currFractal = 0;
	const int SIERPINSKI_TRIANGLE = 0;
	const int SIERPINSKI_PYRAMID = 1;
	const int SIERPINSKI_CARPET = 2;
	const int SIERPINSKI_CUBE = 3;
	const int MANDELBROT = 4;
	const int JULIA = 5;

	// bool shouldClear[2]{false, false};
	const int MAX_VERTEX_COUNT = 8 * pow(20, 4);
	const int MAX_INDEX_COUNT = 36 * pow(20, 4);
	/*
	const std::vector<Vertex> vertices = {
		{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
		{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
		{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
		{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

		{{-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
		{{0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
		{{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
		{{-0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}};
	const std::vector<uint32_t> indices = {0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4};
	*/

	// FUNCTIONS:
	static void framebufferResizeCallback(GLFWwindow *window, int width,
										  int height);

	void createVulkanInstance();
	void pickPhysicalDevice();
	bool isDeviceSuitable(VkPhysicalDevice device);
	void createLogicalDevice();
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	bool comparePhysicalDevices(VkPhysicalDevice device0,
								VkPhysicalDevice device1);
	std::vector<const char *> getRequiredExtensions();
	void populateDebugMessengerCreateInfo(
		VkDebugUtilsMessengerCreateInfoEXT &createInfo);
	void setupDebugMessenger();
	bool checkValidationLayerSupport();
	static VKAPI_ATTR VkBool32 VKAPI_CALL
	debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
				  VkDebugUtilsMessageTypeFlagsEXT messageType,
				  const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
				  void *pUserData);

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
					  VkMemoryPropertyFlags properties, VkBuffer &buffer,
					  VkDeviceMemory &bufferMemory);
	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void copyImageToImage(VkCommandBuffer cmd, VkImage source,
						  VkImage destination, VkExtent2D srcSize,
						  VkExtent2D dstSize);
	void createVertexBuffer();
	void createIndexBuffer();
	void createComputeStorageImages();
	// void createComputeStorageBuffers();
	void createGraphicsAndComputeUniformBuffers();
	void createDescriptorPool();
	void createGraphicsDescriptorSets();
	void createComputeDescriptorSets();
	uint32_t findMemoryType(uint32_t typeFilter,
							VkMemoryPropertyFlags properties);
	void cleanupSwapChain();
	void recreateSwapChain();
	void createSyncObjects();
	void recordGraphicsPass(VkCommandBuffer commandBuffer, uint32_t imageIndex,
							uint32_t indexCount);
	void recordMainCommandBuffer(VkCommandBuffer commandBuffer,
								 uint32_t imageIndex);
	void createCommandBuffers();
	void createCommandPool();

	void createDepthResources();
	VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates,
								 VkImageTiling tiling,
								 VkFormatFeatureFlags features);
	VkFormat findDepthFormat();
	void createTextureImage();
	void createTextureImageView();
	void createTextureSampler();
	VkImageView createImageView(VkImage image, VkFormat format,
								VkImageAspectFlags aspectFlags);

	void createImage(uint32_t width, uint32_t height, VkFormat format,
					 VkImageTiling tiling, VkImageUsageFlags usage,
					 VkMemoryPropertyFlags properties, VkImage &image,
					 VkDeviceMemory &imageMemory);
	void transitionImageLayout(VkCommandBuffer command, VkImage image,
							   VkImageLayout currentLayout,
							   VkImageLayout newLayout);

	void copyBufferToImage(VkCommandBuffer cmd, VkBuffer buffer, VkImage image,
						   uint32_t width, uint32_t height);
	void createGraphicsFramebuffers();
	void createGraphicsRenderPass();
	void createGraphicsDescriptorSetLayout();
	void createComputeDescriptorSetLayout();
	void createComputeBufferDescriptorSetLayout();
	void createGraphicsPipeline();
	void createDefaultComputePipelineLayout();
	void createBufferUsageComputePipelineLayout();
	void createDefaultComputePipeline(const char *fileName,
									  VkPipeline *pipeline, bool bufferAccess);
	VkShaderModule createShaderModule(const std::vector<char> &code);
	static std::vector<char> readFile(const std::string &filename);
	void createImageViews();
	void createSurface();
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
	void createSwapChain();
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(
		const std::vector<VkSurfaceFormatKHR> &availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(
		const std::vector<VkPresentModeKHR> &availablePresentModes);

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
	void drawFrame();
	void updateUniformBuffers();

	void createUIRenderPass();
	void createUIFramebuffers();
	void createUICommandPool();
	void createUICommandBuffers();
	void recordUICommandBuffer(VkCommandBuffer uiCommandBuffer,
							   uint32_t imageIndex);
	void describeUI();
};
