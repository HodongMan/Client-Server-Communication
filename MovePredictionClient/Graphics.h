#pragma once


struct LinearAllocator;
struct Matrix4x4;


class GrpahicsState
{
public:
	static constexpr int32_t			MAX_FRAMES_IN_FLIGHT		= 3;
public:
	GrpahicsState( void ) noexcept;

	bool								initialize( HWND windowHandle, HINSTANCE hInstance, int32_t windowWidth, int32_t windowHeight, int32_t maxPlayer, LinearAllocator* allocator, LinearAllocator* tempAllocator ) noexcept;
	void								updateAndDraw( Matrix4x4* matrices, int32_t numPlayers ) noexcept; 
private:
	VkDevice							_device						= VK_NULL_HANDLE;

	VkQueue								_graphicsQueue				= VK_NULL_HANDLE;
	VkQueue								_presentQueue				= VK_NULL_HANDLE;

	VkSwapchainKHR						_swapChain					= VK_NULL_HANDLE;

	VkSemaphore							_imageAvailableSemaphore	= VK_NULL_HANDLE;
	VkSemaphore							_renderFinishedSemaphore	= VK_NULL_HANDLE;

	VkCommandPool*						_commandPools				= nullptr;
	VkCommandBuffer*					_commandBuffers				= nullptr;
	bool*								_comaandBufferInUse			= nullptr;
	VkFence*							_fences						= nullptr;

	VkSemaphore							_imageAvailableSemaphores[ MAX_FRAMES_IN_FLIGHT ]	= {};
	VkSemaphore							_renderFinishedSemaphores[ MAX_FRAMES_IN_FLIGHT ]	= {};
	VkFence								_inFlightFences[ MAX_FRAMES_IN_FLIGHT ]				= {};
	int32_t								_currentFrame										= 0;
	VkFence*							_imagesInFlight				= nullptr; // swapchain 이미지 개수만큼

	VkBuffer							_matrixBuffer				= VK_NULL_HANDLE;
	VkDeviceMemory						_matrixBufferMemory			= VK_NULL_HANDLE;
	int32_t								_matrixBufferPaddingBytes	= 0;

	VkBuffer							_cubeVertexBuffer			= VK_NULL_HANDLE;
	VkBuffer							_cubeIndexBuffer			= VK_NULL_HANDLE;
	int32_t								_cubeNumIndices				= 0;

	VkBuffer							_sceneryVertexBuffer		= VK_NULL_HANDLE;
	VkBuffer							_sceneryIndexBuffer			= VK_NULL_HANDLE;
	int32_t								_sceneryNumIndices			= 0;

	VkRenderPass						_renderPass					= VK_NULL_HANDLE;
	VkFramebuffer*						_swapchainFramebuffer		= nullptr;
	VkExtent2D							_swapchainExtent;

	VkPipelineLayout					_pipelineLayout				= VK_NULL_HANDLE;
	VkPipeline							_graphicsPipeline			= VK_NULL_HANDLE;
	VkDescriptorSet						_descriptorSet				= VK_NULL_HANDLE;

};