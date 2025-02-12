#include "VulkanEngine.h"
#include "imgui/imgui.h"
#include <cstdint>
#include <vulkan/vulkan_core.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

VkResult createDebugUtilsMessengerEXT(
	VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
	const VkAllocationCallbacks *pAllocator,
	VkDebugUtilsMessengerEXT *pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
		instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		return VK_SUCCESS;
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void destroyDebugUtilsMessengerEXT(VkInstance instance,
								   VkDebugUtilsMessengerEXT debugMessenger,
								   const VkAllocationCallbacks *pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
		instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

void checkVKResult(VkResult result) {
	if (result != VK_SUCCESS) {
		throw std::runtime_error("OH NOBERS SOMETHING WENT WRONG");
	}
}

void VulkanEngine::glfwWindowInit() {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	// glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT,
							  "Fractal Rendering Showcase", nullptr, nullptr);
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void VulkanEngine::framebufferResizeCallback(GLFWwindow *window, int width,
											 int height) {
	auto app =
		reinterpret_cast<VulkanEngine *>(glfwGetWindowUserPointer(window));
	app->framebufferResized = true;
}

void VulkanEngine::initVulkan() {
	subdivisionCount = 0;

	createVulkanInstance();
	setupDebugMessenger();
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();
	createSwapChain();
	createImageViews();
	createGraphicsRenderPass();
	createGraphicsDescriptorSetLayout();
	createComputeDescriptorSetLayout();
	createComputeBufferDescriptorSetLayout();
	createGraphicsPipeline();

	createDefaultComputePipelineLayout();
	createBufferUsageComputePipelineLayout();
	/*
	createDefaultComputePipeline(
		"../../../../MathRenderer/shaders/decay_pass.comp.spv",
		&computeDecayPipeline);
	createDefaultComputePipeline(
		"../../../../MathRenderer/shaders/playground.comp.spv",
		&computeRenderPipeline);
	*/
	createDefaultComputePipeline(
		"../../../../MathRenderer/shaders/SierpinskiTriangle.comp.spv",
		&computeSierpinskiTrianglePipeline, true);
	createDefaultComputePipeline(
		"../../../../MathRenderer/shaders/SierpinskiPyramid.comp.spv",
		&computeSierpinskiPyramidPipeline, true);
	createDefaultComputePipeline(
		"../../../../MathRenderer/shaders/SierpinskiCarpet.comp.spv",
		&computeSierpinskiCarpetPipeline, true);
	createDefaultComputePipeline(
		"../../../../MathRenderer/shaders/SierpinskiCube.comp.spv",
		&computeSierpinskiCubePipeline, true);
	createDefaultComputePipeline(
		"../../../../MathRenderer/shaders/Mandelbrot.comp.spv",
		&computeMandelbrotPipeline, false);
	createDefaultComputePipeline(
		"../../../../MathRenderer/shaders/Julia.comp.spv",
		&computeJuliaPipeline, false);

	/*
	createDefaultComputePipeline(
		"../../../../MathRenderer/shaders/SierpinskiCubeRayTracing.comp.spv",
		&computeSierpinskiCubeRTPipeline, false);
	*/

	createCommandPool();
	createDepthResources();
	createGraphicsFramebuffers();
	createTextureImage();
	createTextureImageView();
	createTextureSampler();

	createVertexBuffer();
	createIndexBuffer();

	createComputeStorageImages();
	// createComputeStorageBuffers();
	createGraphicsAndComputeUniformBuffers();
	createDescriptorPool();
	createGraphicsDescriptorSets();
	createComputeDescriptorSets();
	createCommandBuffers();
	createSyncObjects();
}
void VulkanEngine::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
								VkMemoryPropertyFlags properties,
								VkBuffer &buffer,
								VkDeviceMemory &bufferMemory) {
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex =
		findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) !=
		VK_SUCCESS) {
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

VkCommandBuffer VulkanEngine::beginSingleTimeCommands() {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);
	return commandBuffer;
}

void VulkanEngine::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
	vkEndCommandBuffer(commandBuffer);
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void VulkanEngine::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer,
							  VkDeviceSize size) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
	endSingleTimeCommands(commandBuffer);
}

void VulkanEngine::copyImageToImage(VkCommandBuffer cmd, VkImage source,
									VkImage destination, VkExtent2D srcSize,
									VkExtent2D dstSize) {
	VkImageBlit2 blitRegion{.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
							.pNext = nullptr};

	blitRegion.srcOffsets[1].x = srcSize.width;
	blitRegion.srcOffsets[1].y = srcSize.height;
	blitRegion.srcOffsets[1].z = 1;

	blitRegion.dstOffsets[1].x = dstSize.width;
	blitRegion.dstOffsets[1].y = dstSize.height;
	blitRegion.dstOffsets[1].z = 1;

	blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.srcSubresource.baseArrayLayer = 0;
	blitRegion.srcSubresource.layerCount = 1;
	blitRegion.srcSubresource.mipLevel = 0;

	blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.dstSubresource.baseArrayLayer = 0;
	blitRegion.dstSubresource.layerCount = 1;
	blitRegion.dstSubresource.mipLevel = 0;

	VkBlitImageInfo2 blitInfo{.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
							  .pNext = nullptr};
	blitInfo.dstImage = destination;
	blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	blitInfo.srcImage = source;
	blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	blitInfo.filter = VK_FILTER_LINEAR;
	blitInfo.regionCount = 1;
	blitInfo.pRegions = &blitRegion;

	vkCmdBlitImage2(cmd, &blitInfo);
}

void VulkanEngine::createVertexBuffer() {
	VkDeviceSize bufferSize = sizeof(Vertex) * MAX_VERTEX_COUNT;

	/*
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
					 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				 stagingBuffer, stagingBufferMemory);

	void *data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(device, stagingBufferMemory);
	*/

	createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
	/*
	copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
	*/
}

void VulkanEngine::createIndexBuffer() {
	VkDeviceSize bufferSize = sizeof(uint32_t) * MAX_INDEX_COUNT;
	/*
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
					 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				 stagingBuffer, stagingBufferMemory);

	void *data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), (size_t)bufferSize);
	vkUnmapMemory(device, stagingBufferMemory);
	*/

	createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);
	/*
	copyBuffer(stagingBuffer, indexBuffer, bufferSize);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
	*/
}

void VulkanEngine::createComputeStorageImages() {
	// IMAGE INITIALIZATION
	if (computeStorageImage.size() != MAX_FRAMES_IN_FLIGHT) {
		computeStorageImage.resize(MAX_FRAMES_IN_FLIGHT);
		computeStorageImageMemory.resize(MAX_FRAMES_IN_FLIGHT);
		computeStorageImageView.resize(MAX_FRAMES_IN_FLIGHT);
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		createImage(swapChainExtent.width, swapChainExtent.height,
					computeImageFormat, VK_IMAGE_TILING_OPTIMAL,
					VK_IMAGE_USAGE_TRANSFER_DST_BIT |
						VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
						VK_IMAGE_USAGE_STORAGE_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, computeStorageImage[i],
					computeStorageImageMemory[i]);
		computeStorageImageView[i] =
			createImageView(computeStorageImage[i], computeImageFormat,
							VK_IMAGE_ASPECT_COLOR_BIT);
	}
}

