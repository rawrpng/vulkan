#pragma once
#include <vulkan/vulkan.h>
#include <vk/VkBootstrap.h>
#include "vkobjs.hpp"
class vksyncobjects {
public:
	static bool init(vkobjs& rdata);
	static void cleanup(vkobjs& rdata);
};
