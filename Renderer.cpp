

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define NOMINMAX //bug fix to make std::numeric_limits<size_t>::max() not use max() as a macro but as a function
#include "Renderer.h"



//structure implementation
void Renderer::run() {

	Renderer::initWindow();
	Renderer::initVulkan();
	Renderer::renderLoop();
	Renderer::cleanup();
	
}



void Renderer::initWindow() {

	//initialize glfw for drawing a window

	glfwInit();

	//disable dynamic resizing since that will be done later
	glfwWindowHint(GLFW_CLIENT_API,GLFW_NO_API); //telling it not to use OpenGL API

	//create a window
    Renderer::window = glfwCreateWindow(Renderer::WIDTH, Renderer::HEIGHT,"Renderer v1.0",nullptr,nullptr);//last parameter is only relevant to OpenGL
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void Renderer::initVulkan() {

	Renderer::createInstance();
	Renderer::createSurface();
	Renderer::searchPhysicalDevice();
	Renderer::createLogicalDevice();
	Renderer::createSwapChain();
	Renderer::createImageView();
	Renderer::createRenderPass();
	Renderer::createDescriptionSetLayout();
	Renderer::createGraphicsPipeline();
	Renderer::createFrameBuffers();
	Renderer::createCommandPool();
	Renderer::createTexture();
	Renderer::createTextureImage();
	Renderer::createTextureSampler();
	Renderer::createVertexBuffer(Renderer::verticies);
	Renderer::createIndexBuffer(Renderer::verticies);
	//3D upscale
	Renderer::createUniformBuffers();
	Renderer::createDescriptorPool();
	Renderer::createDescriptorSet();

	Renderer::createCommandBuffers();
	Renderer::createSyncObject();
	
}



void Renderer::createInstance() {

	//check validation layers
	if ((Renderer::enableValidationLayers) && !(Renderer::checkValidationLayerSupport())) {
		throw std::runtime_error("Validation layer required, bur none available");
	}
	
	//to create an instance we have to pass some details about the system to the application
	
	VkApplicationInfo appInfo{}; // VkApp is a structure denoted by {}
	
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Triangle drawing";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1,0,0);
	appInfo.apiVersion = VK_API_VERSION_1_3;	

	/*in vulkan most of the information is passed through strucutres rather than functions
		must pass information is usually prefixed by s and p
		s-Type
		p-Type
		parameters have to be filled
	*/

	//strucute that holds validation 
	VkInstanceCreateInfo createInfo{};

	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	//pass an Interface/API to vulkan which draws/renders to the screen in this case glfw
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;

	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	createInfo.enabledExtensionCount = glfwExtensionCount;
	createInfo.ppEnabledExtensionNames = glfwExtensions;
	createInfo.enabledLayerCount = 0;

	//vulkan functions all return VkResult
	VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);//the last parameter is a pointer to where to store the result of the called function

	if (vkCreateInstance(&createInfo,nullptr,&instance)!= VK_SUCCESS) {
		throw std::runtime_error("Failed to create instance!");
	}
}

void Renderer::renderLoop() {

	//keep the window running either till an error occurs ore we close the window
	while (!glfwWindowShouldClose(Renderer::window)) {
		glfwPollEvents();
		drawFrame();
	}

	vkDeviceWaitIdle(device);

}

void Renderer::cleanup() {

	cleanupSwapChain();


	vkDestroySampler(device, textureSampler, nullptr);
	vkDestroyImageView(device, textureView, nullptr);
	vkDestroyImage(device, texture, nullptr);
	vkFreeMemory(device, textureMemory,nullptr);

	vkDestroyBuffer(device,vertexBuffer,nullptr);
	vkFreeMemory(device, vertexBufferMemory, nullptr);

	vkDestroyBuffer(device, indexBuffer, nullptr);
	vkFreeMemory(device,indexBuffermemory,nullptr);

	//Uniform buffers
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroyBuffer(device, uniformBuffers[i], nullptr);
		vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
	}
	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(device, descriptorSet, nullptr);

	//semaphore synchronizors
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
	vkDestroySemaphore(device, imageAvailableSemaphore[i], nullptr);
	vkDestroySemaphore(device, renderFinishedSemaphore[i], nullptr);
	vkDestroyFence(device, inFlightFence[i], nullptr);
}

	vkDestroyCommandPool(device, commandPool, nullptr);

	vkDestroyPipeline(device,graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroyRenderPass(device,renderPass,nullptr);

	//destroy logical device
	vkDestroyDevice(Renderer::device, nullptr);


	//VkInstance should only be destroyed before the program exits
	vkDestroySurfaceKHR(instance,surface, nullptr);
	vkDestroyInstance(instance ,nullptr);

	//once done we free up resources
	glfwDestroyWindow(Renderer::window);
	glfwTerminate();

}

//create validation layers
bool Renderer::checkValidationLayerSupport() {

	uint32_t layerCount;

	//get all available validation layers
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	/*we use vector for simpler data extraction in specificaly availableLayers.data
	 so we dont have to do any manual data reformating*/
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount,availableLayers.data());


	//check if all validation layers exist
	for (const char* layerName: validationLayers) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName,layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}
		if (!layerFound) {
			return false;
	  }
	}
	return true;

}