/*
void VulkanEngine::createComputeStorageBuffers() {
	// BUFFER INITIALIZATION
	// Initialize particles
	std::default_random_engine rndEngine((unsigned)time(nullptr));
	std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);

	lo_xy = std::array<float, 2>{
		-1.0f * swapChainExtent.width / swapChainExtent.height, -1.0f};
	hi_xy = std::array<float, 2>{
		1.0f * swapChainExtent.width / swapChainExtent.height, 1.0f};

	std::vector<Point> points(POINT_COUNT);
	for (auto &point : points) {
		// float r = 0.5f * sqrt(rndDist(rndEngine));
		// float theta = rndDist(rndEngine) * 2.0f
		// * 3.14159265358979323846f;
		float x = rndDist(rndEngine);
		float y = rndDist(rndEngine);
		point.posr[0] = x * lo_xy[0] + (1 - x) * hi_xy[0];
		point.posr[1] = y * lo_xy[1] + (1 - y) * hi_xy[1];
		point.posr[2] = 0.0f;
		point.posr[3] = 0.01f;
		for (size_t i = 0; i < 3; ++i) {
			point.color[i] = rndDist(rndEngine);
		}
		point.color[3] = 1.0f;

		point.lifetime = defaultLifetime + rndDist(rndEngine) * lifetimeJiggle;
	}

	VkDeviceSize bufferSize = sizeof(Point) * POINT_COUNT;
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
					 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				 stagingBuffer, stagingBufferMemory);

	void *data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, points.data(), (size_t)bufferSize);
	vkUnmapMemory(device, stagingBufferMemory);

	createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, pointBuffer, pointBufferMemory);
	copyBuffer(stagingBuffer, pointBuffer, bufferSize);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}
*/

void VulkanEngine::createGraphicsAndComputeUniformBuffers() {
	// Graphics
	VkDeviceSize graphicsBufferSize = sizeof(GraphicsUniformBufferObject);
	graphicsUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	graphicsUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
	graphicsUniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		createBuffer(graphicsBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
					 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
						 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					 graphicsUniformBuffers[i],
					 graphicsUniformBuffersMemory[i]);
		vkMapMemory(device, graphicsUniformBuffersMemory[i], 0,
					graphicsBufferSize, 0, &graphicsUniformBuffersMapped[i]);
	}

	// Compute
	VkDeviceSize computeBufferSize = sizeof(ComputeUniformBufferObject);
	computeUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	computeUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
	computeUniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		createBuffer(computeBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
					 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
						 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					 computeUniformBuffers[i], computeUniformBuffersMemory[i]);
		vkMapMemory(device, computeUniformBuffersMemory[i], 0,
					computeBufferSize, 0, &computeUniformBuffersMapped[i]);
	}
}

void VulkanEngine::createDescriptorPool() {
	std::array<VkDescriptorPoolSize, 4> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	/*
	poolSizes[1].descriptorCount =
		static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) +
		IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE;
		*/
	// TODO: Make this good
	poolSizes[1].descriptorCount = 1000;

	poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[2].descriptorCount = 17;

	poolSizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	poolSizes[3].descriptorCount = 17;

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	/*
	poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) +
					   IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE;
					*/
	// TODO: Why not :shrug:
	poolInfo.maxSets = 1000;
	// INCLUDED IN IMGUI VULKAN EXAMPLE:
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) !=
		VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor pool");
	}
}

void VulkanEngine::createGraphicsDescriptorSets() {
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
											   graphicsDescriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocInfo.pSetLayouts = layouts.data();

	graphicsDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(device, &allocInfo,
								 graphicsDescriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor sets!");
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = graphicsUniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(
			GraphicsUniformBufferObject); // Since we're overwriting everything,
										  // we may also write VK_WHOLE_SIZE

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = textureImageView;
		imageInfo.sampler = textureSampler;

		std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = graphicsDescriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = graphicsDescriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType =
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(device,
							   static_cast<uint32_t>(descriptorWrites.size()),
							   descriptorWrites.data(), 0, nullptr);
	}
}

void VulkanEngine::createComputeDescriptorSets() {
	/*
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
											   computeDescriptorSetLayout);
											   */
	std::vector<VkDescriptorSetLayout> layouts{
		computeDescriptorSetLayout, computeDescriptorSetLayout,
		computeBufferDescriptorSetLayout};

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
	allocInfo.pSetLayouts = layouts.data();

	computeDescriptorSets.resize(layouts.size());
	if (vkAllocateDescriptorSets(device, &allocInfo,
								 computeDescriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor sets!");
	}

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		VkDescriptorBufferInfo uniformBufferInfo{};
		uniformBufferInfo.buffer = computeUniformBuffers[i];
		uniformBufferInfo.offset = 0;
		uniformBufferInfo.range = sizeof(ComputeUniformBufferObject);

		VkWriteDescriptorSet uboDescriptorWrite{};
		uboDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		uboDescriptorWrite.dstSet = computeDescriptorSets[i];
		uboDescriptorWrite.dstBinding = 0;
		uboDescriptorWrite.dstArrayElement = 0;
		uboDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboDescriptorWrite.descriptorCount = 1;
		uboDescriptorWrite.pBufferInfo = &uniformBufferInfo;

		VkDescriptorImageInfo imgInfo{};
		imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		imgInfo.imageView = computeStorageImageView[i];

		VkWriteDescriptorSet imageDescriptorWrite{};
		imageDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		imageDescriptorWrite.dstSet = computeDescriptorSets[i];
		imageDescriptorWrite.dstBinding = 1;
		imageDescriptorWrite.descriptorCount = 1;
		imageDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		imageDescriptorWrite.pImageInfo = &imgInfo;

		/*
		VkDescriptorBufferInfo pointBufferInfo{};
		pointBufferInfo.buffer = pointBuffer;
		pointBufferInfo.offset = 0;
		pointBufferInfo.range = sizeof(Point) * POINT_COUNT;

		VkWriteDescriptorSet pointDescriptorWrite{};
		pointDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		pointDescriptorWrite.dstSet = computeDescriptorSets[i];
		pointDescriptorWrite.dstBinding = 2;
		pointDescriptorWrite.dstArrayElement = 0;
		pointDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		pointDescriptorWrite.descriptorCount = 1;
		pointDescriptorWrite.pBufferInfo = &pointBufferInfo;
		*/

		std::array<VkWriteDescriptorSet, 2> descriptorWrites{
			uboDescriptorWrite, imageDescriptorWrite};
		vkUpdateDescriptorSets(device,
							   static_cast<uint32_t>(descriptorWrites.size()),
							   descriptorWrites.data(), 0, nullptr);
	}

	VkDescriptorBufferInfo vertexBufferInfo{};
	vertexBufferInfo.buffer = vertexBuffer;
	vertexBufferInfo.offset = 0;
	vertexBufferInfo.range = MAX_VERTEX_COUNT * sizeof(Vertex);

	VkWriteDescriptorSet vertexDescriptorWrite{};
	vertexDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	vertexDescriptorWrite.dstSet = computeDescriptorSets[2];
	vertexDescriptorWrite.dstBinding = 0;
	vertexDescriptorWrite.dstArrayElement = 0;
	vertexDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	vertexDescriptorWrite.descriptorCount = 1;
	vertexDescriptorWrite.pBufferInfo = &vertexBufferInfo;

	VkDescriptorBufferInfo indexBufferInfo{};
	indexBufferInfo.buffer = indexBuffer;
	indexBufferInfo.offset = 0;
	indexBufferInfo.range = MAX_INDEX_COUNT * sizeof(uint32_t);

	VkWriteDescriptorSet indexDescriptorWrite{};
	indexDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	indexDescriptorWrite.dstSet = computeDescriptorSets[2];
	indexDescriptorWrite.dstBinding = 1;
	indexDescriptorWrite.dstArrayElement = 0;
	indexDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	indexDescriptorWrite.descriptorCount = 1;
	indexDescriptorWrite.pBufferInfo = &indexBufferInfo;

	std::array<VkWriteDescriptorSet, 2> descriptorWrites{vertexDescriptorWrite,
														 indexDescriptorWrite};
	vkUpdateDescriptorSets(device,
						   static_cast<uint32_t>(descriptorWrites.size()),
						   descriptorWrites.data(), 0, nullptr);
}

uint32_t VulkanEngine::findMemoryType(uint32_t typeFilter,
									  VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
		if ((typeFilter & (1 << i)) &&
			(memProperties.memoryTypes[i].propertyFlags & properties)) {
			return i;
		}
	}

	throw std::runtime_error("Failed to find suitable memory type");
}

