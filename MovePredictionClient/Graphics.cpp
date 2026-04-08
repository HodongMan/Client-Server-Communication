#include "pch.h"
#include "Graphics.h"

#include "Vertex.h"
#include "../MovePrediction/Math.h"
#include "../MovePrediction/Logger.h"
#include "../MovePrediction/LinearAllocator.h"


static VkBool32 VKAPI_PTR vulkanDebugCallback( VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT /*objType*/, uint64_t /*obj*/,  size_t /*location*/, int32_t /*code*/, const char* layerPrefix, const char* message, void* /*userData*/ ) noexcept
{
	log( "[ graphics::vulkan::%s ] %s\n", layerPrefix, message );
	if ( flags & ~( VK_DEBUG_REPORT_INFORMATION_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT ) )
	{
		//DebugBreak();
	}

	return VK_FALSE;
}

static bool createBuffer( VkPhysicalDevice physicalDevice, VkDevice device, VkBufferUsageFlags bufferUsageFlags, int32_t size, _Out_ VkBuffer* outBuffer, _Out_ VkDeviceMemory* outBufferMemory ) noexcept
{
	HDASSERT( nullptr != outBuffer, "VkBuffer is nullptr " );
	HDASSERT( nullptr != outBufferMemory, "VkDeviceMemory is nullptr " );

	VkBufferCreateInfo bufferCreateInfo						= {};
	bufferCreateInfo.sType									= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size									= size;
	bufferCreateInfo.usage									= bufferUsageFlags;
	bufferCreateInfo.sharingMode							= VK_SHARING_MODE_EXCLUSIVE;

	VkBuffer buffer;
	VkResult result											= vkCreateBuffer( device, &bufferCreateInfo, 0, &buffer );
	if ( VK_SUCCESS != result )
	{
		HDASSERT( false, "vkCreateBuffer Error " );
		return false;
	}

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements( device, buffer, &memoryRequirements );

	VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
	vkGetPhysicalDeviceMemoryProperties( physicalDevice, &physicalDeviceMemoryProperties );

	constexpr uint32_t requiredMemoryProperties				= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	int32_t chosenMemoryTypeIndex							= -1;
	for ( int32_t ii = 0; ii < physicalDeviceMemoryProperties.memoryTypeCount; ++ii )
	{
		if ( ( memoryRequirements.memoryTypeBits & ( 1 << ii ) ) &&
			 ( physicalDeviceMemoryProperties.memoryTypes[ ii ].propertyFlags & requiredMemoryProperties ) == requiredMemoryProperties )
		{
			chosenMemoryTypeIndex							= ii;
			break;
		}
	}

	HDASSERT( -1 != chosenMemoryTypeIndex, "Cannot find memoryTypeCount in physicalDeviceMemoryProperties" );

	VkMemoryAllocateInfo memoryAllocateInfo					= {};
	memoryAllocateInfo.sType								= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize						= memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex						= chosenMemoryTypeIndex;

	VkDeviceMemory bufferMemory								= {};
	result													= vkAllocateMemory( device, &memoryAllocateInfo, 0, &bufferMemory );
	if ( VK_SUCCESS != result )
	{
		HDASSERT( false, "vkAllocateMemory Error " );
		return false;
	}

	vkBindBufferMemory( device, buffer, bufferMemory, 0 );
	*outBuffer												= buffer;
	*outBufferMemory										= bufferMemory;

	return true;
}

static bool copyToBuffer( VkDevice device, VkDeviceMemory bufferMemory, void* src, int32_t size ) noexcept
{
	void* dst												= nullptr;
	VkResult result											= vkMapMemory( device, bufferMemory, 0, size, 0, &dst );
	if ( VK_SUCCESS != result )
	{
		HDASSERT( false, "vkMapMemory Error " );
		return false;
	}

	memcpy( dst, src, size );
	vkUnmapMemory( device, bufferMemory );

	return true;
}

static void createCubeFace( Vertex* vertices, int32_t vertexOffset, uint16_t* indices, int32_t indexOffset, Vector3 center, Vector3 right, Vector3 up, Vector3 color ) noexcept
{
	HDASSERT( nullptr != vertices, "vertices is nullptr" );
	HDASSERT( 0 <= vertexOffset, "vertexOffset is invalid data" );

	Vector3 halfRight										= right;
	halfRight.mul( 0.5f );
	
	Vector3 halfUp											= up;
	halfUp.mul( 0.5f );

	Vector3 position										= center;
	position.sub( halfRight );
	position.add( halfUp );

	vertices[ vertexOffset ]._position						= position;
	vertices[ vertexOffset ]._color							= color;

	position												= center;
	position.add( halfRight );
	position.add( halfUp );

	vertices[ vertexOffset + 1 ]._position					= position;
	vertices[ vertexOffset + 1 ]._color						= color;

	position												= center;
	position.add( halfRight );
	position.sub( halfUp );

	vertices[ vertexOffset + 2 ]._position					= position;
	vertices[ vertexOffset + 2 ]._color						= color;

	position												= center;
	position.sub( halfRight );
	position.sub( halfUp );

	vertices[ vertexOffset + 3 ]._position					= position;
	vertices[ vertexOffset + 3 ]._color						= color;

	indices[ indexOffset ]									= vertexOffset;
	indices[ indexOffset + 1 ]								= vertexOffset + 1;
	indices[ indexOffset + 2 ]								= vertexOffset + 2;
	indices[ indexOffset + 3 ]								= vertexOffset;
	indices[ indexOffset + 4 ]								= vertexOffset + 2;
	indices[ indexOffset + 5 ]								= vertexOffset + 3;

}

GrpahicsState::GrpahicsState( void ) noexcept
{

}