// look for the physical device
void Renderer::searchPhysicalDevice() {

	//get the number of all physical devices
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(Renderer::instance, &deviceCount, nullptr);

	//check if any physical devices are supported
	if (deviceCount == 0) {
		throw std::runtime_error("No physical devices suported!");
	}

	//initialize an array (vector) to store all pointers to physical devices
	std::vector<VkPhysicalDevice> deviceHandler(deviceCount);
	vkEnumeratePhysicalDevices(Renderer::instance, &deviceCount, deviceHandler.data());

	//pick the first available suitable device (Usualy the GPU in the top PCIE slot)
	for (const auto& device : deviceHandler) {
		if (Renderer::isDeviceSuitable(device)) {
			Renderer::physicalDevice = device;
			break;
		}
	}

	if (Renderer::physicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("No suitable physical device/GPU !");
	}
}



//swapChain strucutre
struct Renderer::SwapChainSupportDetails {

	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;

};


//check if the device is suitable for use
bool Renderer::isDeviceSuitable(VkPhysicalDevice device) {

	//we query for device details names,version,firmware... and vulkan support

	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;

	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	
	bool extensionSupport = checkDeviceExtensionSupport(device);

	//check swapchain
	bool swapChainAdequate = false;
	if (extensionSupport) {
		Renderer::SwapChainSupportDetails swapChainSupport = querySwapchainSupport(device);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
		   deviceFeatures.geometryShader && extensionSupport && swapChainAdequate;
	
}

//enumerate and check the extensions
bool Renderer::checkDeviceExtensionSupport(VkPhysicalDevice device) {
	
	//enumerate
	uint32_t extensionCount;
	
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount , nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	//check is extensions are supported
	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto& extensions : availableExtensions) {
		requiredExtensions.erase(extensions.extensionName);
	}

	return requiredExtensions.empty();

}


//command sets(families) to query
struct Renderer::queueFamilies {

	// std::optional is a wrapper that contains no value until we assign something to it
	std::optional<uint32_t> graphiscFamily;
	std::optional<uint32_t> presentationFamily;

	bool isComplete() {
		return graphiscFamily.has_value() && presentationFamily.has_value();
	}
};

//check what command subsets(families) are we capable of running on the device
Renderer::queueFamilies Renderer::queryQueueFamilies(VkPhysicalDevice device) {
	
	Renderer::queueFamilies subsets;

	//get queue command family(subset) information
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	//store all queue families in the array
	std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyProperties.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilyProperties) {
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			subsets.graphiscFamily = i;
		}

		//check presentation family support
		VkBool32 presentationSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentationSupport);

		if (presentationSupport) {
			subsets.presentationFamily = i;
		}
		if (subsets.isComplete()) {
			break;
		}

		i++;
	}
	return subsets;
}


//create a logical device to execute commands on
void Renderer::createLogicalDevice() {

	//search for a device with graphis capabilities
	queueFamilies handler = queryQueueFamilies(physicalDevice);
	//create both queues for rendering graphics and diplaying to screen
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { handler.graphiscFamily.value(),handler.presentationFamily.value() };

	float queuePriority = 1.0f;

	//specify the queue to be created as a structure
	for (uint32_t queueFamily : uniqueQueueFamilies){

		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
		
	}

	//specify physical device features in use
	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE; // required for texture processing

	//specify what info the logical device uses
	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	//pointers for the array vector
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
		
		//logical device wont be created if queueCreateInfoCount is more than 1
		//static_cast<uint32_t>(queueCreateInfos.size());

	createInfo.pEnabledFeatures = &deviceFeatures;

	//parameters for creating swapchain
	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	
	//create the logical devce itself
	if (vkCreateDevice(physicalDevice , &createInfo,nullptr, &device) != VK_SUCCESS) {
		std::runtime_error("failed to create logical device");
	}

	//get ihe interface queu of the logical device
	vkGetDeviceQueue(Renderer::device,handler.graphiscFamily.value(),0,&graphicQueue);
	vkGetDeviceQueue(Renderer::device,handler.presentationFamily.value(),0,&presentationQueue);

	
}

void Renderer::createSurface() {

	//general glfw surface Impelementation
	if (glfwCreateWindowSurface(instance,window,nullptr,&surface) != VK_SUCCESS) {
		std::runtime_error("failed to create surface!");
	}

}



//search for swapchain support
Renderer::SwapChainSupportDetails Renderer::querySwapchainSupport(VkPhysicalDevice device) {

	SwapChainSupportDetails details;

	//query for surface
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	//query a list of supported surface formats
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

	if (formatCount != 0) {

		details.presentModes.resize(formatCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface , &presentModeCount, details.presentModes.data());

	}


	return details;
}

//chose what RGB format and what displaying encoding system we use
VkSurfaceFormatKHR Renderer::choseSwapChainSurfaceFormat(const std::vector<VkSurfaceFormatKHR> availableFormats) {

	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}
	return availableFormats[0];
}

// chose what order we give work and receive output to the screen
VkPresentModeKHR Renderer::choseSwapChainPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes) {

	for (const auto& availablePresntMode : availablePresentModes) {

		if (availablePresntMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresntMode;
		}

	}

	return VK_PRESENT_MODE_MAILBOX_KHR;
}