void VulkanEngine::cleanupSwapChain() {
	vkDestroyImageView(device, depthImageView, nullptr);
	vkDestroyImage(device, depthImage, nullptr);
	vkFreeMemory(device, depthImageMemory, nullptr);

	for (size_t i = 0; i < computeStorageImage.size(); ++i) {
		vkDestroyImage(device, computeStorageImage[i], nullptr);
		vkDestroyImageView(device, computeStorageImageView[i], nullptr);
		vkFreeMemory(device, computeStorageImageMemory[i], nullptr);
	}

	for (size_t i = 0; i < imguiFramebuffers.size(); ++i) {
		vkDestroyFramebuffer(device, swapChainFramebuffers[i], nullptr);
		vkDestroyFramebuffer(device, imguiFramebuffers[i], nullptr);
	}

	for (VkImageView &swapChainImageView : swapChainImageViews) {
		vkDestroyImageView(device, swapChainImageView, nullptr);
	}

	vkDestroySwapchainKHR(device, swapChain, nullptr);
}

void VulkanEngine::recreateSwapChain() {
	std::cout << "Swap chain recreation \n";
	int width = 0, height = 0;
	glfwGetFramebufferSize(window, &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(device);

	for (int f = 0; f < MAX_FRAMES_IN_FLIGHT; ++f) {
		vkWaitForFences(device, 1, &inFlightFences[f], VK_TRUE, UINT64_MAX);
	}
	cleanupSwapChain();

	createSwapChain();
	createImageViews();

	createComputeStorageImages();
	createComputeDescriptorSets();
	createDepthResources();
	createGraphicsFramebuffers();

	// NEW UPDATE FOR IMGUI:
	createUIFramebuffers();
	ImGui_ImplVulkan_SetMinImageCount(swapChainImages.size());
}

void VulkanEngine::createSyncObjects() {
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr,
							  &imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(device, &semaphoreInfo, nullptr,
							  &renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) !=
				VK_SUCCESS) {
			throw std::runtime_error("Failed to create sync objects");
		}
	}
}

void VulkanEngine::recordGraphicsPass(VkCommandBuffer commandBuffer,
									  uint32_t imageIndex,
									  uint32_t indexCount) {
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = graphicsRenderPass;
	renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = swapChainExtent;

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
	clearValues[1].depthStencil = {1.0f, 0};
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
						 VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
					  graphicsPipeline);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)swapChainExtent.width;
	viewport.height = (float)swapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = swapChainExtent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	VkBuffer vertexBuffers[] = {vertexBuffer};
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);

	vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
							graphicsPipelineLayout, 0, 1,
							&graphicsDescriptorSets[currentFrame], 0, nullptr);

	vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);

	vkCmdEndRenderPass(commandBuffer);
}

void VulkanEngine::recordMainCommandBuffer(VkCommandBuffer commandBuffer,
										   uint32_t imageIndex) {
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;
	beginInfo.pInheritanceInfo = nullptr;

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("Failed to begin recording command buffer");
	}

	transitionImageLayout(commandBuffer, computeStorageImage[currentFrame],
						  VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	if (currFractal == SIERPINSKI_TRIANGLE) {
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
						  computeSierpinskiTrianglePipeline);
		std::array<VkDescriptorSet, 2> descriptorSets{
			computeDescriptorSets[currentFrame], computeDescriptorSets[2]};
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
								computeBufferPipelineLayout, 0, 2,
								descriptorSets.data(), 0, nullptr);
		vkCmdDispatch(commandBuffer, (pow(3, subdivisionCount) + 63) / 64, 1,
					  1);

		VkMemoryBarrier2 memBarrier = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT,
			.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT,

		};
		VkDependencyInfo depInfo = {.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
									.memoryBarrierCount = 1,
									.pMemoryBarriers = &memBarrier};
		vkCmdPipelineBarrier2(commandBuffer, &depInfo);

		transitionImageLayout(commandBuffer, swapChainImages[imageIndex],
							  VK_IMAGE_LAYOUT_UNDEFINED,
							  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		recordGraphicsPass(commandBuffer, imageIndex,
						   pow(3, subdivisionCount + 1));

	} else if (currFractal == SIERPINSKI_PYRAMID) {
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
						  computeSierpinskiPyramidPipeline);
		std::array<VkDescriptorSet, 2> descriptorSets{
			computeDescriptorSets[currentFrame], computeDescriptorSets[2]};
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
								computeBufferPipelineLayout, 0, 2,
								descriptorSets.data(), 0, nullptr);
		vkCmdDispatch(commandBuffer, (pow(4, subdivisionCount) + 63) / 64, 1,
					  1);

		VkMemoryBarrier2 memBarrier = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT,
			.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT,

		};
		VkDependencyInfo depInfo = {.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
									.memoryBarrierCount = 1,
									.pMemoryBarriers = &memBarrier};
		vkCmdPipelineBarrier2(commandBuffer, &depInfo);

		transitionImageLayout(commandBuffer, swapChainImages[imageIndex],
							  VK_IMAGE_LAYOUT_UNDEFINED,
							  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		recordGraphicsPass(commandBuffer, imageIndex,
						   12 * pow(4, subdivisionCount));
	} else if (currFractal == SIERPINSKI_CARPET) {
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
						  computeSierpinskiCarpetPipeline);
		std::array<VkDescriptorSet, 2> descriptorSets{
			computeDescriptorSets[currentFrame], computeDescriptorSets[2]};
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
								computeBufferPipelineLayout, 0, 2,
								descriptorSets.data(), 0, nullptr);
		vkCmdDispatch(commandBuffer, ((1 << (3 * subdivisionCount)) + 63) / 64,
					  1, 1);

		VkMemoryBarrier2 memBarrier = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT,
			.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT,

		};
		VkDependencyInfo depInfo = {.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
									.memoryBarrierCount = 1,
									.pMemoryBarriers = &memBarrier};
		vkCmdPipelineBarrier2(commandBuffer, &depInfo);

		transitionImageLayout(commandBuffer, swapChainImages[imageIndex],
							  VK_IMAGE_LAYOUT_UNDEFINED,
							  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		recordGraphicsPass(commandBuffer, imageIndex,
						   6 * (1 << 3 * subdivisionCount));
	} else if (currFractal == SIERPINSKI_CUBE) {
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
						  computeSierpinskiCubePipeline);
		std::array<VkDescriptorSet, 2> descriptorSets{
			computeDescriptorSets[currentFrame], computeDescriptorSets[2]};
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
								computeBufferPipelineLayout, 0, 2,
								descriptorSets.data(), 0, nullptr);
		vkCmdDispatch(commandBuffer, pow(20, subdivisionCount), 1, 1);

		VkMemoryBarrier2 memBarrier = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT,
			.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT,

		};
		VkDependencyInfo depInfo = {.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
									.memoryBarrierCount = 1,
									.pMemoryBarriers = &memBarrier};
		vkCmdPipelineBarrier2(commandBuffer, &depInfo);

		transitionImageLayout(commandBuffer, swapChainImages[imageIndex],
							  VK_IMAGE_LAYOUT_UNDEFINED,
							  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		recordGraphicsPass(commandBuffer, imageIndex,
						   36 * pow(20, subdivisionCount));
	} else if (currFractal == MANDELBROT) {
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
						  computeMandelbrotPipeline);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
								computePipelineLayout, 0, 1,
								&computeDescriptorSets[currentFrame], 0,
								nullptr);
		vkCmdDispatch(
			commandBuffer,
			static_cast<uint32_t>(std::ceil(swapChainExtent.width / 16.0f)),
			static_cast<uint32_t>(std::ceil(swapChainExtent.height / 16.0f)),
			1);

		transitionImageLayout(commandBuffer, computeStorageImage[currentFrame],
							  VK_IMAGE_LAYOUT_GENERAL,
							  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		transitionImageLayout(commandBuffer, swapChainImages[imageIndex],
							  VK_IMAGE_LAYOUT_UNDEFINED,
							  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		copyImageToImage(commandBuffer, computeStorageImage[currentFrame],
						 swapChainImages[imageIndex], swapChainExtent,
						 swapChainExtent);
		transitionImageLayout(commandBuffer, swapChainImages[imageIndex],
							  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
							  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	} else if (currFractal == JULIA) {
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
						  computeJuliaPipeline);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
								computePipelineLayout, 0, 1,
								&computeDescriptorSets[currentFrame], 0,
								nullptr);
		vkCmdDispatch(
			commandBuffer,
			static_cast<uint32_t>(std::ceil(swapChainExtent.width / 16.0f)),
			static_cast<uint32_t>(std::ceil(swapChainExtent.height / 16.0f)),
			1);

		transitionImageLayout(commandBuffer, computeStorageImage[currentFrame],
							  VK_IMAGE_LAYOUT_GENERAL,
							  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		transitionImageLayout(commandBuffer, swapChainImages[imageIndex],
							  VK_IMAGE_LAYOUT_UNDEFINED,
							  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		copyImageToImage(commandBuffer, computeStorageImage[currentFrame],
						 swapChainImages[imageIndex], swapChainExtent,
						 swapChainExtent);
		transitionImageLayout(commandBuffer, swapChainImages[imageIndex],
							  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
							  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	} /*else if (currFractal == SIERPINSKI_CUBE_RT) {
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
						  computeSierpinskiCubeRTPipeline);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
								computePipelineLayout, 0, 1,
								&computeDescriptorSets[currentFrame], 0,
								nullptr);
		vkCmdDispatch(
			commandBuffer,
			static_cast<uint32_t>(std::ceil(swapChainExtent.width / 8.0f)),
			static_cast<uint32_t>(std::ceil(swapChainExtent.height / 8.0f)), 1);

		transitionImageLayout(commandBuffer, computeStorageImage[currentFrame],
							  VK_IMAGE_LAYOUT_GENERAL,
							  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		transitionImageLayout(commandBuffer, swapChainImages[imageIndex],
							  VK_IMAGE_LAYOUT_UNDEFINED,
							  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		copyImageToImage(commandBuffer, computeStorageImage[currentFrame],
						 swapChainImages[imageIndex], swapChainExtent,
						 swapChainExtent);
		transitionImageLayout(commandBuffer, swapChainImages[imageIndex],
							  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
							  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	} else {
		VkClearColorValue clearValue;
		clearValue = {{0.0f, 0.0f, 0.0f, 1.0f}};
		VkImageSubresourceRange subResourceRange{};
		subResourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subResourceRange.baseMipLevel = 0;
		subResourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		subResourceRange.baseArrayLayer = 0;
		subResourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
		vkCmdClearColorImage(commandBuffer, computeStorageImage[currentFrame],
							 VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1,
							 &subResourceRange);
		// shouldClear[currentFrame] = false;
		transitionImageLayout(commandBuffer, swapChainImages[imageIndex],
							  VK_IMAGE_LAYOUT_UNDEFINED,
							  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	}*/

	/*
	VkImageMemoryBarrier2 imageMemoryBarrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR,
		.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT_KHR,
		.dstAccessMask =
			VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.image = swapChainImages[imageIndex],
		.subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
							 .baseMipLevel = 0,
							 .levelCount = 1,
							 .baseArrayLayer = 0,
							 .layerCount = 1}};
	VkDependencyInfoKHR dependencyInfo = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.pNext = nullptr,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &imageMemoryBarrier};
	vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
	// BEGIN NEW GRAPHICS COMMANDS
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = graphicsRenderPass;
	renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = swapChainExtent;

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
	clearValues[1].depthStencil = {1.0f, 0};
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
						 VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
					  graphicsPipeline);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)swapChainExtent.width;
	viewport.height = (float)swapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = swapChainExtent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	VkBuffer vertexBuffers[] = {vertexBuffer};
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

	vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
							graphicsPipelineLayout, 0, 1,
							&graphicsDescriptorSets[currentFrame], 0, nullptr);

	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0,
					 0, 0);

	vkCmdEndRenderPass(commandBuffer);
	// END NEW GRAPHICS COMMANDS
	*/

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("FAILED TO RECORD COMMAND BUFFER");
	}
}

void VulkanEngine::createCommandBuffers() {
	commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

	if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) !=
		VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate Command Buffers");
	}
}

