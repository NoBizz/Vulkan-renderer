

#include <array>
#include <glm/glm.hpp>
#include <vector>
#include <string>

#define VK_USE_PLATFORM_WIN32_KHR // Windows specific
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>


class Verts {

public:

	const std::string imgPath = "textures/mr_bean.png";

	struct verts {

		glm::vec3 pos; // position of verticies
		glm::vec3 color; // color of verticies
		glm::vec2 texture; // texture coordinates

		//tell vulkan how to pass vertex shader after upload
		static VkVertexInputBindingDescription getBindingDescription() {

			VkVertexInputBindingDescription bindingDescription{};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(verts);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		//how to handle vertex input
		static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

			//Verticies pos
			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;;
			attributeDescriptions[0].offset = offsetof(verts, pos);

			//color
			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(verts, color);

			//texture pos
			attributeDescriptions[2].binding = 0;
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[2].offset = offsetof(verts, texture);


			return attributeDescriptions;
		}

	};

	//verticies and indices to be rendered
	const std::vector<verts> verticies = {

		//position (vec3)      // color (vec3) //texture (vec2)
		{{-1.0f, -1.0f,0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f,0.0f}},
		{{1.0f, -1.0f,0.0f}, {0.0f, 1.0f, 0.0f} ,  {0.0f,0.0f}},
		{{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f},   {0.0f,1.0f}},
		{{-1.0f, 1.0f,0.0f}, {1.0f, 1.0f, 1.0f},   {1.0f,1.0f}},

	{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
	{{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
	{{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
	{{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}

	};

	const std::vector<uint32_t> indicies = { 0,1,2,2,3,0, 4, 5, 6, 6, 7, 4 };
	


	
};