//since GLFW works with normal scrren resolution {WIDTH , HEIGHT}
//Vulkan only works with pixels so we have to specify the spawchain in pixels to allow both to interface in the same language
VkExtent2D Renderer::choseSwapExtent(const VkSurfaceCapabilitiesKHR capabilities){

;

	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}else {
		int width, height;

		glfwGetFramebufferSize(window, &width, &height);

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};
		
		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.minImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.minImageExtent.height);


		return actualExtent;
	}

}

//create the sparChain
void Renderer::createSwapChain() {
	
	Renderer::SwapChainSupportDetails swapChainSupport = querySwapchainSupport(physicalDevice);

	VkSurfaceFormatKHR surfaceFormat = choseSwapChainSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = choseSwapChainPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = choseSwapExtent(swapChainSupport.capabilities);


	//specify how many images per cycle the swapchain can take (minImageCount + 1 is standard practice) 
	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	//fill out Swapchain sturcture
	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	queueFamilies indices = queryQueueFamilies(physicalDevice);
	uint32_t queueFamilyIndices[] = { indices.graphiscFamily.value(), indices.presentationFamily.value() };

	if (indices.graphiscFamily != indices.presentationFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	//presentation families and Graphic families should be the same on most hardware

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	//when resizing we have to create a swapchain for the new size so the old one will be outdated

	if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	//image retreval for the created swap chain
	vkGetSwapchainImagesKHR(device,swapChain,&imageCount,nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;
}

//creat eimage should be extecuted after creating a swapchain
void Renderer::createImageView() {
	
	swapChainImageViews.resize(swapChainImages.size());

	//we have to create multiple image ciewers for each swapchain
	for (size_t i = 0; i < swapChainImages.size(); i++) {


		//as all objects in Vulkan it is a structure
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = swapChainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = swapChainImageFormat;

		//define chanels to use for display here we use the standard RGB format
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		//define what part of the given image should be accesed
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;
	
		//create the image viewer
		if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image viewers!");
		}

	}
}


//create pipeline for shaders
void Renderer::createGraphicsPipeline()

{

	auto vertShaderCode = readFile("shaders/vert.spv");
	auto fragShaderCode = readFile("shaders/frag.spv");

	//we store these shaders as local variables so we can fre eup the buffer at the end of their compilation and displating to the screen
	//wrapp shaders into modules
	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

	//create shaders
	VkPipelineShaderStageCreateInfo  vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	//a set of directives on how we wish to render the image
	std::vector<VkDynamicState>  dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR

	};

	//set dynamic states
	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	//set input Vertex and specify what format of the vertex data will be passed on
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{}; 

	auto bindingDescription = Verts::verts::getBindingDescription();
	auto attributeDescription = Verts::verts::getAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescription.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescription.data();

	//input assembly describes what kind of geometry should be used
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	//Region of the buffer to render to
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)swapChainExtent.width;
	viewport.height = (float)swapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	//Draw to screen in a specific form
	VkRect2D scissor{};
	scissor.offset = { 0,0 };
	scissor.extent = swapChainExtent;

	//specify dynamic state for scissor rendering format
	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	//rasterizer takes fragments from the shader to be colored
	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	//rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;//TODO facing
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;//TODO facing
	rasterizer.depthBiasEnable = VK_FALSE;
	

	//multisampling is one way of implementing anti ailasing

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	//color blending
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	//color blending controll factors
	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional

	//pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSet;


	//crate pipeline
	if (vkCreatePipelineLayout(device,&pipelineLayoutInfo,nullptr,&pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create pipeline layout!");
	}

	//create graphical pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; //optional

	if (vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&pipelineInfo,nullptr,&graphicsPipeline)!= VK_SUCCESS) {
		throw std::runtime_error("Failed to create Gaphics Pipeline");
	}

	//free buffer for further shaders
	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);

}

//We hawe to wrap raw binary data into a module shader before passing on the the Graphical pipeline
VkShaderModule Renderer::createShaderModule( std::vector<char> code) {

	VkShaderModuleCreateInfo createInfo{};
	
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device,&createInfo,nullptr,&shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module!");
	}

	return shaderModule;

}