void VulkanEngine::createCommandPool() {
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex =
		queueFamilyIndices.graphicsAndComputeFamily.value();

	if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) !=
		VK_SUCCESS) {
		throw std::runtime_error("Failed to create Command Pool");
	}
}

void VulkanEngine::createDepthResources() {
	VkFormat depthFormat = findDepthFormat();
	createImage(
		swapChainExtent.width, swapChainExtent.height, depthFormat,
		VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);

	depthImageView =
		createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}

VkFormat
VulkanEngine::findSupportedFormat(const std::vector<VkFormat> &candidates,
								  VkImageTiling tiling,
								  VkFormatFeatureFlags features) {
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR &&
			(props.linearTilingFeatures & features) == features) {
			return format;
		} else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
				   (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format!");
}

VkFormat VulkanEngine::findDepthFormat() {
	return findSupportedFormat({VK_FORMAT_D32_SFLOAT,
								VK_FORMAT_D32_SFLOAT_S8_UINT,
								VK_FORMAT_D24_UNORM_S8_UINT},
							   VK_IMAGE_TILING_OPTIMAL,
							   VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void VulkanEngine::createTextureImage() {
	int texWidth, texHeight, texChannels;
	stbi_uc *pixels =
		stbi_load("../../../../MathRenderer/textures/pi.png", &texWidth,
				  &texHeight, &texChannels, STBI_rgb_alpha);

	VkDeviceSize imageSize = texWidth * texHeight * 4;
	if (!pixels) {
		throw std::runtime_error("Failed to load texture image");
	}

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
					 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				 stagingBuffer, stagingBufferMemory);

	void *data;
	vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(device, stagingBufferMemory);

	stbi_image_free(pixels);

	createImage(
		texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);

	VkCommandBuffer cmd = beginSingleTimeCommands();
	transitionImageLayout(cmd, textureImage, VK_IMAGE_LAYOUT_UNDEFINED,
						  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copyBufferToImage(cmd, stagingBuffer, textureImage,
					  static_cast<uint32_t>(texWidth),
					  static_cast<uint32_t>(texHeight));
	transitionImageLayout(cmd, textureImage,
						  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	endSingleTimeCommands(cmd);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void VulkanEngine::createTextureImageView() {
	textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB,
									   VK_IMAGE_ASPECT_COLOR_BIT);
}

void VulkanEngine::createTextureSampler() {
	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(physicalDevice, &properties);

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	samplerInfo.anisotropyEnable = VK_TRUE;

	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) !=
		VK_SUCCESS) {
		throw std::runtime_error("Failed to create texture sampler!");
	}
}

VkImageView VulkanEngine::createImageView(VkImage image, VkFormat format,
										  VkImageAspectFlags aspectFlags) {
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) !=
		VK_SUCCESS) {
		throw std::runtime_error("Failed to create texture image view");
	}
	return imageView;
}

void VulkanEngine::createImage(uint32_t width, uint32_t height, VkFormat format,
							   VkImageTiling tiling, VkImageUsageFlags usage,
							   VkMemoryPropertyFlags properties, VkImage &image,
							   VkDeviceMemory &imageMemory) {
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	// imageInfo.flags = 0;

	if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create image");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex =
		findMemoryType(memRequirements.memoryTypeBits, properties);
	if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) !=
		VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate image memory");
	}

	vkBindImageMemory(device, image, imageMemory, 0);
}

