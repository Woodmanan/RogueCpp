#pragma once
#include "vulkan/vulkan.hpp"
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include "glm/glm.hpp"
#include "Debug/Debug.h"
#include "Render/Fonts/FontManager.h"
#include "Core/CoreDataTypes.h"

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <optional>
#include <fstream>
#include <string>

const uint MAX_DIM = 64;
const uint MAX_TILES = MAX_DIM * MAX_DIM;
const uint MAX_CHARACTERS = 128;
const float offset = 0.1;

struct Vertex {
	alignas(16) glm::vec2 pos;
	alignas(16) glm::vec2 texCoord;
	alignas(16) glm::uint index;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, texCoord);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32_UINT;
		attributeDescriptions[2].offset = offsetof(Vertex, index);

		return attributeDescriptions;
	}
};

struct UniformTileObject {
	glm::uint tileIndices[MAX_TILES];
};

struct UniformFontData {
	glm::vec2 lowerUVs[MAX_CHARACTERS];
	glm::vec2 upperUVs[MAX_CHARACTERS];
};

struct FGColorsObject {
	glm::vec4 colors[MAX_TILES];
};

struct BGColorsObject {
	glm::vec4 colors[MAX_TILES];
};

class VulkanTerminal
{
public:
	void Initialize();
	void RenderFrame();
	void Clear();
	void ClearArea(int x, int y, int width, int height);
	void Shutdown();

	const uint WINDOW_WIDTH = 800;
	const uint WINDOW_HEIGHT = 600;
	const int MAX_FRAMES_IN_FLIGHT = 2;
	const std::wstring preloadChars = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890!@#$%^&*()[]{}\\|/<>,.:;?-=_+~ \u2588\u2550\u2551\u2554\u2557\u255A\u255D";
	
	const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
	};

	const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME,
	VK_KHR_UNIFORM_BUFFER_STANDARD_LAYOUT_EXTENSION_NAME
	};

	static const uint TERMINAL_WIDTH = 51;
	static const uint TERMINAL_HEIGHT = 31;

	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;

#if (DEBUG || REL_WITH_DEBUG)
	const bool enableValidationLayers = true;
#else
	const bool enableValidationLayers = false;
#endif

	void SetFGColor(Color inColor) { fgColor = inColor; }
	void SetBGColor(Color inColor) { bgColor = inColor; }
	void SetCharacter(int x, int y, int characterCode);
	void SetDepth(short newDepth);
	GLFWwindow* GetWindow() { return window; }

private:
	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool IsComplete()
		{
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	GLFWwindow* window;
	VkInstance instance;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkSurfaceKHR surface;
	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	VkRenderPass renderPass;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	VkCommandPool commandPool;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	std::vector<VkBuffer> tileUniformBuffers;
	std::vector<VkDeviceMemory> tileUniformBuffersMemory;
	std::vector<void*> tileUniformBuffersMapped;

	std::vector<VkBuffer> fontUniformBuffers;
	std::vector<VkDeviceMemory> fontUniformBuffersMemory;
	std::vector<void*> fontUniformBuffersMapped;

	std::vector<VkBuffer> fgColorBuffers;
	std::vector<VkDeviceMemory> fgColorBuffersMemory;
	std::vector<void*> fgColorBuffersMapped;

	std::vector<VkBuffer> bgColorBuffers;
	std::vector<VkDeviceMemory> bgColorBuffersMemory;
	std::vector<void*> bgColorBuffersMapped;

	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;

	std::vector<VkCommandBuffer> commandBuffers;

	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
	VkSampler textureSampler;

	RogueResources::TResourcePointer<RogueFont> font;
	UniformTileObject tileData;
	UniformFontData fontData;
	FGColorsObject fgColorData;
	BGColorsObject bgColorData;

	Color fgColor;
	Color bgColor;

	std::array<short, MAX_TILES> tileDepths;
	short depth;

	//Rendering Sync tools
	int currentFrame = 0;
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	bool framebufferResized = false;

	std::vector<VkImageView> swapChainImageViews;

	void InitWindow();
	void InitVulkan();

	void CreateInstance();
	void CreateSurface();
	bool CheckValidationLayerSupport();
	bool CheckDeviceExtensionSupport(VkPhysicalDevice device);

	void PickPhysicalDevice();
	bool IsDeviceSuitable(VkPhysicalDevice device);
	float RateDevice(VkPhysicalDevice device);
	void CreateLogicalDevice();
	void CreateSwapChain();
	void CreateImageViews();
	void CreateRenderPass();
	void CreateDescriptorSetLayout();
	void CreateGraphicsPipeline();
	void CreateFrameBuffers();
	void CreateCommandPool();
	void CreateFont();
	void CreateTextureImage();
	void CreateTextureImageView();
	void CreateTextureSampler();
	void CreateVertices();
	void CreateVertexBuffer();
	void CreateIndexBuffer();
	void CreateUniformBuffers();
	void CreateDescriptorPool();
	void CreateDescriptorSets();
	void CreateCommandBuffers();
	void CreateSyncObjects();

	//Command Helpers
	VkCommandBuffer BeginSingleTimeCommands();
	void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

	VkShaderModule CreateShaderModule(const std::vector<char>& code);
	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void CopyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);
	void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	VkImageView CreateImageView(VkImage image, VkFormat format);
	void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);
	
	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	//Swap chain settings
	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	void RecreateSwapChain();
	void CleanupSwapChain();

	static void NotifyResize(GLFWwindow* window, int width, int height);

	//Draw commands
	void RecordCommandBuffer(VkCommandBuffer buffer, uint32_t imageIndex);
	void UpdateUniformBuffer(int currentFrame);
	void DrawFrame();

	void MainLoop();

	void Cleanup();
};

static std::vector<char> readFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}