bool GrpahicsState::initialize( HWND windowHandle, HINSTANCE hInstance, int32_t windowWidth, int32_t windowHeight, int32_t maxPlayer, LinearAllocator* allocator, LinearAllocator* subAllocator ) noexcept
{
	char appName[64];
	sprintf_s( appName, "MovePredictionClient_%d", GetCurrentProcessId() );

	VkApplicationInfo appilcationInfo						= {};
	appilcationInfo.sType									= VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appilcationInfo.pApplicationName						= appName;
	appilcationInfo.apiVersion								= VK_API_VERSION_1_0;

	VkInstanceCreateInfo instanceCreateInfo					= {};
	instanceCreateInfo.sType								= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pApplicationInfo						= &appilcationInfo;

#ifndef RELEASE
	instanceCreateInfo.enabledLayerCount					= 1;
	const char* validationLayers[]							= { "VK_LAYER_KHRONOS_validation" };
	instanceCreateInfo.ppEnabledLayerNames					= validationLayers;
#endif

	const char* enabledExtensionNames[]						= { VK_EXT_DEBUG_REPORT_EXTENSION_NAME, VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
	instanceCreateInfo.enabledExtensionCount				= sizeof( enabledExtensionNames ) / sizeof( enabledExtensionNames[ 0 ] );
	instanceCreateInfo.ppEnabledExtensionNames				= enabledExtensionNames;

	//SetEnvironmentVariableA("DISABLE_LAYER_AMD_SWITCHABLE_GRAPHICS_1", "1");

	VkInstance vkInstance									= VK_NULL_HANDLE;
	VkResult result											= vkCreateInstance( &instanceCreateInfo, nullptr, &vkInstance );
	if ( VK_SUCCESS != result )
	{
		HDASSERT( false, "vkCreateInstance Error " );
		return false;
	}

	VkDebugReportCallbackCreateInfoEXT debugCallbackCreateInfo	= {};
	debugCallbackCreateInfo.sType							= VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	debugCallbackCreateInfo.flags							= VK_DEBUG_REPORT_INFORMATION_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT;
	debugCallbackCreateInfo.pfnCallback						= vulkanDebugCallback;

	PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT	= ( PFN_vkCreateDebugReportCallbackEXT )vkGetInstanceProcAddr( vkInstance, "vkCreateDebugReportCallbackEXT" );
	HDASSERT( nullptr != vkCreateDebugReportCallbackEXT, "PFN_vkCreateDebugReportCallbackEXT call error" );

	VkDebugReportCallbackEXT debugCallback;
	result													= vkCreateDebugReportCallbackEXT( vkInstance, &debugCallbackCreateInfo, 0, &debugCallback );
	if ( VK_SUCCESS != result )
	{
		HDASSERT( false, "vkCreateDebugReportCallbackEXT Error " );
		return false;
	}

	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo			= {};
	surfaceCreateInfo.sType									= VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.hwnd									= windowHandle;
	surfaceCreateInfo.hinstance								= hInstance;

	PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR		= ( PFN_vkCreateWin32SurfaceKHR )vkGetInstanceProcAddr( vkInstance, "vkCreateWin32SurfaceKHR" );
	HDASSERT( nullptr != vkCreateWin32SurfaceKHR, "PFN_vkCreateWin32SurfaceKHR call error" );

	VkSurfaceKHR surface;
	result													= vkCreateWin32SurfaceKHR( vkInstance, &surfaceCreateInfo, 0, &surface );
	if ( VK_SUCCESS != result )
	{
		HDASSERT( false, "vkCreateWin32SurfaceKHR Error " );
		return false;
	}

	uint32_t physicalDeviceCount							= 0;
	result													= vkEnumeratePhysicalDevices( vkInstance, &physicalDeviceCount, nullptr );
	if ( VK_SUCCESS != result )
	{
		HDASSERT( false, "vkEnumeratePhysicalDevices Error " );
		return false;
	}
	
	HDASSERT( 0 < physicalDeviceCount, "physicalDeviceCount is zero" );

	VkPhysicalDevice* physicalDevice						= ( VkPhysicalDevice* )subAllocator->alloc( sizeof( VkPhysicalDevice ) * physicalDeviceCount );
	HDASSERT( nullptr != physicalDevice, "allocator VkPhysicalDevice alloc error" );

	result													= vkEnumeratePhysicalDevices( vkInstance, &physicalDeviceCount, physicalDevice );
	if ( VK_SUCCESS != result )
	{
		HDASSERT( false, "vkEnumeratePhysicalDevices Error " );
		return false;
	}

	VkPhysicalDevice chosenPhysicalDevice					= VK_NULL_HANDLE;
	VkPhysicalDeviceProperties chosenPhysicalDeviceProperties	= {};
	VkSurfaceFormatKHR swapChainSurfaceFormat				= {};
	VkPresentModeKHR swapChainPresentMode					= VK_PRESENT_MODE_FIFO_KHR;
	
	uint32_t swapChainImageCount							= 0;
	
	for ( uint32_t ii = 0; ii < physicalDeviceCount; ++ii )
	{
		VkPhysicalDeviceProperties deviceProperties;

		vkGetPhysicalDeviceProperties( physicalDevice[ ii ], &deviceProperties );

		if ( VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU == deviceProperties.deviceType )
		{
			uint32_t extensionCount							= 0;
			result											= vkEnumerateDeviceExtensionProperties( physicalDevice[ ii ], nullptr, &extensionCount, nullptr );
			HDASSERT( VK_SUCCESS == result, "vkEnumerateDeviceExtensionProperties VK ERROR" );
			HDASSERT( 0 < extensionCount, "vkEnumerateDeviceExtensionProperties extension count error" );

			VkExtensionProperties* deviceExtensions			= ( VkExtensionProperties* )subAllocator->alloc( sizeof( VkExtensionProperties ) * extensionCount );
			HDASSERT( nullptr != deviceExtensions, "VkExtensionProperties alloc failed" );

			result											= vkEnumerateDeviceExtensionProperties( physicalDevice[ ii ], nullptr, &extensionCount, deviceExtensions );
			HDASSERT( VK_SUCCESS == result, "vkEnumerateDeviceExtensionProperties VK ERROR" );
			HDASSERT( 0 < extensionCount, "vkEnumerateDeviceExtensionProperties extension count error" );
			
			for ( uint32_t jj = 0; jj < extensionCount; ++jj )
			{
				if ( 0 == strcmp( deviceExtensions[ jj ].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME ) )
				{
					chosenPhysicalDevice					= physicalDevice[ ii ];
					chosenPhysicalDeviceProperties			= deviceProperties;
					break;
				}
			}

			if ( nullptr != chosenPhysicalDevice )
			{
				uint32_t formatCount						= 0;
				uint32_t presentModeCount					= 0;

				result										= vkGetPhysicalDeviceSurfaceFormatsKHR( chosenPhysicalDevice, surface, &formatCount, nullptr );
				HDASSERT( VK_SUCCESS == result, "vkGetPhysicalDeviceSurfaceFormatsKHR VK ERROR" );

				result										= vkGetPhysicalDeviceSurfacePresentModesKHR( chosenPhysicalDevice, surface, &presentModeCount, nullptr );
				HDASSERT( VK_SUCCESS == result, "vkGetPhysicalDeviceSurfacePresentModesKHR VK ERROR" );

				if ( 0 < formatCount && 0 < presentModeCount )
				{
					VkSurfaceFormatKHR* surfaceFormats		= ( VkSurfaceFormatKHR* )subAllocator->alloc( sizeof( VkSurfaceFormatKHR ) * formatCount );
					HDASSERT( nullptr != surfaceFormats, "VkSurfaceFormatKHR alloc ERROR" );

					result									= vkGetPhysicalDeviceSurfaceFormatsKHR( chosenPhysicalDevice, surface, &formatCount, surfaceFormats );
					HDASSERT( VK_SUCCESS == result, "vkGetPhysicalDeviceSurfaceFormatsKHR VK ERROR" );

					if ( 1 == formatCount && VK_FORMAT_UNDEFINED == surfaceFormats[ 0 ].format )
					{
						swapChainSurfaceFormat.format		= VK_FORMAT_R8G8B8A8_UNORM;
						swapChainSurfaceFormat.colorSpace	= VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
					}
					else
					{
						swapChainSurfaceFormat				= surfaceFormats[ 0 ];
					}

					VkPresentModeKHR* presentModes			= ( VkPresentModeKHR* )subAllocator->alloc( sizeof( VkPresentModeKHR ) * presentModeCount );
					HDASSERT( nullptr != presentModes, "VkPresentModeKHR alloc ERROR" );

					result									= vkGetPhysicalDeviceSurfacePresentModesKHR( chosenPhysicalDevice, surface, &presentModeCount, presentModes );
					HDASSERT( VK_SUCCESS == result, "vkGetPhysicalDeviceSurfacePresentModesKHR VK ERROR" );

					for ( uint32_t jj = 0; jj < presentModeCount; ++jj )
					{
						if ( VK_PRESENT_MODE_MAILBOX_KHR == presentModes[ jj ] )
						{
							swapChainPresentMode			= VK_PRESENT_MODE_MAILBOX_KHR;
						}
					}

					VkSurfaceCapabilitiesKHR surfaceCapabilities	= {};
					result									= vkGetPhysicalDeviceSurfaceCapabilitiesKHR( chosenPhysicalDevice, surface, &surfaceCapabilities );
					HDASSERT( VK_SUCCESS == result, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR VK ERROR" );

					if ( 0xFFFFFFFF == surfaceCapabilities.currentExtent.width )
					{
						_swapchainExtent					= { ( uint32_t )windowWidth, ( uint32_t )windowHeight };
					}
					else
					{
						_swapchainExtent					= surfaceCapabilities.currentExtent;
					}

					if ( 0 == surfaceCapabilities.maxImageCount || 3 <= surfaceCapabilities.maxImageCount )
					{
						swapChainImageCount					= 3;
					}
					else
					{
						swapChainImageCount					= surfaceCapabilities.maxImageCount;
					}
				}
				else
				{
					chosenPhysicalDevice					= nullptr;
				}				
			}
		}
		else if ( VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU == deviceProperties.deviceType )
		{
			HDASSERT( false, "혹시 모를 예외를..." );
		}

		if ( nullptr == chosenPhysicalDevice )
		{
			break;
		}
	}

	HDASSERT( nullptr != chosenPhysicalDevice, "can not choise physical device" );

	uint32_t queueFamilyCount								= 0;
	vkGetPhysicalDeviceQueueFamilyProperties( chosenPhysicalDevice, &queueFamilyCount, nullptr );
	assert( 0 < queueFamilyCount );

	VkQueueFamilyProperties* queueFamilies					= ( VkQueueFamilyProperties* )subAllocator->alloc( sizeof( VkQueueFamilyProperties ) * queueFamilyCount );
	assert( nullptr != queueFamilies );

	vkGetPhysicalDeviceQueueFamilyProperties( chosenPhysicalDevice, &queueFamilyCount, queueFamilies );

	int32_t graphicsQueueFamilyIndex						= -1;
	int32_t presentQueueFamilyIndex							= -1;

	for ( uint32_t ii = 0; ii < queueFamilyCount; ++ii )
	{
		if ( 0 < queueFamilies[ ii ].queueCount )
		{
			if ( queueFamilies[ ii ].queueFlags & VK_QUEUE_GRAPHICS_BIT )
			{
				graphicsQueueFamilyIndex					= ii;
			}

			VkBool32 presentSupport							= false;
			result											= vkGetPhysicalDeviceSurfaceSupportKHR( chosenPhysicalDevice, ii, surface, &presentSupport );
			assert( VK_SUCCESS == result );

			if ( true == presentSupport )
			{
				presentQueueFamilyIndex						= ii;
			}

			if ( -1 != presentQueueFamilyIndex && -1 != graphicsQueueFamilyIndex )
			{
				break;
			}
		}
	}

	HDASSERT( -1 != presentQueueFamilyIndex, "present queue family index값을 찾을 수 없습니다. 비정상 입니다." );
	HDASSERT( -1 != graphicsQueueFamilyIndex, "graphics queue family index값을 찾을 수 없습니다. 비정상 입니다." );

	VkDeviceQueueCreateInfo deviceQueueCreateInfos[ 2 ];
	deviceQueueCreateInfos[ 0 ]								= {};
	deviceQueueCreateInfos[ 0 ].sType						= VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	deviceQueueCreateInfos[ 0 ].queueFamilyIndex			= graphicsQueueFamilyIndex;
	deviceQueueCreateInfos[ 0 ].queueCount					= 1;
	float queuePriority										= 1.0f;
	deviceQueueCreateInfos[ 0 ].pQueuePriorities			= &queuePriority;

	uint32_t queueCount										= 1;
	if ( graphicsQueueFamilyIndex != presentQueueFamilyIndex )
	{
		queueCount											= 2;
		deviceQueueCreateInfos[ 1 ]							= {};
		deviceQueueCreateInfos[ 1 ].sType					= VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		deviceQueueCreateInfos[ 1 ].queueFamilyIndex		= presentQueueFamilyIndex;
		deviceQueueCreateInfos[ 1 ].queueCount				= 1;
		deviceQueueCreateInfos[ 1 ].pQueuePriorities		= &queuePriority;
	}

	VkPhysicalDeviceFeatures deviceFeatures					= {};
	deviceFeatures.depthClamp								= VK_TRUE;

	VkDeviceCreateInfo deviceCreateInfo						= {};
	deviceCreateInfo.sType									= VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount					= queueCount;
	deviceCreateInfo.pQueueCreateInfos						= deviceQueueCreateInfos;
	deviceCreateInfo.pEnabledFeatures						= &deviceFeatures;
	const char* enabledDeviceExtensionNames[]				= { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	deviceCreateInfo.enabledExtensionCount					= sizeof( enabledDeviceExtensionNames ) / sizeof( enabledDeviceExtensionNames[ 0 ] );
	deviceCreateInfo.ppEnabledExtensionNames				= enabledDeviceExtensionNames;

	result													= vkCreateDevice( chosenPhysicalDevice, &deviceCreateInfo, nullptr, &_device );
	if ( VK_SUCCESS != result )
	{
		return false;
	}

	vkGetDeviceQueue( _device, graphicsQueueFamilyIndex, 0, &_graphicsQueue );
	if ( graphicsQueueFamilyIndex != presentQueueFamilyIndex )
	{
		vkGetDeviceQueue( _device, presentQueueFamilyIndex, 0, &_presentQueue );
	}
	else
	{
		_presentQueue										= _graphicsQueue;
	}

	VkSwapchainCreateInfoKHR swapchainCreateInfo			= {};
	swapchainCreateInfo.sType								= VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.surface								= surface;
	swapchainCreateInfo.minImageCount						= swapChainImageCount;
	swapchainCreateInfo.imageFormat							= swapChainSurfaceFormat.format;
	swapchainCreateInfo.imageColorSpace						= swapChainSurfaceFormat.colorSpace;
	swapchainCreateInfo.imageExtent							= _swapchainExtent;
	swapchainCreateInfo.imageArrayLayers					= 1;
	swapchainCreateInfo.imageUsage							= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32_t queueFamilyIndices[]							= { graphicsQueueFamilyIndex, presentQueueFamilyIndex };
	swapchainCreateInfo.queueFamilyIndexCount				= queueCount;
	swapchainCreateInfo.pQueueFamilyIndices					= queueFamilyIndices;

	if ( 1 < queueCount )
	{
		swapchainCreateInfo.imageSharingMode				= VK_SHARING_MODE_CONCURRENT;
	}
	else
	{
		swapchainCreateInfo.imageSharingMode				= VK_SHARING_MODE_EXCLUSIVE;
	}

	swapchainCreateInfo.preTransform						= VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	swapchainCreateInfo.compositeAlpha						= VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.presentMode							= swapChainPresentMode;
	swapchainCreateInfo.clipped								= VK_TRUE;

	result													= vkCreateSwapchainKHR( _device, &swapchainCreateInfo, nullptr, &_swapChain );
	if ( VK_SUCCESS != result )
	{
		return false;
	}

	result													= vkGetSwapchainImagesKHR( _device, _swapChain, &swapChainImageCount, nullptr );
	if ( VK_SUCCESS != result )
	{
		return false;
	}

	assert( 0 < swapChainImageCount );
	VkImage* swapchainImages								= ( VkImage* )subAllocator->alloc( sizeof( VkImage ) * swapChainImageCount );
	assert( nullptr != swapchainImages );
	result													= vkGetSwapchainImagesKHR( _device, _swapChain, &swapChainImageCount, swapchainImages );
	if ( VK_SUCCESS != result )
	{
		return false;
	}

	VkImageView* swapchainImageViews						= ( VkImageView* )subAllocator->alloc( sizeof( VkImageView ) * swapChainImageCount );
	assert( nullptr != swapchainImageViews );
	
	VkImageViewCreateInfo imageViewCreateInfo				= {};
	imageViewCreateInfo.sType								= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.viewType							= VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.format								= swapChainSurfaceFormat.format;
	imageViewCreateInfo.components.r						= VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.g						= VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.b						= VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.a						= VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.subresourceRange.aspectMask			= VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewCreateInfo.subresourceRange.baseMipLevel		= 0;
	imageViewCreateInfo.subresourceRange.levelCount			= 1;
	imageViewCreateInfo.subresourceRange.baseArrayLayer		= 0;
	imageViewCreateInfo.subresourceRange.layerCount			= 1;

	for ( uint32_t ii = 0; ii < swapChainImageCount; ++ii )
	{
		imageViewCreateInfo.image							= swapchainImages[ ii ];

		result												= vkCreateImageView( _device, &imageViewCreateInfo, nullptr, &swapchainImageViews[ ii ] );
		if ( VK_SUCCESS != result )
		{
			return false;
		}
	}

	_imagesInFlight = (VkFence*)allocator->alloc(sizeof(VkFence) * swapChainImageCount);
	memset(_imagesInFlight, 0, sizeof(VkFence) * swapChainImageCount);

	const VkFormat depthBufferFormats[ 6 ]					=
	{
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_X8_D24_UNORM_PACK32,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D16_UNORM,
		VK_FORMAT_D16_UNORM_S8_UINT
	};

	VkFormat chosenDepthBufferFormat						= VK_FORMAT_UNDEFINED;

	for ( int32_t ii = 0; ii < 6; ++ii )
	{
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties( chosenPhysicalDevice, depthBufferFormats[ ii ], &formatProperties );
		if ( formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT )
		{
			chosenDepthBufferFormat							= depthBufferFormats[ ii ];
			break;
		}
	}

	assert( VK_FORMAT_UNDEFINED != chosenDepthBufferFormat );

	VkImageCreateInfo depthBufferImageCreateInfo			= {};
	depthBufferImageCreateInfo.sType						= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	depthBufferImageCreateInfo.imageType					= VK_IMAGE_TYPE_2D;
	depthBufferImageCreateInfo.format						= chosenDepthBufferFormat;
	depthBufferImageCreateInfo.extent.width					= _swapchainExtent.width;
	depthBufferImageCreateInfo.extent.height				= _swapchainExtent.height;
	depthBufferImageCreateInfo.extent.depth					= 1;
	depthBufferImageCreateInfo.mipLevels					= 1;
	depthBufferImageCreateInfo.arrayLayers					= 1;
	depthBufferImageCreateInfo.samples						= VK_SAMPLE_COUNT_1_BIT;
	depthBufferImageCreateInfo.initialLayout				= VK_IMAGE_LAYOUT_UNDEFINED;
	depthBufferImageCreateInfo.usage						= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	depthBufferImageCreateInfo.sharingMode					= VK_SHARING_MODE_EXCLUSIVE;

	VkImage depthBufferImage;
	result													= vkCreateImage( _device, &depthBufferImageCreateInfo, nullptr, &depthBufferImage );
	if ( VK_SUCCESS != result )
	{
		return false;
	}

	VkMemoryRequirements depthBufferImageMemoryRequirements;
	vkGetImageMemoryRequirements( _device, depthBufferImage, &depthBufferImageMemoryRequirements );

	VkPhysicalDeviceMemoryProperties physicalDeviceMemoryRequirements;
	vkGetPhysicalDeviceMemoryProperties( chosenPhysicalDevice, &physicalDeviceMemoryRequirements );

	int32_t chosenMemoryTypeIndex							= -1;
	for ( uint32_t ii = 0; ii <physicalDeviceMemoryRequirements.memoryTypeCount; ++ii )
	{
		if ( ( depthBufferImageMemoryRequirements.memoryTypeBits & ( 1 << ii ) ) &&
			 ( physicalDeviceMemoryRequirements.memoryTypes[ ii ].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ) )
		{
			chosenMemoryTypeIndex							= ii;
			break;
		}
	}

	assert( -1 != chosenMemoryTypeIndex );

	VkMemoryAllocateInfo depthBufferMemoryAllocation		= {};
	depthBufferMemoryAllocation.sType						= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	depthBufferMemoryAllocation.allocationSize				= depthBufferImageMemoryRequirements.size;
	depthBufferMemoryAllocation.memoryTypeIndex				= chosenMemoryTypeIndex;

	VkDeviceMemory depthBufferMemory;
	result													= vkAllocateMemory( _device, &depthBufferMemoryAllocation, nullptr, &depthBufferMemory );
	if ( VK_SUCCESS != result )
	{
		return false;
	}

	vkBindImageMemory( _device, depthBufferImage, depthBufferMemory, 0 );

	VkImageViewCreateInfo depthBufferImageViewInfo			= {};
	depthBufferImageViewInfo.sType							= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	depthBufferImageViewInfo.image							= depthBufferImage;
	depthBufferImageViewInfo.format							= chosenDepthBufferFormat;
	depthBufferImageViewInfo.components.r					= VK_COMPONENT_SWIZZLE_R;
	depthBufferImageViewInfo.components.g					= VK_COMPONENT_SWIZZLE_G;
	depthBufferImageViewInfo.components.b					= VK_COMPONENT_SWIZZLE_B;
	depthBufferImageViewInfo.components.a					= VK_COMPONENT_SWIZZLE_A;
	depthBufferImageViewInfo.subresourceRange.aspectMask	= VK_IMAGE_ASPECT_DEPTH_BIT;
	depthBufferImageViewInfo.subresourceRange.baseMipLevel	= 0;
	depthBufferImageViewInfo.subresourceRange.levelCount	= 1;
	depthBufferImageViewInfo.subresourceRange.baseArrayLayer= 0;
	depthBufferImageViewInfo.subresourceRange.layerCount	= 1;
	depthBufferImageViewInfo.viewType						= VK_IMAGE_VIEW_TYPE_2D;

	VkImageView depthBufferImageView;
	result													= vkCreateImageView( _device, &depthBufferImageViewInfo, nullptr, &depthBufferImageView );
	if ( VK_SUCCESS != result )
	{
		return false;
	}

	// shader
	HANDLE file												= ::CreateFileA( "./shader.vert.spv", GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0 );
	assert( INVALID_HANDLE_VALUE != file );

	DWORD vertShaderSize									= ::GetFileSize( file, 0 );
	assert( INVALID_FILE_SIZE != vertShaderSize );

	int8_t* vertShaderBytes									= subAllocator->alloc( vertShaderSize );
	bool readSuccess										= ::ReadFile( file, vertShaderBytes, vertShaderSize, nullptr, nullptr );
	assert( true == readSuccess );

	file													= ::CreateFileA( "./shader.frag.spv", GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0 );
	assert( INVALID_HANDLE_VALUE != file );

	DWORD fragShaderSize									= ::GetFileSize( file, 0 );
	assert( INVALID_FILE_SIZE != fragShaderSize );

	int8_t* fragShaderBytes									= subAllocator->alloc( fragShaderSize );
	readSuccess												= ::ReadFile( file, fragShaderBytes, fragShaderSize, nullptr, nullptr );
	assert( true == readSuccess );

	VkShaderModuleCreateInfo shaderModuleCreateInfo			= {};
	shaderModuleCreateInfo.sType							= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.codeSize							= vertShaderSize;
	shaderModuleCreateInfo.pCode							= ( uint32_t* )vertShaderBytes;
	VkShaderModule vertShaderModule;

	result													= vkCreateShaderModule( _device, &shaderModuleCreateInfo, nullptr, &vertShaderModule );
	if ( VK_SUCCESS != result )
	{
		return false;
	}

	shaderModuleCreateInfo									= {};
	shaderModuleCreateInfo.sType							= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.codeSize							= fragShaderSize;
	shaderModuleCreateInfo.pCode							= ( uint32_t* )fragShaderBytes;
	VkShaderModule fragShaderModule;

	result													= vkCreateShaderModule( _device, &shaderModuleCreateInfo, nullptr, &fragShaderModule );
	if ( VK_SUCCESS != result )
	{
		return false;
	}

	VkPipelineShaderStageCreateInfo shaderStageCreateInfo[ 2 ];
	shaderStageCreateInfo[ 0 ]								= {};
	shaderStageCreateInfo[ 0 ].sType						= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageCreateInfo[ 0 ].stage						= VK_SHADER_STAGE_VERTEX_BIT;
	shaderStageCreateInfo[ 0 ].module						= vertShaderModule;
	shaderStageCreateInfo[ 0 ].pName						= "main";

	shaderStageCreateInfo[ 1 ]								= {};
	shaderStageCreateInfo[ 1 ].sType						= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageCreateInfo[ 1 ].stage						= VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStageCreateInfo[ 1 ].module						= fragShaderModule;
	shaderStageCreateInfo[ 1 ].pName						= "main";

	VkVertexInputBindingDescription vertexInputBindingDesc	= {};
	vertexInputBindingDesc.binding							= 0;
	vertexInputBindingDesc.stride							= sizeof( Vertex );
	vertexInputBindingDesc.inputRate						= VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription vertexInputAttributeDesc[ 2 ];
	vertexInputAttributeDesc[ 0 ]							= {};
	vertexInputAttributeDesc[ 0 ].binding					= 0;
	vertexInputAttributeDesc[ 0 ].location					= 0;
	vertexInputAttributeDesc[ 0 ].format					= VK_FORMAT_R32G32B32_SFLOAT;
	vertexInputAttributeDesc[ 0 ].offset					= 0;

	vertexInputAttributeDesc[ 1 ]							= {};
	vertexInputAttributeDesc[ 1 ].binding					= 0;
	vertexInputAttributeDesc[ 1 ].location					= 1;
	vertexInputAttributeDesc[ 1 ].format					= VK_FORMAT_R32G32B32_SFLOAT;
	vertexInputAttributeDesc[ 1 ].offset					= 12;

	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo	= {};
	vertexInputCreateInfo.sType								= VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputCreateInfo.vertexBindingDescriptionCount		= 1;
	vertexInputCreateInfo.pVertexBindingDescriptions		= &vertexInputBindingDesc;
	vertexInputCreateInfo.vertexAttributeDescriptionCount	= 2;
	vertexInputCreateInfo.pVertexAttributeDescriptions		= vertexInputAttributeDesc;

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo	= {};
	inputAssemblyCreateInfo.sType							= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyCreateInfo.topology						= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	
	VkViewport viewport										= {};
	viewport.x												= 0.0f;
	viewport.y												= 0.0f;
	viewport.width											= ( float )_swapchainExtent.width;
	viewport.height											= ( float )_swapchainExtent.height;
	viewport.minDepth										= 0.0f;
	viewport.maxDepth										= 1.0f;

	VkRect2D scissor										= {};
	scissor.offset											= { 0, 0 };
	scissor.extent											= _swapchainExtent;

	VkPipelineViewportStateCreateInfo viewportCreateInfo	= {};
	viewportCreateInfo.sType								= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportCreateInfo.viewportCount						= 1;
	viewportCreateInfo.pViewports							= &viewport;
	viewportCreateInfo.scissorCount							= 1;
	viewportCreateInfo.pScissors							= &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizeCreateInfo	= {};
	rasterizeCreateInfo.sType								= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizeCreateInfo.polygonMode							= VK_POLYGON_MODE_FILL;
	rasterizeCreateInfo.lineWidth							= 1.0f;
	rasterizeCreateInfo.cullMode							= VK_CULL_MODE_BACK_BIT;
	rasterizeCreateInfo.frontFace							= VK_FRONT_FACE_CLOCKWISE;
	rasterizeCreateInfo.depthClampEnable					= VK_TRUE;

	VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo	= {};
	multisamplingCreateInfo.sType							= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplingCreateInfo.rasterizationSamples			= VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState colourBlendAttachment	= {};
	colourBlendAttachment.colorWriteMask					= VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colourBlendAttachment.blendEnable						= VK_FALSE;
	colourBlendAttachment.alphaBlendOp						= VK_BLEND_OP_ADD;
	colourBlendAttachment.colorBlendOp						= VK_BLEND_OP_ADD;
	colourBlendAttachment.srcColorBlendFactor				= VK_BLEND_FACTOR_ZERO;
	colourBlendAttachment.dstColorBlendFactor				= VK_BLEND_FACTOR_ZERO;
	colourBlendAttachment.srcAlphaBlendFactor				= VK_BLEND_FACTOR_ZERO;
	colourBlendAttachment.dstAlphaBlendFactor				= VK_BLEND_FACTOR_ZERO;

	VkPipelineColorBlendStateCreateInfo colourBlendStateCreateInfo	= {};
	colourBlendStateCreateInfo.sType						= VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colourBlendStateCreateInfo.attachmentCount				= 1;
	colourBlendStateCreateInfo.pAttachments					= &colourBlendAttachment;
	colourBlendStateCreateInfo.logicOpEnable				= VK_FALSE;
	colourBlendStateCreateInfo.logicOp						= VK_LOGIC_OP_NO_OP;
	colourBlendStateCreateInfo.blendConstants[ 0 ]			= 1.0f;
	colourBlendStateCreateInfo.blendConstants[ 1 ]			= 1.0f;
	colourBlendStateCreateInfo.blendConstants[ 2 ]			= 1.0f;
	colourBlendStateCreateInfo.blendConstants[ 3 ]			= 1.0f;

	VkPipelineDepthStencilStateCreateInfo depthStateCreateInfo	= {};
	depthStateCreateInfo.sType								= VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStateCreateInfo.pNext								= nullptr;
	depthStateCreateInfo.flags								= 0;
	depthStateCreateInfo.depthTestEnable					= VK_TRUE;
	depthStateCreateInfo.depthWriteEnable					= VK_TRUE;
	depthStateCreateInfo.depthCompareOp						= VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStateCreateInfo.depthBoundsTestEnable				= VK_FALSE;
	depthStateCreateInfo.minDepthBounds						= 0;
	depthStateCreateInfo.maxDepthBounds						= 0;
	depthStateCreateInfo.stencilTestEnable					= VK_FALSE;
	depthStateCreateInfo.back.failOp						= VK_STENCIL_OP_KEEP;
	depthStateCreateInfo.back.passOp						= VK_STENCIL_OP_KEEP;
	depthStateCreateInfo.back.compareOp						= VK_COMPARE_OP_ALWAYS;
	depthStateCreateInfo.back.compareMask					= 0;
	depthStateCreateInfo.back.reference						= 0;
	depthStateCreateInfo.back.depthFailOp					= VK_STENCIL_OP_KEEP;
	depthStateCreateInfo.back.writeMask						= 0;
	depthStateCreateInfo.front								= depthStateCreateInfo.back;

	VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {};
	descriptorSetLayoutBinding.binding						= 0;
	descriptorSetLayoutBinding.descriptorType				= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	descriptorSetLayoutBinding.descriptorCount				= 1;
	descriptorSetLayoutBinding.stageFlags					= VK_SHADER_STAGE_VERTEX_BIT;
	descriptorSetLayoutBinding.pImmutableSamplers			= 0;

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo	= {};
	descriptorSetLayoutCreateInfo.sType						= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCreateInfo.bindingCount				= 1;
	descriptorSetLayoutCreateInfo.pBindings					= &descriptorSetLayoutBinding;

	VkDescriptorSetLayout descriptorSetLayout;
	result													= vkCreateDescriptorSetLayout( _device, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout );
	if ( VK_SUCCESS != result )
	{
		return false;
	}

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo		= {};
	pipelineLayoutCreateInfo.sType							= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount					= 1;
	pipelineLayoutCreateInfo.pSetLayouts					= &descriptorSetLayout;

	result													= vkCreatePipelineLayout( _device, &pipelineLayoutCreateInfo, nullptr, &_pipelineLayout );
	if ( VK_SUCCESS != result )
	{
		return false;
	}

	VkAttachmentDescription attachmentDescriptions[ 2 ];
	attachmentDescriptions[ 0 ]								= {};
	attachmentDescriptions[ 0 ].format						= swapChainSurfaceFormat.format;
	attachmentDescriptions[ 0 ].samples						= VK_SAMPLE_COUNT_1_BIT;
	attachmentDescriptions[ 0 ].loadOp						= VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescriptions[ 0 ].storeOp						= VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescriptions[ 0 ].stencilLoadOp				= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescriptions[ 0 ].stencilStoreOp				= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescriptions[ 0 ].initialLayout				= VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescriptions[ 0 ].finalLayout					= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	attachmentDescriptions[ 1 ]								= {};
	attachmentDescriptions[ 1 ].format						= chosenDepthBufferFormat;
	attachmentDescriptions[ 1 ].samples						= VK_SAMPLE_COUNT_1_BIT;
	attachmentDescriptions[ 1 ].loadOp						= VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescriptions[ 1 ].storeOp						= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescriptions[ 1 ].stencilLoadOp				= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescriptions[ 1 ].stencilStoreOp				= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescriptions[ 1 ].initialLayout				= VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescriptions[ 1 ].finalLayout					= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colourAttachmentReference;
	colourAttachmentReference.attachment					= 0;
	colourAttachmentReference.layout						= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentReference;
	depthAttachmentReference.attachment						= 1;
	depthAttachmentReference.layout							= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassDescription					= {};
	subpassDescription.pipelineBindPoint					= VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount					= 1;
	subpassDescription.pColorAttachments					= &colourAttachmentReference;
	subpassDescription.pDepthStencilAttachment				= &depthAttachmentReference;

	VkRenderPassCreateInfo renderPassCreateInfo				= {};
	renderPassCreateInfo.sType								= VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount					= 2;
	renderPassCreateInfo.pAttachments						= attachmentDescriptions;
	renderPassCreateInfo.subpassCount						= 1;
	renderPassCreateInfo.pSubpasses							= &subpassDescription;

	result													= vkCreateRenderPass( _device, &renderPassCreateInfo, nullptr, &_renderPass );
	if ( VK_SUCCESS != result )
	{
		return false;
	}

	VkGraphicsPipelineCreateInfo pipelineCreateInfo			= {};
	pipelineCreateInfo.sType								= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.stageCount							= 2;
	pipelineCreateInfo.pStages								= shaderStageCreateInfo;
	pipelineCreateInfo.pVertexInputState					= &vertexInputCreateInfo;
	pipelineCreateInfo.pInputAssemblyState					= &inputAssemblyCreateInfo;
	pipelineCreateInfo.pViewportState						= &viewportCreateInfo;
	pipelineCreateInfo.pRasterizationState					= &rasterizeCreateInfo;
	pipelineCreateInfo.pMultisampleState					= &multisamplingCreateInfo;
	pipelineCreateInfo.pColorBlendState						= &colourBlendStateCreateInfo;
	pipelineCreateInfo.pDepthStencilState					= &depthStateCreateInfo;
	pipelineCreateInfo.layout								= _pipelineLayout;
	pipelineCreateInfo.renderPass							= _renderPass;
	pipelineCreateInfo.subpass								= 0;

	result													= vkCreateGraphicsPipelines( _device, 0, 1, &pipelineCreateInfo, nullptr, &_graphicsPipeline );
	if ( VK_SUCCESS != result )
	{
		return false;
	}

	_swapchainFramebuffer									= ( VkFramebuffer* )allocator->alloc( sizeof( VkFramebuffer ) * swapChainImageCount );
	assert( nullptr != _swapchainFramebuffer );

	VkImageView framebufferAttachments[ 2 ];
	framebufferAttachments[ 1 ]								= depthBufferImageView;

	VkFramebufferCreateInfo framebufferCreateInfo			= {};
	framebufferCreateInfo.sType								= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferCreateInfo.renderPass						= _renderPass;
	framebufferCreateInfo.attachmentCount					= 2;
	framebufferCreateInfo.pAttachments						= framebufferAttachments;
	framebufferCreateInfo.width								= _swapchainExtent.width;
	framebufferCreateInfo.height							= _swapchainExtent.height;
	framebufferCreateInfo.layers							= 1;

	for ( uint32_t ii = 0; ii < swapChainImageCount; ++ii )
	{
		framebufferAttachments[ 0 ]							= swapchainImageViews[ ii ];
		result												= vkCreateFramebuffer( _device, &framebufferCreateInfo, nullptr, &_swapchainFramebuffer[ ii ] );
		if ( VK_SUCCESS != result )
		{
			return false;
		}
	}

	_matrixBufferPaddingBytes								= ( ( ( ( sizeof( Matrix4x4 ) / chosenPhysicalDeviceProperties.limits.minUniformBufferOffsetAlignment ) + 1 ) * chosenPhysicalDeviceProperties.limits.minUniformBufferOffsetAlignment ) - sizeof( Matrix4x4 ) );
	const int32_t matrixBufferSize							= ( sizeof( Matrix4x4 ) + _matrixBufferPaddingBytes ) * ( maxPlayer + 1 );

	if ( false == createBuffer( chosenPhysicalDevice, _device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, matrixBufferSize, &_matrixBuffer, &_matrixBufferMemory ) )
	{
		return false;
	}

	VkDescriptorPoolSize descriptorPoolSize					= {};
	descriptorPoolSize.type									= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	descriptorPoolSize.descriptorCount						= 1;

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo		= {};
	descriptorPoolCreateInfo.sType							= VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.flags							= 0;
	descriptorPoolCreateInfo.maxSets						= 1;
	descriptorPoolCreateInfo.pPoolSizes						= &descriptorPoolSize;
	descriptorPoolCreateInfo.poolSizeCount					= 1;

	VkDescriptorPool descriptorPool							= VK_NULL_HANDLE;
	result													= vkCreateDescriptorPool( _device, &descriptorPoolCreateInfo, nullptr, &descriptorPool );
	if ( VK_SUCCESS != result )
	{
		return false;
	}

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo	= {};
	descriptorSetAllocateInfo.sType							= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.descriptorPool				= descriptorPool;
	descriptorSetAllocateInfo.descriptorSetCount			= 1;
	descriptorSetAllocateInfo.pSetLayouts					= &descriptorSetLayout;

	result													= vkAllocateDescriptorSets( _device, &descriptorSetAllocateInfo, &_descriptorSet );
	if ( VK_SUCCESS != result )
	{
		return false;
	}

	VkDescriptorBufferInfo bufferInfo						= {};
	bufferInfo.buffer										= _matrixBuffer;
	bufferInfo.offset										= 0;
	bufferInfo.range										= sizeof( Matrix4x4 );

	VkWriteDescriptorSet writeDescriptorSet					= {};
	writeDescriptorSet.sType								= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet.dstSet								= _descriptorSet;
	writeDescriptorSet.dstBinding							= 0;
	writeDescriptorSet.dstArrayElement						= 0;
	writeDescriptorSet.descriptorCount						= 1;
	writeDescriptorSet.descriptorType						= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	writeDescriptorSet.pImageInfo							= nullptr;
	writeDescriptorSet.pBufferInfo							= &bufferInfo;
	writeDescriptorSet.pTexelBufferView						= nullptr;

	uint32_t descriptorWriteCount							= 1;
	const VkWriteDescriptorSet* descriptorWrites			= &writeDescriptorSet;

	uint32_t descriptorCopyCount							= 0;
	const VkCopyDescriptorSet* descriptorCopies				= nullptr;

	vkUpdateDescriptorSets( _device, descriptorWriteCount, descriptorWrites, descriptorCopyCount, descriptorCopies );

	_commandPools											= ( VkCommandPool* )allocator->alloc( sizeof( VkCommandPool ) * swapChainImageCount );
	assert( nullptr != _commandPools );

	_commandBuffers											= ( VkCommandBuffer* )allocator->alloc( sizeof( VkCommandBuffer ) * swapChainImageCount );
	assert( nullptr != _commandBuffers );
	
	_comaandBufferInUse										= ( bool* )allocator->alloc( sizeof( bool ) * swapChainImageCount );
	assert( nullptr != _comaandBufferInUse );
	memset( _comaandBufferInUse, 0, sizeof( bool ) * swapChainImageCount );
	
	_fences													= ( VkFence* )allocator->alloc( sizeof( VkFence ) * swapChainImageCount );
	assert( nullptr != _fences );

	VkCommandPoolCreateInfo commandPoolCreateInfo			= {};
	commandPoolCreateInfo.sType								= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.queueFamilyIndex					= graphicsQueueFamilyIndex;

	VkCommandBufferAllocateInfo commandBufferAllocateInfo	= {};
	commandBufferAllocateInfo.sType							= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.level							= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount			= 1;

	VkFenceCreateInfo fenceCreateInfo						= {};
	fenceCreateInfo.sType									= VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags									= VK_FENCE_CREATE_SIGNALED_BIT;

	for ( uint32_t ii = 0; ii < swapChainImageCount; ++ii )
	{
		result												= vkCreateCommandPool( _device, &commandPoolCreateInfo, nullptr, &_commandPools[ ii ] );
		if ( VK_SUCCESS != result )
		{
			return false;
		}

		commandBufferAllocateInfo.commandPool				= _commandPools[ ii ];
		result												= vkAllocateCommandBuffers( _device, &commandBufferAllocateInfo, &_commandBuffers[ ii ] );
		if ( VK_SUCCESS != result )
		{
			return false;
		}

		_comaandBufferInUse[ ii ]							= false;

		result												= vkCreateFence( _device, &fenceCreateInfo, nullptr, &_fences[ ii ] );
		if ( VK_SUCCESS != result )
		{
			return false;
		}
	}

	VkSemaphoreCreateInfo semaphoreCreateInfo				= {};
	semaphoreCreateInfo.sType								= VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	result													= vkCreateSemaphore( _device, &semaphoreCreateInfo, nullptr, &_imageAvailableSemaphore );
	if ( VK_SUCCESS != result )
	{
		return false;
	}

	result													= vkCreateSemaphore( _device, &semaphoreCreateInfo, nullptr, &_renderFinishedSemaphore );
	if ( VK_SUCCESS != result )
	{
		return false;
	}

	for ( int32_t ii = 0; ii < MAX_FRAMES_IN_FLIGHT; ++ii )
	{
		vkCreateSemaphore( _device, &semaphoreCreateInfo, nullptr, &_imageAvailableSemaphores[ ii ] );
		vkCreateSemaphore( _device, &semaphoreCreateInfo, nullptr, &_renderFinishedSemaphores[ ii ] );
		vkCreateFence( _device, &fenceCreateInfo, nullptr, &_inFlightFences[ ii ] );
	}

	// player mesh
	constexpr int32_t numVertices							= 24;
	constexpr int32_t numIndices							= 36;
	constexpr int32_t cubeVertexBufferSize					= numVertices * sizeof( Vertex );
	constexpr int32_t cubeIndexBufferSize					= numIndices * sizeof( uint16_t );

	Vertex* vertices										= ( Vertex* )subAllocator->alloc( cubeVertexBufferSize );
	assert( nullptr != vertices );

	uint16_t* indices										= ( uint16_t* )subAllocator->alloc( cubeIndexBufferSize );
	assert( nullptr != indices );

	// front face

	constexpr float size									= 1.0f;

	Vector3 center											= Vector3( 0.0f, -0.5f, 0.0f );
	Vector3 colour											= Vector3( 1.0f, 0.0f, 0.0f );
	Vector3 right											= Vector3( size, 0.0f, 0.0f );
	Vector3 up												= Vector3( 0.0f, 0.0f, size );

	createCubeFace( vertices, 0, indices, 0, center, right, up, colour );

	// back face

	center													= Vector3( 0.0f, 0.5f, 0.0f );
	colour													= Vector3( 0.0f, 1.0f, 0.0f );
	right													= Vector3( -size, 0.0f, 0.0f );

	createCubeFace( vertices, 4, indices, 6, center, right, up, colour );

	// left face
	center													= Vector3( -0.5f, 0.0f, 0.0f );
	colour													= Vector3( 0.0f, 0.0f, 1.0f );
	right													= Vector3( 0.0f, -size, 0.0f );

	createCubeFace( vertices, 8, indices, 12, center, right, up, colour );

	// right face

	center													= Vector3( 0.5f, 0.0f, 0.0f );
	colour													= Vector3( 1.0f, 1.0f, 0.0f );
	right													= Vector3( 0.0f, size, 0.0f );

	createCubeFace( vertices, 12, indices, 18, center, right, up, colour );

	// bottom face

	center													= Vector3( 0.0f, 0.0f, -0.5f );
	colour													= Vector3( 0.0f, 1.0f, 1.0f );
	right													= Vector3( size, 0.0f, 0.0f );
	up														= Vector3( 0.0f, -size, 0.0f );

	createCubeFace( vertices, 16, indices, 24, center, right, up, colour );

	// top face

	center													= Vector3( 0.0f, 0.0f, 0.5f );
	colour													= Vector3( 1.0f, 0.0f, 1.0f );
	right													= Vector3( -size, 0.0f, 0.0f );

	createCubeFace( vertices, 20, indices, 30, center, right, up, colour );

	VkDeviceMemory cubeVertexBufferMemory					= VK_NULL_HANDLE;
	if ( false == createBuffer( chosenPhysicalDevice, _device, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, cubeVertexBufferSize, &_cubeVertexBuffer, &cubeVertexBufferMemory ) )
	{
		return false;
	}

	copyToBuffer( _device, cubeVertexBufferMemory, ( void* )vertices, cubeVertexBufferSize );

	VkDeviceMemory cubeIndexBufferMemory					= VK_NULL_HANDLE;
	if ( false == createBuffer( chosenPhysicalDevice, _device, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, cubeIndexBufferSize, &_cubeIndexBuffer, &cubeIndexBufferMemory ) )
	{
		return false;
	}

	copyToBuffer( _device, cubeIndexBufferMemory, ( void* )indices, cubeIndexBufferSize );

	_cubeNumIndices											= numIndices;
	
	constexpr int32_t floorTilesCount						= 50;
	constexpr int32_t floorTilesTotal						= floorTilesCount * floorTilesCount;
	constexpr float floorTileSize							= 1.0f;
	constexpr float floorTileSpacing						= 0.5f;

	constexpr int32_t numSceneryVertices					= floorTilesTotal * 4;
	constexpr int32_t numSceneryIndices						= floorTilesTotal * 6;

	constexpr int32_t sceneryVertexBufferSize				= numSceneryVertices * sizeof( Vertex );
	constexpr int32_t sceneryIndexBufferSize				= numSceneryIndices * sizeof( uint16_t );

	vertices												= ( Vertex* )subAllocator->alloc( sceneryVertexBufferSize );
	assert( nullptr != vertices );

	indices													= ( uint16_t* )subAllocator->alloc( sceneryIndexBufferSize );
	assert( nullptr != indices );

	up														= Vector3( 0.0f, 1.0f, 0.0f );
	right													= Vector3( 1.0f, 0.0f, 0.0f );
	colour													= Vector3( 1.0f, 1.0f, 1.0f );

	int32_t vertexOffset									= 0;
	int32_t indexOffset										= 0;

	for ( int32_t xx = 0; xx < floorTilesCount; ++xx )
	{
		for ( int32_t yy = 0; yy < floorTilesCount; ++yy )
		{
			center._x										= ( xx - ( ( floorTilesCount - 1 ) / 2.0f ) ) * ( floorTileSize + floorTileSpacing );
			center._y										= ( yy - ( ( floorTilesCount - 1 ) / 2.0f ) ) * ( floorTileSize + floorTileSpacing );
			center._z										= -0.5f;

			createCubeFace( vertices, vertexOffset, indices, indexOffset, center, right, up, colour );

			vertexOffset									+= 4;
			indexOffset										+= 6;
		}
	}

	VkDeviceMemory sceneryVertexBufferMemory				= VK_NULL_HANDLE;
	VkDeviceMemory sceneryIndexBufferMemory					= VK_NULL_HANDLE;

	if ( false == createBuffer( chosenPhysicalDevice, _device, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, sceneryVertexBufferSize, &_sceneryVertexBuffer, &sceneryVertexBufferMemory ) )
	{
		return false;
	}

	copyToBuffer( _device, sceneryVertexBufferMemory, ( void* )vertices, sceneryVertexBufferSize );

	if ( false == createBuffer( chosenPhysicalDevice, _device, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, sceneryIndexBufferSize, &_sceneryIndexBuffer, &sceneryIndexBufferMemory ) )
	{
		return false;
	}

	copyToBuffer( _device, sceneryIndexBufferMemory, ( void* )indices, sceneryIndexBufferSize );

	_sceneryNumIndices										= numSceneryIndices;

	return true;
}

void GrpahicsState::updateAndDraw( Matrix4x4* matrices, int32_t numPlayers ) noexcept
{
	assert( nullptr != matrices );

	if ( numPlayers <= 0 )
	{
		return;
	}

	VkFence fence = _inFlightFences[_currentFrame];
	VkResult result											= vkWaitForFences( _device, 1, &fence, VK_TRUE, UINT64_MAX );
	if ( VK_SUCCESS != result )
	{
		HDASSERT( false, "vkWaitForFences에 실패 했습니다." );
		return;
	}

	uint32_t imageIndex										= 0;
	result													= vkAcquireNextImageKHR( _device, _swapChain, UINT64_MAX, _imageAvailableSemaphores[ _currentFrame ], 0, &imageIndex );
	if ( VK_SUCCESS != result )
	{
		HDASSERT( false, "vkAcquireNextImageKHR에 실패 했습니다." );
		return;
	}

	// 이 swapchain 이미지를 이전에 사용한 프레임이 아직 실행 중이면 대기
	if ( VK_NULL_HANDLE != _imagesInFlight[ imageIndex ] )
	{
		result												= vkWaitForFences( _device, 1, &_imagesInFlight[ imageIndex ], VK_TRUE, UINT64_MAX );
		if ( VK_SUCCESS != result )
		{
			HDASSERT( false, "vkWaitForFences에 실패 했습니다." );
			return;
		}
	}

	_imagesInFlight[ imageIndex ]							= fence;

	result													= vkResetFences( _device, 1, &fence );
	if ( VK_SUCCESS != result )
	{
		HDASSERT( false, "vkResetFences에 실패 했습니다." );
		return;
	}

	const int32_t matrixDataSize							= ( numPlayers + 1 ) * sizeof( matrices[ 0 ] );
	uint8_t* dst											= nullptr;
	result													= vkMapMemory( _device, _matrixBufferMemory, 0, matrixDataSize, 0, ( void ** )&dst );
	if ( VK_SUCCESS != result )
	{
		HDASSERT( false, "vkMapMemory에 실패 했습니다." );
		return;
	}

	for ( int32_t ii = 0; ii < ( numPlayers + 1 ); ++ii )
	{
		int32_t offset										= ii * ( sizeof( matrices[ 0 ] ) + _matrixBufferPaddingBytes );
		memcpy( &dst[ offset ], &matrices[ ii ], sizeof( matrices[ 0 ] ) );
	}

	vkUnmapMemory( _device, _matrixBufferMemory );

	result													= vkResetCommandPool( _device, _commandPools[ imageIndex ], 0 );
	if ( VK_SUCCESS != result )
	{
		HDASSERT( false, "vkResetCommandPool에 실패 했습니다." );
		return;
	}

	VkCommandBufferBeginInfo commandBufferBeginInfo			= {};
	commandBufferBeginInfo.sType							= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.flags							= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	result													= vkBeginCommandBuffer( _commandBuffers[ imageIndex ], &commandBufferBeginInfo );

	VkClearValue clearColour[ 2 ];
	clearColour[ 0 ].color									= { 0.0f, 0.0f, 0.0f, 1.0f };
	clearColour[ 1 ].depthStencil.depth						= 1.0f;
	clearColour[ 1 ].depthStencil.stencil					= 0;

	VkRenderPassBeginInfo renderPassBeginInfo				= {};

	renderPassBeginInfo.sType								= VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass							= _renderPass;
	renderPassBeginInfo.framebuffer							= _swapchainFramebuffer[ imageIndex ];
	renderPassBeginInfo.renderArea.offset					= { 0, 0 };
	renderPassBeginInfo.renderArea.extent					= _swapchainExtent;
	renderPassBeginInfo.clearValueCount						= 2;
	renderPassBeginInfo.pClearValues						= clearColour;

	vkCmdBeginRenderPass( _commandBuffers[ imageIndex ], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );
	vkCmdBindPipeline( _commandBuffers[ imageIndex ], VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline );

	// draw scenery
	VkDeviceSize offset										= 0;
	vkCmdBindVertexBuffers( _commandBuffers[ imageIndex ], 0, 1, &_sceneryVertexBuffer, &offset );
	vkCmdBindIndexBuffer( _commandBuffers[ imageIndex ], _sceneryIndexBuffer, 0, VK_INDEX_TYPE_UINT16 );

	VkPipelineBindPoint pipelineBindPoint					= VK_PIPELINE_BIND_POINT_GRAPHICS;

	uint32_t firstSet										= 0;
	uint32_t descriptorSetCount								= 1;
	uint32_t dynamicOffsetCount								= 1;
	uint32_t dynamicOffset									= 0;

	vkCmdBindDescriptorSets( _commandBuffers[ imageIndex ], pipelineBindPoint, _pipelineLayout, firstSet, descriptorSetCount, &_descriptorSet, dynamicOffsetCount, &dynamicOffset );

	vkCmdDrawIndexed( _commandBuffers[ imageIndex ], _sceneryNumIndices, 1, 0, 0, 0 );

	// draw players

	vkCmdBindVertexBuffers( _commandBuffers[ imageIndex ], 0, 1, &_cubeVertexBuffer, &offset );
	vkCmdBindIndexBuffer( _commandBuffers[ imageIndex ], _cubeIndexBuffer, 0, VK_INDEX_TYPE_UINT16 );

	for ( uint32_t ii = 0; ii < numPlayers; ++ii )
	{
		firstSet											= 0;
		descriptorSetCount									= 1;
		dynamicOffsetCount									= 1;
		dynamicOffset										= ( ii + 1 ) * ( sizeof( matrices[ 0 ] ) + _matrixBufferPaddingBytes );

		vkCmdBindDescriptorSets( _commandBuffers[ imageIndex ], pipelineBindPoint, _pipelineLayout, firstSet, descriptorSetCount, &_descriptorSet, dynamicOffsetCount, &dynamicOffset );
		vkCmdDrawIndexed( _commandBuffers[ imageIndex ], _cubeNumIndices, 1, 0, 0, 0 );
	}

	vkCmdEndRenderPass( _commandBuffers[ imageIndex ] );

	result													= vkEndCommandBuffer( _commandBuffers[ imageIndex ] );
	if ( VK_SUCCESS != result )
	{
		return;
	}

	VkSubmitInfo submitInfo									= {};
	submitInfo.sType										= VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount							= 1;
	submitInfo.pWaitSemaphores								= &_imageAvailableSemaphores[ _currentFrame ];

	VkPipelineStageFlags waitStage							= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submitInfo.pWaitDstStageMask							= &waitStage;
	submitInfo.commandBufferCount							= 1;
	submitInfo.pCommandBuffers								= &_commandBuffers[ imageIndex ];
	submitInfo.signalSemaphoreCount							= 1;
	submitInfo.pSignalSemaphores							= &_renderFinishedSemaphores[_currentFrame];

	result													= vkQueueSubmit( _graphicsQueue, 1, &submitInfo, fence );
	if ( VK_SUCCESS != result )
	{
		HDASSERT( false, "vkQueueSubmit에 실패 했습니다." );
		return;
	}

	VkPresentInfoKHR presentInfo							= {};
	presentInfo.sType										= VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount							= 1;
	presentInfo.pWaitSemaphores								= &_renderFinishedSemaphores[_currentFrame];
	presentInfo.swapchainCount								= 1;
	presentInfo.pSwapchains									= &_swapChain;
	presentInfo.pImageIndices								= &imageIndex;

	result													= vkQueuePresentKHR( _presentQueue, &presentInfo );
	if ( VK_SUCCESS != result )
	{
		HDASSERT( false, "vkQueuePresentKHR에 실패 했습니다." );
		return;
	}

	_currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