void VulkanEngine::transitionImageLayout(VkCommandBuffer commandBuffer,
										 VkImage image,
										 VkImageLayout currentLayout,
										 VkImageLayout newLayout) {
	// VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	/*
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
		newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
			   newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else {
		throw std::invalid_argument("Unsupported layout transition!");
	}
	endSingleTimeCommands(commandBuffer);
	*/
	VkImageMemoryBarrier2 imageBarrier{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
	imageBarrier.pNext = nullptr;
	imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
	imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	imageBarrier.dstAccessMask =
		VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
	imageBarrier.oldLayout = currentLayout;
	imageBarrier.newLayout = newLayout;
	imageBarrier.image = image;

	VkImageAspectFlags aspectMask =
		(newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
			? VK_IMAGE_ASPECT_DEPTH_BIT
			: VK_IMAGE_ASPECT_COLOR_BIT;

	// imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageBarrier.subresourceRange.aspectMask = aspectMask;
	imageBarrier.subresourceRange.baseMipLevel = 0;
	imageBarrier.subresourceRange.levelCount = 1;
	imageBarrier.subresourceRange.baseArrayLayer = 0;
	imageBarrier.subresourceRange.layerCount = 1;

	VkDependencyInfo depInfo{};
	depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	depInfo.pNext = nullptr;

	depInfo.imageMemoryBarrierCount = 1;
	depInfo.pImageMemoryBarriers = &imageBarrier;

	vkCmdPipelineBarrier2(commandBuffer, &depInfo);
}

void VulkanEngine::copyBufferToImage(VkCommandBuffer cmd, VkBuffer buffer,
									 VkImage image, uint32_t width,
									 uint32_t height) {

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = {0, 0, 0};
	region.imageExtent = {width, height, 1};

	vkCmdCopyBufferToImage(cmd, buffer, image,
						   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void VulkanEngine::createGraphicsFramebuffers() {
	swapChainFramebuffers.resize(swapChainImageViews.size());

	for (size_t i = 0; i < swapChainImageViews.size(); ++i) {
		std::array<VkImageView, 2> attachments = {swapChainImageViews[i],
												  depthImageView};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = graphicsRenderPass;
		framebufferInfo.attachmentCount =
			static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr,
								&swapChainFramebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}

void VulkanEngine::createGraphicsRenderPass() {
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

	// colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	// colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = findDepthFormat();
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout =
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout =
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
							  VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
							  VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
							   VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 2> attachments = {colorAttachment,
														  depthAttachment};
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(device, &renderPassInfo, nullptr,
						   &graphicsRenderPass) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create render pass!");
	}
}
void VulkanEngine::createGraphicsDescriptorSetLayout() {
	std::array<VkDescriptorSetLayoutBinding, 2> bindings;
	bindings[0].binding = 0;
	bindings[0].descriptorCount = 1;
	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindings[0].pImmutableSamplers = nullptr;
	bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	bindings[1].binding = 1;
	bindings[1].descriptorCount = 1;
	bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindings[1].pImmutableSamplers = nullptr;
	bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr,
									&graphicsDescriptorSetLayout) !=
		VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor set layout");
	}
}

void VulkanEngine::createComputeDescriptorSetLayout() {
	std::array<VkDescriptorSetLayoutBinding, 2> bindings;
	bindings[0].binding = 0;
	bindings[0].descriptorCount = 1;
	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindings[0].pImmutableSamplers = nullptr;
	bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	/*
	bindings[1].binding = 1;
	bindings[1].descriptorCount = 1;
	bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bindings[1].pImmutableSamplers = nullptr;
	bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	*/

	bindings[1].binding = 1;
	bindings[1].descriptorCount = 1;
	bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	bindings[1].pImmutableSamplers = nullptr;
	bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr,
									&computeDescriptorSetLayout) !=
		VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor set layout");
	}
}

void VulkanEngine::createComputeBufferDescriptorSetLayout() {
	std::array<VkDescriptorSetLayoutBinding, 2> bindings;
	bindings[0].binding = 0;
	bindings[0].descriptorCount = 1;
	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bindings[0].pImmutableSamplers = nullptr;
	bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	bindings[1].binding = 1;
	bindings[1].descriptorCount = 1;
	bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bindings[1].pImmutableSamplers = nullptr;
	bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr,
									&computeBufferDescriptorSetLayout) !=
		VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor set layout");
	}
}

void VulkanEngine::createGraphicsPipeline() {
	auto vertShaderCode = readFile("../../../../MathRenderer/shaders/vert.spv");
	auto fragShaderCode = readFile("../../../../MathRenderer/shaders/frag.spv");

	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType =
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType =
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo,
													  fragShaderStageInfo};

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType =
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount =
		static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType =
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)swapChainExtent.width;
	viewport.height = (float)swapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.f;

	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = swapChainExtent;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType =
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	// TODO:????
	// rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	// rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f;		   // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f;	   // Optional

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType =
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType =
		VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f;
	depthStencil.maxDepthBounds = 1.0f;
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {};
	depthStencil.back = {};

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType =
		VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	// Dynamic States
	std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
												 VK_DYNAMIC_STATE_SCISSOR};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount =
		static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &graphicsDescriptorSetLayout;
	// pipelineLayoutInfo.pushConstantRangeCount = 0;	  // Optional
	// pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr,
							   &graphicsPipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create pipeline layout!");
	}

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = graphicsPipelineLayout;
	pipelineInfo.renderPass = graphicsRenderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1;			  // Optional

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
								  nullptr, &graphicsPipeline) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(device, vertShaderModule, nullptr);
	vkDestroyShaderModule(device, fragShaderModule, nullptr);
}

void VulkanEngine::createDefaultComputePipelineLayout() {
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &computeDescriptorSetLayout;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr,
							   &computePipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create compute pipeline layout!");
	}
}
void VulkanEngine::createBufferUsageComputePipelineLayout() {
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 2;
	VkDescriptorSetLayout layouts[2] = {computeDescriptorSetLayout,
										computeBufferDescriptorSetLayout};
	pipelineLayoutInfo.pSetLayouts = &layouts[0];

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr,
							   &computeBufferPipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create compute pipeline layout!");
	}
}

void VulkanEngine::createDefaultComputePipeline(const char *fileName,
												VkPipeline *pipeline,
												bool bufferAccess) {
	// auto computeShaderCode = readFile("shaders/gradient.comp.spv");
	auto computeShaderCode = readFile(fileName);

	VkShaderModule computeShaderModule = createShaderModule(computeShaderCode);

	VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
	computeShaderStageInfo.sType =
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	computeShaderStageInfo.module = computeShaderModule;
	computeShaderStageInfo.pName = "main";

	VkComputePipelineCreateInfo computePipelineCreateInfo{};
	computePipelineCreateInfo.sType =
		VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineCreateInfo.layout =
		bufferAccess ? computeBufferPipelineLayout : computePipelineLayout;
	computePipelineCreateInfo.stage = computeShaderStageInfo;

	if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1,
								 &computePipelineCreateInfo, nullptr,
								 pipeline) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create compute pipeline");
	}

	vkDestroyShaderModule(device, computeShaderModule, nullptr);
}

VkShaderModule VulkanEngine::createShaderModule(const std::vector<char> &code) {
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) !=
		VK_SUCCESS) {
		throw std::runtime_error("Failed to create shader module");
	}

	return shaderModule;
}

std::vector<char> VulkanEngine::readFile(const std::string &filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

void VulkanEngine::createImageViews() {
	swapChainImageViews.resize(swapChainImages.size());

	for (uint32_t i = 0; i < swapChainImages.size(); ++i) {
		swapChainImageViews[i] =
			createImageView(swapChainImages[i], swapChainImageFormat,
							VK_IMAGE_ASPECT_COLOR_BIT);
	}
}

void VulkanEngine::createSurface() {
	if (glfwCreateWindowSurface(vulkanInstance, window, nullptr, &surface) !=
		VK_SUCCESS) {
		throw std::runtime_error("Failed to create window surface");
	}
}
QueueFamilyIndices VulkanEngine::findQueueFamilies(VkPhysicalDevice device) {
	QueueFamilyIndices indices;

	// Assign index to queue families that could be found
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
											 nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
											 queueFamilies.data());

	int i = 0;
	for (const auto &queueFamily : queueFamilies) {
		if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
			(queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
			indices.graphicsAndComputeFamily = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface,
											 &presentSupport);
		if (presentSupport) {
			indices.presentFamily = i;
		}

		if (indices.isComplete()) {
			break;
		}
		++i;
	}

	return indices;
}

void VulkanEngine::createSwapChain() {
	SwapChainSupportDetails swapChainSupport =
		querySwapChainSupport(physicalDevice);

	VkSurfaceFormatKHR surfaceFormat =
		chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode =
		chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 &&
		imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage =
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
	uint32_t queueFamilyIndices[] = {indices.graphicsAndComputeFamily.value(),
									 indices.presentFamily.value()};

	if (indices.graphicsAndComputeFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
		// createInfo.queueFamilyIndexCount = 0;
		// createInfo.pQueueFamilyIndices = nullptr;
	} else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) !=
		VK_SUCCESS) {
		throw std::runtime_error("Failed to create swap chain!");
	}

	// Must requery imageCount as we only specify a minimum
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount,
							swapChainImages.data());

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;
}