//get the bytecode of a shader (spv) file
 std::vector<char> Renderer::readFile(const std::string fileName) {
	
	//read binary file
	std::ifstream file(fileName, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file! " + fileName);
	}

	//store size of the shader in bytes
	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	//go to the starting byte (0) and read the bytecode
	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}


 void Renderer::createRenderPass() {
	
	 //create attachment description
	 VkAttachmentDescription colorAttachment{};
	 colorAttachment.format = swapChainImageFormat;
	 colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	 colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	 colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	 colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	 colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	 colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	 colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	 //Subpasses

	 VkAttachmentReference colorAttachmentRef{};
	 colorAttachmentRef.attachment = 0;
	 colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	
	 VkSubpassDescription subpass{};
	 subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	 subpass.colorAttachmentCount = 1;
	 subpass.pColorAttachments = &colorAttachmentRef;
	 

	 VkRenderPassCreateInfo renderPassInfo{};
	 renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	 renderPassInfo.attachmentCount = 1;
	 renderPassInfo.pAttachments = &colorAttachment;
	 renderPassInfo.subpassCount = 1;
	 renderPassInfo.pSubpasses = &subpass;

	 //create subpasses
	 VkSubpassDependency dependancy{};
	 dependancy.srcSubpass = VK_SUBPASS_EXTERNAL;
	 dependancy.dstSubpass = 0;
	 dependancy.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	 dependancy.srcAccessMask = 0;
	 dependancy.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	 dependancy.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	 renderPassInfo.dependencyCount = 1;
	 renderPassInfo.pDependencies = &dependancy;

	 if (vkCreateRenderPass(device,&renderPassInfo,nullptr,&renderPass) != VK_SUCCESS){

		 throw std::runtime_error("Failed to create render pass!");
	 }

 
 }

 void Renderer::createFrameBuffers() {
	 
	 swapChainFrameBuffers.resize(swapChainImageViews.size());

	 for (size_t i = 0; i < swapChainImageViews.size(); i++) {
		 VkImageView attachments[] = { swapChainImageViews[i] };


		 VkFramebufferCreateInfo frameBufferInfo{};
		 frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		 frameBufferInfo.renderPass = renderPass;
		 frameBufferInfo.attachmentCount = 1;
		 frameBufferInfo.pAttachments = attachments;
		 frameBufferInfo.width = swapChainExtent.width;
		 frameBufferInfo.height = swapChainExtent.height;
		 frameBufferInfo.layers = 1;

		 if (vkCreateFramebuffer(device, &frameBufferInfo, nullptr, &swapChainFrameBuffers[i]) != VK_SUCCESS) {
			 throw std::runtime_error("Failed to create framebuffer!");
		 }
	 }
 }

 void Renderer::createCommandPool() {

	 queueFamilies queryFamilyIndices = queryQueueFamilies(physicalDevice);

	 VkCommandPoolCreateInfo poolInfo{};
	 poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	 poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	 poolInfo.queueFamilyIndex = queryFamilyIndices.graphiscFamily.value();

	 if (vkCreateCommandPool(device,&poolInfo,nullptr,&commandPool) != VK_SUCCESS) {
		 throw std::runtime_error("Failed to create command pool!");
	 }
 }

 void Renderer::createCommandBuffers() {
	 
	 VkCommandBufferAllocateInfo allocInfo{};

	 commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	
	 allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	 allocInfo.commandPool = commandPool;
	 allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	 allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

	 if (vkAllocateCommandBuffers(device, &allocInfo,commandBuffers.data()) != VK_SUCCESS) {
		 throw std::runtime_error("Failed to allocate command buffers!");
	 }
 }

 void Renderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {

	 VkCommandBufferBeginInfo beginInfo{};
	 beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	 beginInfo.flags = 0; // optional
	 beginInfo.pInheritanceInfo = nullptr; // optional

	 if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		 throw std::runtime_error("Failed to begin recording command buffer!");
	 }


	 //start rendering
	 VkRenderPassBeginInfo renderPassInfo{};
	 renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	 renderPassInfo.renderPass = renderPass;
	 renderPassInfo.framebuffer = swapChainFrameBuffers[imageIndex];
	 renderPassInfo.renderArea.offset = { 0,0 };
	 renderPassInfo.renderArea.extent = swapChainExtent;
	 

	 VkClearValue clearColor = { {{0.0f,0.0f,0.0f,1.0f}} };
	 renderPassInfo.clearValueCount = 1;
	 renderPassInfo.pClearValues = &clearColor;

	 vkCmdBeginRenderPass(commandBuffer , &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	 //bind graphics pipeline by giving commands to the allocated commandBuffer
	 vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

	 VkViewport viewport{};
	 viewport.x = 0.0f;
	 viewport.y = 0.0f;
	 viewport.width = (float)swapChainExtent.width;
	 viewport.height = (float)swapChainExtent.height;
	 viewport.minDepth = 0.0f;
	 viewport.maxDepth = 1.0f;

	 vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	 VkRect2D scissor{};
	 scissor.offset = { 0,0 };
	 scissor.extent = swapChainExtent;
	 vkCmdSetScissor(commandBuffer,0,1, &scissor);

	 VkBuffer vertexBuffers[] = { vertexBuffer };
	 VkDeviceSize  offsets[] = { 0 };
	 vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

	 vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
	 
	 if (hasIndexBuffer) {
		 //bind descriptor for 3D graphics
		 vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

		 vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(verticies.indicies.size()), 1, 0, 0, 0);
	 }
	 else {
		 vkCmdDraw(commandBuffer, vertexIndex,1,0,0);
	 }

	 vkCmdEndRenderPass(commandBuffer);

	 if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS){
		 throw std::runtime_error("Failed to record command buffer!");
	 }

 }

 void Renderer::drawFrame() {

	 vkWaitForFences(device, 1, &inFlightFence[currentFrame], VK_TRUE, UINT64_MAX);

	 uint32_t imageIndex;
	 VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore[currentFrame],
		 VK_NULL_HANDLE, &imageIndex);

	 if (result ==	VK_ERROR_OUT_OF_DATE_KHR) {
		 recreateSwapChain();
		 return;
	 }
	 else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		 throw std::runtime_error("Failed to acquire swap chain image!");
	 }

	 //only reset fence if we are submitting work
	 vkResetFences(device, 1, &inFlightFence[currentFrame]);

	 //record to the command buffer
	 vkResetCommandBuffer(commandBuffers[currentFrame], 0);
	 recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

	 //update uniform buffer befor submiting next frame
	 updateUniformBuffer(currentFrame);

	 //submit command buffer
	 VkSubmitInfo submitInfo{};
	 submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	 //setup semaphores
	 VkSemaphore waitSemaphores[] = { imageAvailableSemaphore[currentFrame]};
	 VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	 submitInfo.waitSemaphoreCount = 1;
	 submitInfo.pWaitSemaphores = waitSemaphores;
	 submitInfo.pWaitDstStageMask = waitStages;
	 submitInfo.commandBufferCount = 1;
	 submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
	 
	 VkSemaphore signalSemaphores[] = { renderFinishedSemaphore[currentFrame]};
	 submitInfo.signalSemaphoreCount = 1;
	 submitInfo.pSignalSemaphores = signalSemaphores;

	 if (vkQueueSubmit(graphicQueue,1,&submitInfo,inFlightFence[currentFrame]) != VK_SUCCESS) {
		 throw std::runtime_error("Failed to submit draw command to buffer!");
	 }

	 //display the image on screen
	 VkPresentInfoKHR presentInfo{};
	 presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	 presentInfo.waitSemaphoreCount = 1;
	 presentInfo.pWaitSemaphores = signalSemaphores;
	 
	 VkSwapchainKHR swapChains[] = { swapChain };
	 presentInfo.swapchainCount = 1;
	 presentInfo.pSwapchains = swapChains;
	 presentInfo.pImageIndices = &imageIndex;
	 presentInfo.pResults = nullptr; //optional


	 //handle out of date swap chains

     result = vkQueuePresentKHR(presentationQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || frameBufferResized) {
		recreateSwapChain();
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to present swap chain image!");
	}

	 currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

 }

 void Renderer::createSyncObject() {

	 imageAvailableSemaphore.resize(MAX_FRAMES_IN_FLIGHT);
	 renderFinishedSemaphore.resize(MAX_FRAMES_IN_FLIGHT);
	 inFlightFence.resize(MAX_FRAMES_IN_FLIGHT);

	 VkSemaphoreCreateInfo semaphoreInfo{};
	 semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	 VkFenceCreateInfo fenceInfo{};
	 fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	 fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	 for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		 if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore[i]) != VK_SUCCESS ||
			 vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore[i]) != VK_SUCCESS ||
			 vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence[i]) != VK_SUCCESS) {

			 throw std::runtime_error("Failed to create semaphore!");
		 }
	 }

 }

 void Renderer::recreateSwapChain() {

	 //set new window parameters
	 int width = 0, height = 0;
	 glfwGetFramebufferSize(window, &width, &height);
	 while (width == 0 || height == 0) {
		 glfwGetFramebufferSize(window, &width, &height);
		 glfwWaitEvents();
	 }


	 //waits untill all reseources all no longer used
	 vkDeviceWaitIdle(device);

	 //create new swapchain for new window
	 cleanupSwapChain();
	 createSwapChain();
	 createImageView();
	 createFrameBuffers();
 }

 void Renderer::cleanupSwapChain() {
	 //frame buffers
	 for (auto framebuffer : swapChainFrameBuffers) {
		 vkDestroyFramebuffer(device, framebuffer, nullptr);
	 }
	 //delete all created image viewers
	 for (auto imageViewer : swapChainImageViews) {
		 vkDestroyImageView(device, imageViewer, nullptr);
	 }

	 vkDestroySwapchainKHR(device, swapChain, nullptr);
 }

  void Renderer::framebufferResizeCallback(GLFWwindow* window, int width, int height) {

	  auto app = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
	  app->frameBufferResized = true;

 }

 

  //create a buffer for vertex input to be displayed on the screen
  void Renderer::createVertexBuffer(Verts verts) {

	  VkDeviceSize bufferSize = sizeof(verts.verticies[0]) * verts.verticies.size();

	  //create and load in staging buffer
	  VkBuffer stagingBuffer;
	  VkDeviceMemory stagingBufferMemory;
	  createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	  //copy vertex data to GPU from staging buffer
	  void* data;
	  vkMapMemory(device,stagingBufferMemory,0,bufferSize,0,&data);
	  memcpy(data, verts.verticies.data(), (size_t)bufferSize);
	  vkUnmapMemory(device, stagingBufferMemory);

	  //load data from staging memory to GPU memory
	  createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,vertexBuffer,vertexBufferMemory);
	  copyBuffer(stagingBuffer,vertexBuffer,bufferSize);
	
	  //cleanup temporary buffers
	  vkDestroyBuffer(device,stagingBuffer,nullptr);
	  vkFreeMemory(device,stagingBufferMemory,nullptr);

  };

  //query for available types of memory
  uint32_t Renderer::findMemoryType(uint32_t type, VkMemoryPropertyFlags properties) {

	  VkPhysicalDeviceMemoryProperties memProperties;
	  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		  if (type & (1 << i) && ((memProperties.memoryTypes[i].propertyFlags & properties) == properties)) {
			  return i;
		  }
	  }
	  

	  throw std::runtime_error("Failed to find suitable memory type!");
  }


  //shader verticies are rendered in an order
  void Renderer::createIndexBuffer(Verts verts) {

	  VkDeviceSize bufferSize = sizeof(verts.indicies[0]) * verts.indicies.size();
	  
	  //create staging buffer
	  VkBuffer stagingBuffer;
	  VkDeviceMemory stagingBufferMemory;

	  createBuffer(bufferSize,VK_BUFFER_USAGE_TRANSFER_SRC_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,stagingBuffer,stagingBufferMemory);
	  
	  void* data;
	  vkMapMemory(device,stagingBufferMemory,0,bufferSize,0,&data);
	  memcpy(data, verts.indicies.data(), (size_t)bufferSize);
	  vkUnmapMemory(device,stagingBufferMemory);

	  createBuffer(bufferSize,VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,indexBuffer,indexBuffermemory);

	  copyBuffer(stagingBuffer,indexBuffer,bufferSize);

	  vkDestroyBuffer(device,stagingBuffer,nullptr);
	  vkFreeMemory(device, stagingBufferMemory, nullptr);
  }

  //staging helper function
  void Renderer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory){

	  VkBufferCreateInfo bufferInfo{};
	  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	  bufferInfo.size = size;
	  bufferInfo.usage = usage;
	  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	  if (vkCreateBuffer(device,&bufferInfo,nullptr,&buffer)!= VK_SUCCESS) {
		  throw std::runtime_error("Failed to create buffer!");
	  }

	  VkMemoryRequirements memRequirements;
	  vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

	  VkMemoryAllocateInfo allocInfo{};
	  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	  allocInfo.allocationSize = memRequirements.size;
	  allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	  if (vkAllocateMemory(device,&allocInfo,nullptr,&bufferMemory) != VK_SUCCESS) {
		  throw std::runtime_error("Failed to allocate buffer memory!");
	  }

	  vkBindBufferMemory(device, buffer, bufferMemory, 0);
  }

  void Renderer::copyBuffer(VkBuffer sourceBuffer, VkBuffer destinationBuffer, VkDeviceSize size ) {
	

	  VkCommandBuffer commandbuffer = textureLoadStart();

	  VkBufferCopy copyRegion{};
	  copyRegion.size = size;
	  vkCmdCopyBuffer(commandbuffer, sourceBuffer, destinationBuffer, 1, &copyRegion);


	  textureLoadEnd(commandbuffer);
  }

  //load raw image into pixels
  void Renderer::createTexture()
  {

	  int width, height, channels;
	  stbi_uc* pixel = stbi_load("textures/mr_bean.png", &width, &height, &channels, STBI_rgb_alpha);
	  VkDeviceSize imgSize = width * height * 4;

	  if (!pixel) {
		  throw std::runtime_error("Failed to load texture!");
	  }

	  //setup staging
	  VkBuffer stagingBuffer;
	  VkDeviceMemory stagingBufferMemory;

	  createBuffer(imgSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	  //load image pixels directly
	  void* data;
	  vkMapMemory(device, stagingBufferMemory, 0, imgSize, 0, &data);
	  memcpy(data, pixel, static_cast<size_t>(imgSize));
	  vkUnmapMemory(device, stagingBufferMemory);

	  //cleanup
	  stbi_image_free(pixel);

	  createImage(width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture, textureMemory);

	  transitionTextureLayout(texture, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	  
	 
	  copyBufferToTexture(stagingBuffer, texture, static_cast<uint32_t>(width), static_cast<uint32_t>(height));

	  transitionTextureLayout(texture, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	 
	  vkDestroyBuffer(device, stagingBuffer, nullptr);
	  vkFreeMemory(device, stagingBufferMemory, nullptr);
  }

  //recreate image from given data
  void Renderer::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& texture, VkDeviceMemory& textureMemory) {


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
	  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	 
	  if (vkCreateImage(device,&imageInfo,nullptr,&texture) != VK_SUCCESS) {
		  throw std::runtime_error("Failed to create image!");
	  }

	  //allocating memory for image
	  VkMemoryRequirements requirements;
	  vkGetImageMemoryRequirements(device, texture, &requirements);

	  VkMemoryAllocateInfo allocationInfo{};
	  allocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	  allocationInfo.allocationSize = requirements.size;
	  allocationInfo.memoryTypeIndex = findMemoryType(requirements.memoryTypeBits, properties);
	  
	  if (vkAllocateMemory(device,&allocationInfo,nullptr,&textureMemory) != VK_SUCCESS) {
		  throw std::runtime_error("Failed to allocate memory for texture!");
	  }

	  vkBindImageMemory(device, texture, textureMemory,0);
  }

  //texture buffer layout start
  VkCommandBuffer Renderer::textureLoadStart() {
	  
	  VkCommandBufferAllocateInfo allocationInfo{};

	  allocationInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	  allocationInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	  allocationInfo.commandPool = commandPool;
	  allocationInfo.commandBufferCount = 1;
	  
	  VkCommandBuffer commandBuffer;
	  vkAllocateCommandBuffers(device, &allocationInfo, &commandBuffer);

	  //start recording
	  VkCommandBufferBeginInfo startInfo{};
	  startInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	  startInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	  vkBeginCommandBuffer(commandBuffer, &startInfo);

	  return commandBuffer;
	  
  }

  //texture buffer layout end
  void Renderer::textureLoadEnd(VkCommandBuffer commandBuffer) {
	  
	  vkEndCommandBuffer(commandBuffer);

	  VkSubmitInfo submitInfo{};
	  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	  submitInfo.commandBufferCount = 1;
	  submitInfo.pCommandBuffers = &commandBuffer;

	  vkQueueSubmit(graphicQueue, 1, &submitInfo, VK_NULL_HANDLE);
	  vkQueueWaitIdle(graphicQueue);

	  vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
  }

  void Renderer::transitionTextureLayout(VkImage texture, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
	  
	  VkCommandBuffer commandBuffer = textureLoadStart();

	  //use image memory barrier for synchronization
	  VkImageMemoryBarrier barrier{};
	  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	  barrier.oldLayout = oldLayout;
	  barrier.newLayout = newLayout;
	  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	  barrier.image = texture;
	  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	  barrier.subresourceRange.baseMipLevel = 0;
	  barrier.subresourceRange.levelCount = 1;
	  barrier.subresourceRange.baseArrayLayer = 0;
	  barrier.subresourceRange.layerCount = 1;
	  VkPipelineStageFlags sourceStage;
	  VkPipelineStageFlags destinationStage;

	  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		  barrier.srcAccessMask = 0;
		  barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		  sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		  destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	  }
	  else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		  sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		  destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	  }
	  else {
		  throw std::invalid_argument("unsupported layout transition!");
	  }

	  vkCmdPipelineBarrier(
		  commandBuffer,
		  sourceStage, destinationStage,
		  0,
		  0, nullptr,
		  0, nullptr,
		  1, &barrier
	  );

	  textureLoadEnd(commandBuffer);
  }


  void Renderer::copyBufferToTexture(VkBuffer buffer, VkImage texture, uint32_t width, uint32_t height) {

	  VkCommandBuffer commandBuffer = textureLoadStart();

	  VkBufferImageCopy region{};
	  region.bufferOffset = 0;
	  region.bufferRowLength = 0;
	  region.bufferImageHeight = 0;
	  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	  region.imageSubresource.mipLevel = 0;
	  region.imageSubresource.baseArrayLayer = 0;
	  region.imageSubresource.layerCount = 1;

	  region.imageOffset = { 0, 0, 0 };
	  region.imageExtent = {width,height,1};

	  vkCmdCopyBufferToImage(commandBuffer,buffer, texture,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&region );
	  //vkCmdCopyImageToBuffer(commandBuffer,image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,nullptr,1,&region);

	  textureLoadEnd(commandBuffer);
  }

  //tell vulkan how to acces 3D shaders
  void Renderer::createDescriptionSetLayout() {

	  VkDescriptorSetLayoutBinding uniformLayoutBinding{};
	  uniformLayoutBinding.binding = 0;
	  uniformLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	  uniformLayoutBinding.descriptorCount = 1;

	  uniformLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	  uniformLayoutBinding.pImmutableSamplers = nullptr;

	  VkDescriptorSetLayoutBinding samplerLayout{};
	  samplerLayout.binding = 1;
	  samplerLayout.descriptorCount = 1;
	  samplerLayout.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	  samplerLayout.pImmutableSamplers = nullptr;
      samplerLayout.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	  

	  std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uniformLayoutBinding , samplerLayout };

	  VkDescriptorSetLayoutCreateInfo layoutInfo{};

	  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	  layoutInfo.pBindings = bindings.data();

	  if (vkCreateDescriptorSetLayout(device,&layoutInfo,nullptr,&descriptorSet) != VK_SUCCESS) {
		  throw std::runtime_error("Failed to create descriptor set!");
	  }
  }

  void Renderer::createUniformBuffers() {
	  
	  VkDeviceSize bufferSize = sizeof(UniformBufferObj::UniformBufferObject);

	  uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	  uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
	  uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

	  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		  createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);

		  vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
	  }
  }


  void Renderer::createDescriptorPool() {

	  std::array<VkDescriptorPoolSize,2> poolSize{};
	  poolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	  poolSize[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	  poolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	  poolSize[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	  VkDescriptorPoolCreateInfo poolInfo{};
	  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSize.size());
	  poolInfo.pPoolSizes = poolSize.data();
	  poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	  if (vkCreateDescriptorPool(device,&poolInfo,nullptr,&descriptorPool) != VK_SUCCESS) {
		  throw std::runtime_error("failed to create descriptor pool!");
	  }
  }

  //Descriptors describe how the object is to be drawn to the screen
  void Renderer::createDescriptorSet() {

	  std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSet);

	  VkDescriptorSetAllocateInfo allocationInfo{};

	  allocationInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	  allocationInfo.descriptorPool = descriptorPool;
	  allocationInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	  allocationInfo.pSetLayouts = layouts.data();
	  descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

	  if (vkAllocateDescriptorSets(device,&allocationInfo, descriptorSets.data()) != VK_SUCCESS) {
		  throw std::runtime_error("Failed to allocate descriptor sets!");
	  }

	  //populate descriptor sets
	  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		  
		  VkDescriptorBufferInfo bufferInfo{};
		  bufferInfo.buffer = uniformBuffers[i];
		  bufferInfo.offset = 0;
		  bufferInfo.range = sizeof(UniformBufferObj::UniformBufferObject);


		  VkDescriptorImageInfo imgInfo{};
		  imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		  imgInfo.imageView = textureView;
		  imgInfo.sampler = textureSampler;

		  std::array<VkWriteDescriptorSet, 2> descriptorWrite{};

		  descriptorWrite[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		  descriptorWrite[0].dstSet = descriptorSets[i];
		  descriptorWrite[0].dstBinding = 0;
		  descriptorWrite[0].dstArrayElement = 0;
		  descriptorWrite[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		  descriptorWrite[0].descriptorCount = 1;
		  descriptorWrite[0].pBufferInfo = &bufferInfo;

		  descriptorWrite[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		  descriptorWrite[1].dstSet = descriptorSets[i];
		  descriptorWrite[1].dstBinding = 1;
		  descriptorWrite[1].dstArrayElement = 0;
		  descriptorWrite[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		  descriptorWrite[1].descriptorCount = 1;
		  descriptorWrite[1].pImageInfo = &imgInfo;
		
		  vkUpdateDescriptorSets(device,static_cast<uint32_t>(descriptorWrite.size()),descriptorWrite.data(), 0, nullptr);
	  
	  }

  }

  //a helper function so multaple Textures can be processed at once
  VkImageView Renderer::createTextureView(VkImage texture, VkFormat format) {

	  VkImageViewCreateInfo viewInfo{};
	  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	  viewInfo.image = texture;
	  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	  viewInfo.format = format;
	  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	  viewInfo.subresourceRange.baseMipLevel = 0;
	  viewInfo.subresourceRange.levelCount = 1;
	  viewInfo.subresourceRange.baseArrayLayer = 0;
	  viewInfo.subresourceRange.layerCount = 1;
	  
	  VkImageView textureImg;
	  if (vkCreateImageView(device,&viewInfo,nullptr,&textureImg) != VK_SUCCESS) {
		  throw std::runtime_error("Failed to create Texture View!");
	  }

	  return textureImg;
  }

  //create image view to load onto a surface
  void Renderer::createTextureImage() {
	  textureView = createTextureView(texture, VK_FORMAT_R8G8B8A8_SRGB);
  }

  void Renderer::createTextureImageViews() {

	  swapChainImageViews.resize(swapChainImages.size());

	  for (uint32_t i = 0; i < swapChainImages.size(); i++) {
		  swapChainImageViews[i] = createTextureView(swapChainImages[i], swapChainImageFormat);
	  }

  }

  void Renderer::createTextureSampler() {

	  VkSamplerCreateInfo sampleInfo{};
	  sampleInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	  sampleInfo.magFilter = VK_FILTER_LINEAR;
	  sampleInfo.minFilter = VK_FILTER_LINEAR;

	  //TODO: how the Texture is being handeled (mirroring, repeats, clamp to edge an dother such transformations)
	  sampleInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT; // axis X
	  sampleInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT; // axis Y
	  sampleInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT; // axis Z

	  sampleInfo.anisotropyEnable = VK_TRUE;

	  //use smoothing based on GPU capabilities
	  VkPhysicalDeviceProperties  properties{};
	  vkGetPhysicalDeviceProperties(physicalDevice, &properties);
	  sampleInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

	  sampleInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE; // TODO: border color of texture
	  sampleInfo.unnormalizedCoordinates = VK_FALSE;// what coordinate system we use to shift textures on the surface current(0 to 1)
	  
	  sampleInfo.compareEnable = VK_FALSE;
	  sampleInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	  sampleInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	  sampleInfo.mipLodBias = 0.0f;
	  sampleInfo.minLod = 0.0f;
	  sampleInfo.maxLod = 0.0f;

	  if (vkCreateSampler(device,&sampleInfo,nullptr,&textureSampler) != VK_SUCCESS) {
		  throw std::runtime_error("Failed to create texture sampler!");
	  }

  }

  //requires matrix and chrono libraris , standardly they should be packed with object they render
  void Renderer::updateUniformBuffer(uint32_t currentFrame) {

	  static auto startTime = std::chrono::high_resolution_clock::now();
	  auto currentTime = std::chrono::high_resolution_clock::now();

	  //calculate the distance traveled since the start of the rendering process
	  float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	  //program the 3D model
	  UniformBufferObj::UniformBufferObject RenderModel{};

	  RenderModel.model = glm::rotate(glm::mat4(0.1f), time * glm::radians(5.0f), glm::vec3(0.0f, 0.0f, 0.1f));
	  RenderModel.view = glm::lookAt(glm::vec3(1.0f, 2.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	  RenderModel.proj = glm::perspective(glm::radians(45.0f),swapChainExtent.width / (float) swapChainExtent.height,0.5f,10.0f);
	  RenderModel.proj[1][1] *= -1;// since GLM was originaly intendet for openGL we have to invert the Y coordinates

	  //copy the transformation data to buffer
	  memcpy(uniformBuffersMapped[currentFrame], &RenderModel, sizeof(RenderModel));

  }
