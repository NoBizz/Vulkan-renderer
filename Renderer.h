#pragma  once

#include <cstring>
#include <optional>
#include <limits>
#include <algorithm>
#include <cstdint>
#include <array>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <fstream>
#include <set>


#include "Verts.cpp"
#include "UniformBufferObj.cpp"




//basic display initialisation structure
class Renderer {

public:

	Verts verticies;

	void run();

	//read spv binary files of images to display on the screen
	static std::vector<char> readFile(const std::string);

	VkShaderModule createShaderModule(const std::vector<char>);

	//logical device
	VkDevice device;

	struct SwapChainSupportDetails;

	struct queueFamilies;

	void initWindow();

	void initVulkan();

	void createInstance();

	void renderLoop();

	void cleanup();

	bool checkValidationLayerSupport();

	void searchPhysicalDevice();

	bool isDeviceSuitable(VkPhysicalDevice);

	void createLogicalDevice();

	void createSurface(); // windows specific

	bool checkDeviceExtensionSupport(VkPhysicalDevice);

	SwapChainSupportDetails querySwapchainSupport(VkPhysicalDevice);

	VkSurfaceFormatKHR choseSwapChainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>);

	VkPresentModeKHR choseSwapChainPresentMode(const std::vector<VkPresentModeKHR>);

	VkExtent2D choseSwapExtent(const VkSurfaceCapabilitiesKHR);

	void createSwapChain();

	void createImageView();

	void createGraphicsPipeline();

	void createRenderPass();

	void createFrameBuffers();

	void createCommandPool();

	void createCommandBuffers();

	void recordCommandBuffer(VkCommandBuffer, uint32_t);

	void drawFrame();

	void createSyncObject();

	void createVertexBuffer(Verts);

	void createIndexBuffer(Verts);

	void copyBuffer(VkBuffer,VkBuffer,VkDeviceSize);

	void createTexture();

	void createImage(uint32_t , uint32_t , VkFormat , VkImageTiling , VkImageUsageFlags , VkMemoryPropertyFlags , VkImage& , VkDeviceMemory& );

	void createDescriptionSetLayout();

	void createUniformBuffers();

	void updateUniformBuffer(uint32_t);

	void createDescriptorPool();

	void createDescriptorSet();

	VkImageView createTextureView(VkImage,VkFormat);



	//this handels resizing of the window
	void recreateSwapChain();
	void cleanupSwapChain();
    static void framebufferResizeCallback(GLFWwindow*, int, int);

	//texture processing
	VkCommandBuffer textureLoadStart();
	VkImageView textureView;
	VkSampler textureSampler;
	void textureLoadEnd(VkCommandBuffer);
	void transitionTextureLayout(VkImage , VkFormat , VkImageLayout , VkImageLayout );
	void copyBufferToTexture(VkBuffer , VkImage , uint32_t , uint32_t );

	void createTextureImage();
	void createTextureImageViews();
	void createTextureSampler();

	//texture handling
	VkImage texture;
	VkDeviceMemory textureMemory;

	uint32_t findMemoryType(uint32_t, VkMemoryPropertyFlags);

	queueFamilies queryQueueFamilies(VkPhysicalDevice);

	//attributer of Verticies to be drawn on the screen
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	//Index buffer
	VkBuffer indexBuffer;
	VkDeviceMemory indexBuffermemory;


	bool hasIndexBuffer = true;
	int vertexIndex;

	void createBuffer(VkDeviceSize,VkBufferUsageFlags,VkMemoryPropertyFlags,VkBuffer&,VkDeviceMemory&);

	//Variable to keep track of the physical device
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE; //initialization required before setup so we initialize with null

	//width and height for the window initialization
	const uint32_t WIDTH = 1920;
	const uint32_t HEIGHT = 1080;

	//structure we store the windows we create in
    GLFWwindow* window;

	//create handel for the vulkan instance
	VkInstance instance;

	//interface to render graphics
	VkQueue graphicQueue;

	//interface to display to the screen
	VkQueue presentationQueue;

	//surface to display to the window 
	VkSurfaceKHR surface; //this is widnows specific

	//swapchain to give and get images from
	VkSwapchainKHR swapChain;

	//retreave swap chain images
	std::vector<VkImage> swapChainImages;

	//swap Chain variables for render use
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

	//an array to hold all image viewers for swapchain to collor in later
	std::vector<VkImageView> swapChainImageViews;

	//pipeline layout
	VkPipelineLayout pipelineLayout;

	//rendering
	VkRenderPass renderPass;

	//Graphical pipeline
	VkPipeline graphicsPipeline;

	//frame buffers
	std::vector<VkFramebuffer> swapChainFrameBuffers;

	//command pool
	VkCommandPool commandPool;

	std::vector<VkCommandBuffer> commandBuffers;

	//synchronization
	std::vector<VkSemaphore> imageAvailableSemaphore;
	std::vector<VkSemaphore> renderFinishedSemaphore;
	std::vector<VkFence> inFlightFence;

	//keep track weather or not we resize
	bool frameBufferResized = false;

	//list of extensions to search for
	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	//max frames we want to run parralel to each other
	const int MAX_FRAMES_IN_FLIGHT = 2;

	//keep track of current frame
	uint32_t currentFrame = 0;

	//variable to hold a list of the validation layers we want to have
	const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

	//Descriptor buffers
	VkDescriptorSetLayout descriptorSet;
	VkPipelineLayout descriptorPipelineLayout;
	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;

	//Uniform buffers
	VkBuffer uniformIndexBuffer;
	VkDeviceMemory UniformIndexBufferMemory;
	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	std::vector<void*> uniformBuffersMapped;

	//validation layer settings
    //Validation layers are deactivated as they cause a crash on cleanup
#ifdef NDEBUG
	const bool enableValidationLayers = false;

#else
	const bool enableValidationLayers = false;

#endif

};