SwapChainSupportDetails
VulkanEngine::querySwapChainSupport(VkPhysicalDevice device) {
	SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface,
											  &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount,
										 nullptr);
	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount,
											 details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface,
											  &presentModeCount, nullptr);

	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(
			device, surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

VkSurfaceFormatKHR VulkanEngine::chooseSwapSurfaceFormat(
	const std::vector<VkSurfaceFormatKHR> &availableFormats) {
	for (const auto &availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
			availableFormat.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}

	// Could instead search for second-best but who cares :shrug:
	return availableFormats[0];
}

VkPresentModeKHR VulkanEngine::chooseSwapPresentMode(
	const std::vector<VkPresentModeKHR> &availablePresentModes) {
	for (const auto &availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentMode;
		}
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D
VulkanEngine::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) {
	if (capabilities.currentExtent.width !=
		std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	} else {
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		VkExtent2D actualExtent = {static_cast<uint32_t>(width),
								   static_cast<uint32_t>(height)};

		actualExtent.width =
			std::clamp(actualExtent.width, capabilities.minImageExtent.width,
					   capabilities.maxImageExtent.width);
		actualExtent.height =
			std::clamp(actualExtent.height, capabilities.minImageExtent.height,
					   capabilities.maxImageExtent.height);

		return actualExtent;
	}
}

void VulkanEngine::createLogicalDevice() {
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

	// VkDeviceQueueCreateInfo queueCreateInfo{};
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies{
		indices.graphicsAndComputeFamily.value(),
		indices.presentFamily.value()};
	float queuePriority = 1.0f;

	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;

		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceVulkan13Features features13 = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
	features13.synchronization2 = VK_TRUE;
	VkPhysicalDeviceFeatures2 physicalFeatures2 = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
	physicalFeatures2.pNext = &features13;

	vkGetPhysicalDeviceFeatures2(physicalDevice, &physicalFeatures2);
	if (features13.synchronization2 == VK_FALSE) {
		throw std::runtime_error("SYNC 2 NOT SUPPORTED :(");
	}

	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo createInfo{};
	createInfo.pNext = &physicalFeatures2;
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount =
		static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures =
		nullptr; // NOT &deviceFeatures (we never use that)

	createInfo.enabledExtensionCount =
		static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	if (enableValidationLayers) {
		createInfo.enabledLayerCount =
			static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	} else {
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) !=
		VK_SUCCESS) {
		throw std::runtime_error("Failed to create logical device!");
	}

	vkGetDeviceQueue(device, indices.graphicsAndComputeFamily.value(), 0,
					 &graphicsQueue);
	vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

void VulkanEngine::pickPhysicalDevice() {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, nullptr);
	if (deviceCount == 0) {
		throw std::runtime_error("No GPUs with Vulkan support found");
	}
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, devices.data());

	for (const auto &device : devices) {
		if (isDeviceSuitable(device)) {
			if (comparePhysicalDevices(physicalDevice, device)) {
				physicalDevice = device;
			}
		}
	}
	if (physicalDevice == VK_NULL_HANDLE) {
		std::cerr << "NO SUITABLE GPUS :(" << std::endl;
		throw std::runtime_error("No Suitable GPUs found");
	}
}

bool VulkanEngine::isDeviceSuitable(VkPhysicalDevice device) {
	bool extensionsSupported = checkDeviceExtensionSupport(device);

	QueueFamilyIndices indices = findQueueFamilies(device);

	bool swapChainAdequate = false;
	if (extensionsSupported) {
		SwapChainSupportDetails swapChainSupport =
			querySwapChainSupport(device);
		swapChainAdequate = !swapChainSupport.formats.empty() &&
							!swapChainSupport.presentModes.empty();
	}

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

	return indices.isComplete() && extensionsSupported && swapChainAdequate &&
		   supportedFeatures.samplerAnisotropy;
	// return true;
	// return deviceProperties.deviceType ==
	// VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
	// deviceFeatures.geometryShader;
}

bool VulkanEngine::checkDeviceExtensionSupport(VkPhysicalDevice device) {
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
										 nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
										 availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(),
											 deviceExtensions.end());

	for (const auto &extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

bool VulkanEngine::comparePhysicalDevices(VkPhysicalDevice device0,
										  VkPhysicalDevice device1) {
	if (device1 == VK_NULL_HANDLE)
		return false;
	if (device0 == VK_NULL_HANDLE)
		return true;

	VkPhysicalDeviceProperties deviceProperties0;
	VkPhysicalDeviceFeatures deviceFeatures0;
	vkGetPhysicalDeviceProperties(device0, &deviceProperties0);
	vkGetPhysicalDeviceFeatures(device0, &deviceFeatures0);

	VkPhysicalDeviceProperties deviceProperties1;
	VkPhysicalDeviceFeatures deviceFeatures1;
	vkGetPhysicalDeviceProperties(device1, &deviceProperties1);
	vkGetPhysicalDeviceFeatures(device1, &deviceFeatures1);

	if (deviceProperties0.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
		deviceProperties1.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		return false;
	} else if (deviceProperties0.deviceType !=
				   VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
			   deviceProperties1.deviceType ==
				   VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		return true;
	}

	return false;
}

void VulkanEngine::mainLoop() {
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		// NEW IMGUI UPDATE:
		describeUI();
		drawFrame();
	}
	vkDeviceWaitIdle(device);
}

void VulkanEngine::drawFrame() {
	vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE,
					UINT64_MAX);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(
		device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame],
		VK_NULL_HANDLE, &imageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapChain();
		return;
	} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("Failed to acquire swap chain image");
	}

	updateUniformBuffers();

	// Only reset fence if we are all good
	vkResetFences(device, 1, &inFlightFences[currentFrame]);

	vkResetCommandBuffer(commandBuffers[currentFrame], 0);
	recordMainCommandBuffer(commandBuffers[currentFrame], imageIndex);

	vkResetCommandBuffer(imguiCommandBuffers[currentFrame], 0);
	recordUICommandBuffer(imguiCommandBuffers[currentFrame], imageIndex);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
	VkPipelineStageFlags waitStages[] = {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	std::array<VkCommandBuffer, 2> currCommandBuffers = {
		commandBuffers[currentFrame], imguiCommandBuffers[currentFrame]};

	submitInfo.commandBufferCount =
		static_cast<uint32_t>(currCommandBuffers.size());
	submitInfo.pCommandBuffers = currCommandBuffers.data();

	VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo,
					  inFlightFences[currentFrame]) != VK_SUCCESS) {
		throw std::runtime_error("Failed to submit draw command buffer");
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = {swapChain};
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	result = vkQueuePresentKHR(presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
		framebufferResized) {
		framebufferResized = false;
		recreateSwapChain();
	} else if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to present swapchain image");
	}

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanEngine::updateUniformBuffers() {
	static auto startTime = std::chrono::high_resolution_clock::now();
	static auto prevTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(
					 currentTime - startTime)
					 .count();
	float deltaTime =
		std::chrono::duration<float, std::chrono::seconds::period>(currentTime -
																   prevTime)
			.count();
	prevTime = currentTime;

	// UPDATE GRAPHICS UBO
	float currTheta = time * M_PI / 6;
	float c = cos(currTheta);
	float s = sin(currTheta);

	/*
	mat4x4 model_rotation{
		1, 0, 0, 0, 0, 1, 0, 0, c, s, 1, 0, 0, 0, 0, 1,
	};
	*/
	mat4x4 model_rotation{
		1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1,
	};

	mat4x4 view_mat{
		1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1,
	};

	mat4x4 proj_mat{
		1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1,
	};

	/*
	if (currFractal == SIERPINSKI_PYRAMID) {
		model_rotation =
			mat4x4{c, s, 0, 0, -s, c, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
		model_rotation =
			mat4x4{c, 0, s, 0, 0, 1, 0, 0, -s, 0, c, 0, 0, 0, 0, 1};
		view_mat = mat4x4{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1};
		proj_mat = mat4x4{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0.1, 0, 0, 0, 0, 1};
		model_rotation =
			mat4x4{cos(theta), sin(theta), 0, 0, -sin(theta), cos(theta), 0, 0,
				   0,		   0,		   1, 0, 0,			  0,		  0, 1};
		view_mat = mat4x4{c, 0, s, 0, 0, 1, 0, 0, -s, 0, c, 0, 0, 0, 0, 1};
		proj_mat = mat4x4{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0.1, 0, 0, 0, 0.5, 1};
	} else if (currFractal == SIERPINSKI_CUBE) {
		model_rotation = {sqrt(2.f / 3.f),
						  0,
						  1 / sqrt(3.f),
						  0,
						  -1 / sqrt(6.f),
						  1 / sqrt(2.f),
						  1 / sqrt(3.f),
						  0,
						  -1 / sqrt(6.f),
						  -1 / sqrt(2.f),
						  1 / sqrt(3.f),
						  0,
						  0,
						  0,
						  0,
						  1};

		model_rotation =
			mat4x4{cos(theta), sin(theta), 0, 0, -sin(theta), cos(theta), 0, 0,
				   0,		   0,		   1, 0, 0,			  0,		  0, 1};
		view_mat = mat4x4{c, 0, s, 0, 0, 1, 0, 0, -s, 0, c, 0, 0, 0, 0, 1};
		proj_mat = mat4x4{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0.1, 0, 0, 0, 0.5, 1};
	}
	*/

	/*
	// UPDATE COMPUTE UBO
	ComputeUniformBufferObject cUbo{};
	cUbo.time = time;
	cUbo.deltaTime = deltaTime;
	lo_xy = std::array<float, 2>{
		-1.0f * swapChainExtent.width / swapChainExtent.height, -1.0f};
	hi_xy = std::array<float, 2>{
		1.0f * swapChainExtent.width / swapChainExtent.height, 1.0f};
	for (int i = 0; i < 2; ++i) {
		cUbo.lo_xy[i] = lo_xy[i];
		cUbo.hi_xy[i] = hi_xy[i];
	}
	cUbo.defaultLifetime = defaultLifetime;
	cUbo.lifetimeJiggle = lifetimeJiggle;

	cUbo.sphereBufferSize = POINT_COUNT;
	*/
	ComputeUniformBufferObject cUbo{};
	lo_xy =
		std::array<float, 2>{-1.0f * static_cast<float>(swapChainExtent.width) /
								 static_cast<float>(swapChainExtent.height),
							 -1.0f};
	hi_xy =
		std::array<float, 2>{1.0f * static_cast<float>(swapChainExtent.width) /
								 static_cast<float>(swapChainExtent.height),
							 1.0f};
	julia_c = std::array<float, 2>{c * 0.7885f, s * 0.7885f};

	for (int i = 0; i < 2; ++i) {
		cUbo.lo_xy[i] = lo_xy[i];
		cUbo.hi_xy[i] = hi_xy[i];
		cUbo.julia_c[i] = julia_c[i];
	}
	cUbo.subdivisionCount = subdivisionCount;

	memcpy(computeUniformBuffersMapped[currentFrame], &cUbo, sizeof(cUbo));

	if (currFractal == SIERPINSKI_PYRAMID || currFractal == SIERPINSKI_CUBE) {
		model_rotation =
			mat4x4{cos(theta), sin(theta), 0, 0, -sin(theta), cos(theta), 0, 0,
				   0,		   0,		   1, 0, 0,			  0,		  0, 1};
		view_mat = mat4x4{c, 0, s, 0, 0, 1, 0, 0, -s, 0, c, 0, 0, 0, 0, 1};
		proj_mat = mat4x4{2.f / (hi_xy[0] - lo_xy[0]),
						  0,
						  0,
						  0,
						  0,
						  1,
						  0,
						  0,
						  0,
						  0,
						  0.1,
						  0,
						  0,
						  0,
						  0.5,
						  1};
	}
	GraphicsUniformBufferObject gUbo{model_rotation, view_mat, proj_mat};
	memcpy(graphicsUniformBuffersMapped[currentFrame], &gUbo, sizeof(gUbo));
}

std::vector<const char *> VulkanEngine::getRequiredExtensions() {
	uint32_t glfwExtensionCount = 0;
	const char **glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char *> extensions(glfwExtensions,
										 glfwExtensions + glfwExtensionCount);

	if (enableValidationLayers)
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	return extensions;
}

void VulkanEngine::createVulkanInstance() {
	if (enableValidationLayers && !checkValidationLayerSupport()) {
		throw std::runtime_error(
			"Validation layers requested but were not available");
	}
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Complex Analysis Visualization Framework";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_HEADER_VERSION_COMPLETE;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	auto extensions = getRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (enableValidationLayers) {
		createInfo.enabledLayerCount =
			static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

		populateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext =
			(VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
	} else {
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	if (vkCreateInstance(&createInfo, nullptr, &vulkanInstance) != VK_SUCCESS) {
		throw std::runtime_error("Vulkan Instance Creation Failed");
	}
}

void VulkanEngine::populateDebugMessengerCreateInfo(
	VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
							 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
							 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
}

void VulkanEngine::setupDebugMessenger() {
	if (!enableValidationLayers)
		return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	populateDebugMessengerCreateInfo(createInfo);

	if (createDebugUtilsMessengerEXT(vulkanInstance, &createInfo, nullptr,
									 &debugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug messenger!");
	}
}

bool VulkanEngine::checkValidationLayerSupport() {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char *layerName : validationLayers) {
		bool found = false;
		for (const auto &layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				found = true;
				break;
			}
		}
		if (not found)
			return false;
	}
	return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanEngine::debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
	void *pUserData) {
	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT ||
		messageType == VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT ||
		messageType == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
		// Breakpointing and checking the pObjects value of pCallbackData
		// should let you get the Vulkan object handles related to the
		// message
		std::cerr << "\n";
		std::cerr << "validation layer: " << pCallbackData->pMessage << "\n";
	}
	return VK_FALSE;
}

void VulkanEngine::initUI() {
	// Create ImGUI RenedrPass
	createUIRenderPass();
	createUIFramebuffers();
	createUICommandPool();
	createUICommandBuffers();

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	io.ConfigFlags |=
		ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	io.ConfigFlags |=
		ImGuiConfigFlags_NavEnableGamepad;			  // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // IF using Docking Branch
	ImGui::StyleColorsDark();

	// Provide bind points from Vulkan API
	ImGui_ImplGlfw_InitForVulkan(window, true);
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = vulkanInstance;
	init_info.PhysicalDevice = physicalDevice;
	init_info.Device = device;
	init_info.QueueFamily =
		findQueueFamilies(physicalDevice).graphicsAndComputeFamily.value();
	init_info.Queue = graphicsQueue;
	init_info.PipelineCache = VK_NULL_HANDLE;
	init_info.DescriptorPool = descriptorPool;
	init_info.RenderPass = imguiRenderPass;
	init_info.Subpass = 0;
	init_info.MinImageCount = swapChainImages.size();
	init_info.ImageCount = swapChainImages.size();
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.Allocator = nullptr;
	init_info.CheckVkResultFn = checkVKResult;
	ImGui_ImplVulkan_Init(&init_info);

	// Upload the fonts for DearImgui
	ImGui_ImplVulkan_CreateFontsTexture();
}

void VulkanEngine::createUIRenderPass() {
	VkAttachmentDescription attachment = {};
	attachment.format = swapChainImageFormat;
	attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	// UNSURE ON THIS ONE
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	VkAttachmentReference color_attachment = {};
	color_attachment.attachment = 0;
	color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment;
	VkSubpassDependency dependency = {};
	// TODO: FIGURE OUT HOW DEPENDENCIES WORK
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	VkRenderPassCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	info.attachmentCount = 1;
	info.pAttachments = &attachment;
	info.subpassCount = 1;
	info.pSubpasses = &subpass;
	info.dependencyCount = 1;
	info.pDependencies = &dependency;

	if (vkCreateRenderPass(device, &info, nullptr, &imguiRenderPass) !=
		VK_SUCCESS) {
		throw std::runtime_error("Faiiled to create IMGUI Render Pass");
	}
}

void VulkanEngine::createUIFramebuffers() {
	imguiFramebuffers.resize(swapChainImageViews.size());
	for (size_t i = 0; i < swapChainImageViews.size(); ++i) {
		VkImageView attachments[1] = {swapChainImageViews[i]};
		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = imguiRenderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = &attachments[0];
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = 1;
		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr,
								&imguiFramebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}
void VulkanEngine::createUICommandPool() {
	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.queueFamilyIndex =
		findQueueFamilies(physicalDevice).graphicsAndComputeFamily.value();
	commandPoolCreateInfo.flags =
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr,
							&imguiCommandPool) != VK_SUCCESS) {
		throw std::runtime_error("Could not create imgui command pool!");
	}
}

void VulkanEngine::createUICommandBuffers() {
	imguiCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = imguiCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount =
		static_cast<uint32_t>(imguiCommandBuffers.size());

	if (vkAllocateCommandBuffers(device, &allocInfo,
								 imguiCommandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate imgui command buffers");
	}
}

void VulkanEngine::recordUICommandBuffer(VkCommandBuffer uiCommandBuffer,
										 uint32_t imageIndex) {
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	if (vkBeginCommandBuffer(uiCommandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error(
			"Unable to start recording imgui command buffer!");
	}

	VkClearValue clearColor{0.0f, 0.0f, 0.0f, 1.0f};
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = imguiRenderPass;
	renderPassInfo.framebuffer = imguiFramebuffers[imageIndex];
	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = swapChainExtent;

	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;

	vkCmdBeginRenderPass(uiCommandBuffer, &renderPassInfo,
						 VK_SUBPASS_CONTENTS_INLINE);
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), uiCommandBuffer);
	vkCmdEndRenderPass(uiCommandBuffer);

	if (vkEndCommandBuffer(uiCommandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to record imgui command buffer!");
	}
}

void VulkanEngine::describeUI() {
	/*
	static bool autoClear = false;
	static auto prevClearTime = std::chrono::high_resolution_clock::now();
	static float clearTime = 1.0f;
	*/

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Controls");

	const char *options[] = {"Sierpinski Triangle", "Sierpinski Pyramid",
							 "Sierpinski Carpet",	"Sierpinski Cube",
							 "Mandelbrot Set",		"Julia Set",
							 "Sierpinski Cube RT"};
	ImGui::Combo("Current Fractal", &currFractal, options, 7);

	if (currFractal == SIERPINSKI_PYRAMID || currFractal == SIERPINSKI_CUBE) {
		ImGui::SliderFloat("Theta", &theta, 0.0f, 2 * M_PI);
	}
	if (currFractal == SIERPINSKI_TRIANGLE ||
		currFractal == SIERPINSKI_PYRAMID || currFractal == SIERPINSKI_CARPET ||
		currFractal == SIERPINSKI_CUBE) {
		int maxSubdivisions = 0;
		if (currFractal == SIERPINSKI_TRIANGLE) {
			// maxSubdivisions = 11;
			maxSubdivisions = 7;
		} else if (currFractal == SIERPINSKI_PYRAMID) {
			// maxSubdivisions = 9;
			maxSubdivisions = 7;
		} else if (currFractal == SIERPINSKI_CARPET) {
			maxSubdivisions = 6;
		} else if (currFractal == SIERPINSKI_CUBE) {
			maxSubdivisions = 4;
		}

		subdivisionCount = subdivisionCount > maxSubdivisions
							   ? maxSubdivisions
							   : subdivisionCount;

		ImGui::SliderInt("Subdivisions", &subdivisionCount, 0, maxSubdivisions);
	}

	/*
	if (ImGui::Button("Clear")) {
		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			shouldClear[i] = true;
		}
	}

	ImGui::Checkbox("Auto Clear", &autoClear);
	ImGui::SliderFloat("Clear Timer", &clearTime, 0.0f, 5.0f);

	auto currentTime = std::chrono::high_resolution_clock::now();
	if (autoClear && std::chrono::duration<float, std::chrono::seconds::period>(
						 currentTime - prevClearTime)
							 .count() > clearTime) {
		prevClearTime = currentTime;
		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			shouldClear[i] = true;
		}
	}

	ImGui::SliderFloat("Default Lifetime", &defaultLifetime, 0.1f, 5.0f);
	ImGui::SliderFloat("Lifetime Jiggle", &lifetimeJiggle, 0.1f, 5.0f);
	*/
	ImGui::End();

	ImGui::Render();
}

void VulkanEngine::cleanup() {
	cleanupSwapChain();

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	vkDestroyCommandPool(device, imguiCommandPool, nullptr);
	vkDestroyRenderPass(device, imguiRenderPass, nullptr);

	vkDestroyPipeline(device, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device, graphicsPipelineLayout, nullptr);

	vkDestroyPipeline(device, computeSierpinskiTrianglePipeline, nullptr);
	vkDestroyPipeline(device, computeSierpinskiPyramidPipeline, nullptr);
	vkDestroyPipeline(device, computeSierpinskiCarpetPipeline, nullptr);
	vkDestroyPipeline(device, computeSierpinskiCubePipeline, nullptr);
	vkDestroyPipeline(device, computeMandelbrotPipeline, nullptr);
	vkDestroyPipeline(device, computeJuliaPipeline, nullptr);
	vkDestroyPipelineLayout(device, computePipelineLayout, nullptr);
	vkDestroyPipelineLayout(device, computeBufferPipelineLayout, nullptr);

	vkDestroyRenderPass(device, graphicsRenderPass, nullptr);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		vkDestroyBuffer(device, computeUniformBuffers[i], nullptr);
		vkFreeMemory(device, computeUniformBuffersMemory[i], nullptr);
		vkDestroyBuffer(device, graphicsUniformBuffers[i], nullptr);
		vkFreeMemory(device, graphicsUniformBuffersMemory[i], nullptr);
	}

	vkDestroyDescriptorPool(device, descriptorPool, nullptr);

	vkDestroySampler(device, textureSampler, nullptr);
	vkDestroyImageView(device, textureImageView, nullptr);
	vkDestroyImage(device, textureImage, nullptr);
	vkFreeMemory(device, textureImageMemory, nullptr);

	vkDestroyDescriptorSetLayout(device, computeDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(device, computeBufferDescriptorSetLayout,
								 nullptr);
	vkDestroyDescriptorSetLayout(device, graphicsDescriptorSetLayout, nullptr);

	vkDestroyBuffer(device, pointBuffer, nullptr);
	vkFreeMemory(device, pointBufferMemory, nullptr);

	vkDestroyBuffer(device, indexBuffer, nullptr);
	vkFreeMemory(device, indexBufferMemory, nullptr);

	vkDestroyBuffer(device, vertexBuffer, nullptr);
	vkFreeMemory(device, vertexBufferMemory, nullptr);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
		vkDestroyFence(device, inFlightFences[i], nullptr);
	}

	vkDestroyCommandPool(device, commandPool, nullptr);
	vkDestroyDevice(device, nullptr);

	if (enableValidationLayers) {
		destroyDebugUtilsMessengerEXT(vulkanInstance, debugMessenger, nullptr);
	}

	vkDestroySurfaceKHR(vulkanInstance, surface, nullptr);
	vkDestroyInstance(vulkanInstance, nullptr);

	glfwDestroyWindow(window);
	glfwTerminate();
}

mat4x4 transpose(mat4x4 a) {
	mat4x4 b;
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			b[i][j] = a[j][i];
		}
	}
	return b;
}
mat4x4 mul(mat4x4 a, mat4x4 b) {
	mat4x4 c;

	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			c[i][j] = 0;
			for (int k = 0; k < 4; ++k) {
				c[i][j] += a[i][k] * b[k][j];
			}
		}
	}
	return c;
}
