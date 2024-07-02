#define GLM_ENABLE_EXPERIMENTAL
#define VMA_IMPLEMENTATION


//#define _DEBUG

#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <ctime>
#include <cstdlib>
#include <glm/gtx/spline.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <vk/vk_mem_alloc.h>
#include <iostream>
#include "vkrenderer.hpp"
#ifdef _DEBUG
#include "logger.hpp"
#endif

float map2(glm::vec3 x) {
	return std::max(x.y, 0.0f);
}
vkrenderer::vkrenderer(GLFWwindow* wind,GLFWmonitor* mont,const GLFWvidmode* mode) {
	mvkobjs.rdwind = wind;
	mvkobjs.rdmonitor = mont;
	mvkobjs.rdmode = mode;
	mvkobjs.decaying = &decaying;


	mpersviewmats.emplace_back(glm::mat4{ 1.0f });
	mpersviewmats.emplace_back(glm::mat4{ 1.0f });
}
bool vkrenderer::init() {

	

	std::srand(static_cast<int>(time(NULL)));
	mvkobjs.rdheight = mvkobjs.rdvkbswapchain.extent.height;
	mvkobjs.rdwidth = mvkobjs.rdvkbswapchain.extent.width;



	if (!mvkobjs.rdwind)return false;


	//std::lock_guard<std::shared_mutex> lg{ *mvkobjs.mtx2 };
	mvkobjs.mtx2->lock();
	if (!deviceinit()){
		#ifdef _DEBUG
		logger::log(0, "crashed at ", __FUNCTION__);
		#endif
		return false;
	}
	if (!initvma()){
		#ifdef _DEBUG
		logger::log(0,"crashed at ",__FUNCTION__);
		#endif
		return false;
	}
	mvkobjs.mtx2->unlock();


	if (!getqueue()){
		#ifdef _DEBUG
		logger::log(0,"crashed at ",__FUNCTION__);
		#endif
		return false;
	}
	if (!createswapchain()){
		#ifdef _DEBUG
		logger::log(0,"crashed at ",__FUNCTION__);
		#endif
		return false;
	}
	if (!createdepthbuffer()){
		#ifdef _DEBUG
		logger::log(0,"crashed at ",__FUNCTION__);
		#endif
		return false;
	}
	if (!createcommandpool()){
		#ifdef _DEBUG
		logger::log(0,"crashed at ",__FUNCTION__);
		#endif
		return false;
	}
	if (!createcommandbuffer()){
		#ifdef _DEBUG
		logger::log(0,"crashed at ",__FUNCTION__);
		#endif
		return false;
	}
	if (!createrenderpass()){
		#ifdef _DEBUG
		logger::log(0,"crashed at ",__FUNCTION__);
		#endif
		return false;
	}
	if (!createframebuffer()){
		#ifdef _DEBUG
		logger::log(0,"crashed at ",__FUNCTION__);
		#endif
		return false;
	}
	if (!createsyncobjects()){
		#ifdef _DEBUG
		logger::log(0,"crashed at ",__FUNCTION__);
		#endif
		return false;
	}


	if (!loadbackground()){
		#ifdef _DEBUG
		logger::log(0,"crashed at ",__FUNCTION__);
		#endif
		return false;
	}



	if (!initui()){
		#ifdef _DEBUG
		logger::log(0,"crashed at ",__FUNCTION__);
		#endif
		return false;
	}

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;


	mframetimer.start();


	return true;
}



bool vkrenderer::initscene() {

	mpgltf.reserve(animfname.size());
	mpgltf.resize(animfname.size());
	mstatic0.reserve(staticfname.size());
	mstatic0.resize(staticfname.size());

	if (!setupground())return false;
	mvkobjs.loadingprog += 0.1f;
	if (!setupplayer())return false;
	mvkobjs.loadingprog += 0.1f;
	if (!setupmodels())return false;
	mvkobjs.loadingprog += 0.1f;
	if (!setupstaticmodels())return false;
	mvkobjs.loadingprog += 0.1f;
	if (!setuplifebar())return false;
	mvkobjs.loadingprog += 0.1f;
	if (!setupground2())return false;
	mvkobjs.loadingprog += 0.1f;
	if (!setupplayer2())return false;
	mvkobjs.loadingprog += 0.1f;
	if (!setupmodels2())return false;
	mvkobjs.loadingprog += 0.1f;
	if (!setupstaticmodels2())return false;
	mvkobjs.loadingprog += 0.1f;
	if (!setuplifebar2())return false;
	mvkobjs.loadingprog += 0.1f;

	return true;
}

bool vkrenderer::getserverclientstatus(){
	return mnobjs.rdserverclient;
}
ui* vkrenderer::getuihandle(){
	return &mui;
}
netobjs& vkrenderer::getnetobjs() {
	return mnobjs;
}
vkobjs& vkrenderer::getvkobjs() {
	return mvkobjs;
}


bool vkrenderer::quicksetup(netclient* nclient){
	mnobjs.nclient = nclient;
	ImGui_ImplGlfw_RestoreCallbacks(mvkobjs.rdwind);
	glfwSetKeyCallback(mvkobjs.rdwind, [](GLFWwindow* win, int key, int scancode, int action, int mods) {
		auto r = static_cast<vkrenderer*>(glfwGetWindowUserPointer(win));
		r->handlekey(key, scancode, action, mods);
	});
	ImGui_ImplGlfw_InstallCallbacks(mvkobjs.rdwind);
	//ImGui_ImplGlfw_RestoreCallbacks(mvkobjs.rdwind);
	inmenu = false;
	return true;
}
bool vkrenderer::quicksetup(netserver* nserver) {
	mnobjs.nserver = nserver;
	ImGui_ImplGlfw_RestoreCallbacks(mvkobjs.rdwind);
	glfwSetKeyCallback(mvkobjs.rdwind, [](GLFWwindow* win, int key, int scancode, int action, int mods) {
		auto r = static_cast<vkrenderer*>(glfwGetWindowUserPointer(win));
		r->handlekey(key, scancode, action, mods);
	});
	ImGui_ImplGlfw_InstallCallbacks(mvkobjs.rdwind);
	//ImGui_ImplGlfw_RestoreCallbacks(mvkobjs.rdwind);
	inmenu = false;
	return true;
}

bool vkrenderer::setuplifebar() {
	lifebar = std::make_shared<playoutmenubg>();
	if (!lifebar->setup(mvkobjs,1))return false;
	return true;
}
bool vkrenderer::setuplifebar2() {
	if (!lifebar->setup2(mvkobjs, lifebarshaders[0], lifebarshaders[1]))return false;
	return true;
}

bool vkrenderer::loadbackground(){
	mbackground = std::make_shared<playoutback>();
	mmenubg = std::make_shared<playoutmenubg>();
	if (!mbackground->setup(mvkobjs, backfname, backgobjs)){
		#ifdef _DEBUG
		logger::log(0,"crashed at ",__FUNCTION__);
		#endif
		return false;
	}
	if (!mbackground->setup2(mvkobjs, backshaders[0], backshaders[1])){
		#ifdef _DEBUG
		logger::log(0,"crashed at ",__FUNCTION__);
		#endif
		return false;
	}
	if (!mmenubg->setup(mvkobjs,1)){
		#ifdef _DEBUG
		logger::log(0,"crashed at ",__FUNCTION__);
		#endif
		return false;
	}
	if (!mmenubg->setup2(mvkobjs, menubgshaders[0], menubgshaders[1])){
		#ifdef _DEBUG
		logger::log(0,"crashed at ",__FUNCTION__);
		#endif
		return false;
	}
	return true;
}

bool vkrenderer::setupground() {
	mground = std::make_shared<playoutground>();
	if (!mground->setup(mvkobjs, groundfname, groundobjs));
	return true;
}
bool vkrenderer::setupground2() {
	if (!mground->setup2(mvkobjs, groundshaders[0], groundshaders[1]));
	return true;
}
bool vkrenderer::setupplayer(){
	mplayer = std::make_shared<playoutplayer>();
	if (!mplayer->setup(mvkobjs, playerfname, playercount));
	return true;
}
bool vkrenderer::setupplayer2(){
	if (!mplayer->setup2(mvkobjs, playershaders[0], playershaders[1]));
	return true;
}
bool vkrenderer::setupmodels() {
	for (size_t i{ 0 }; i < mpgltf.size(); i++) {
		mpgltf[i] = std::make_shared<playoutmodel>();
		if (!mpgltf[i]->setup(mvkobjs, animfname[i], animcounts[i]))return false;
	}
	return true;
}
bool vkrenderer::setupmodels2() {
	for (size_t i{ 0 }; i < mpgltf.size(); i++) {
		if (!mpgltf[i]->setup2(mvkobjs, animshaders[0], animshaders[1]))return false;
	}
	return true;
}
bool vkrenderer::setupstaticmodels(){
	for (size_t i{ 0 }; i < mstatic0.size(); i++) {
		mstatic0[i] = std::make_shared<playoutstatic>();
		if (!mstatic0[i]->setup(mvkobjs, staticfname[i], staticcounts[i]))return false;
	}
	return true;
}
bool vkrenderer::setupstaticmodels2(){
	for (size_t i{ 0 }; i < mstatic0.size(); i++) {
		if (!mstatic0[i]->setup2(mvkobjs, staticshaders[0], staticshaders[1]))return false;
	}
	return true;
}

bool vkrenderer::deviceinit() {
	vkb::InstanceBuilder instbuild{};

	//std::lock_guard<std::shared_mutex> lg{ *mvkobjs.mtx2 };
	auto instret = instbuild.use_default_debug_messenger().request_validation_layers().enable_validation_layers().require_api_version(1, 2, 0).build();
	

	//instret.value().

	mvkobjs.rdvkbinstance = instret.value();

	VkResult res = VK_ERROR_UNKNOWN;
	res = glfwCreateWindowSurface(mvkobjs.rdvkbinstance, mvkobjs.rdwind, nullptr, &msurface);

	

	vkb::PhysicalDeviceSelector physicaldevsel{ mvkobjs.rdvkbinstance };
	auto firstphysicaldevselret = physicaldevsel.set_surface(msurface).set_minimum_version(1,2).select();

	//VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT x;
	//x.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT;
	//x.swapchainMaintenance1 = VK_FALSE;
	//x.pNext = VK_NULL_HANDLE;

	//VkPhysicalDeviceMeshShaderFeaturesEXT physmeshfeatures;
	VkPhysicalDeviceVulkan13Features physfeatures13;

	//VkPhysicalDevice8BitStorageFeatures b8storagefeature;
	VkPhysicalDeviceVulkan12Features physfeatures12;
	//VkPhysicalDeviceVulkan11Features physfeatures11;
	VkPhysicalDeviceFeatures2 physfeatures;
	//b8storagefeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES;
	//physmeshfeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
	physfeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	physfeatures12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	physfeatures13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	//physmeshfeatures.pNext = &physfeatures12;
	physfeatures.pNext = &physfeatures12;
	physfeatures13.pNext = VK_NULL_HANDLE;
	//b8storagefeature.pNext = VK_NULL_HANDLE;
	physfeatures12.pNext = &physfeatures13;
	vkGetPhysicalDeviceFeatures2(firstphysicaldevselret.value(), &physfeatures);
	//std::cout << "\n\n\n\n" << physfeatures12.runtimeDescriptorArray << std::endl;
	//if (physmeshfeatures.meshShader == VK_FALSE) {
	//	std::cout << "NO mesh shader support"  << std::endl;
	//}
	//if (physmeshfeatures.taskShader == VK_FALSE) {
	//	std::cout << "NO task shader support" << std::endl;
	//}
	//physmeshfeatures.meshShader = VK_TRUE;
	//physmeshfeatures.taskShader = VK_TRUE;
	//physmeshfeatures.multiviewMeshShader = VK_FALSE;
	//physmeshfeatures.primitiveFragmentShadingRateMeshShader = VK_FALSE;
	{
		

		




	}






	//auto secondphysicaldevselret = physicaldevsel.set_minimum_version(1, 3).set_surface(msurface).set_required_features(physfeatures.features).add_required_extension_features(physmeshfeatures).set_required_features_12(physfeatures12).set_required_features_13(physfeatures13).add_required_extension("VK_EXT_mesh_shader").select();
	auto secondphysicaldevselret = physicaldevsel.set_minimum_version(1, 3).set_surface(msurface).set_required_features(physfeatures.features).set_required_features_12(physfeatures12).set_required_features_13(physfeatures13).select();
	//auto secondphysicaldevselret = physicaldevsel.set_minimum_version(1, 0).set_surface(msurface).select();

	//std::cout << "\n\n\n\n\n\n\n\n\n\n mesh shader value: " << secondphysicaldevselret.value().is_extension_present("VK_EXT_mesh_shader") << "\n\n\n\n\n";
	//std::cout << "\n\n\n\n\n\n\n\n\n\n maintenance: " << secondphysicaldevselret.value().is_extension_present("VK_EXT_swapchain_maintenance1") << "\n\n\n\n\n";
	mvkobjs.rdvkbphysdev = secondphysicaldevselret.value();


	//for (auto& x : secondphysicaldevselret.value().get_available_extensions()) { std::cout << x << std::endl; }

	mminuniformbufferoffsetalignment = mvkobjs.rdvkbphysdev.properties.limits.minUniformBufferOffsetAlignment;

	vkb::DeviceBuilder devbuilder{ mvkobjs.rdvkbphysdev };
	auto devbuilderret = devbuilder.build();
	mvkobjs.rdvkbdevice = devbuilderret.value();

	return true;
}

bool vkrenderer::getqueue()
{
	auto graphqueueret = mvkobjs.rdvkbdevice.get_queue(vkb::QueueType::graphics);
	mvkobjs.rdgraphicsqueue = graphqueueret.value();

	auto presentqueueret = mvkobjs.rdvkbdevice.get_queue(vkb::QueueType::present);
	mvkobjs.rdpresentqueue = presentqueueret.value();

	return true;
}

bool vkrenderer::createdepthbuffer() {
	VkExtent3D depthimageextent = {
		mvkobjs.rdvkbswapchain.extent.width,
		mvkobjs.rdvkbswapchain.extent.height,
		1
	};

	mvkobjs.rddepthformat = VK_FORMAT_D32_SFLOAT;

	VkImageCreateInfo depthimginfo{};
	depthimginfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	depthimginfo.imageType = VK_IMAGE_TYPE_2D;
	depthimginfo.format = mvkobjs.rddepthformat;
	depthimginfo.extent = depthimageextent;
	depthimginfo.mipLevels = 1;
	depthimginfo.arrayLayers = 1;
	depthimginfo.samples = VK_SAMPLE_COUNT_1_BIT;
	depthimginfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	depthimginfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	VmaAllocationCreateInfo depthallocinfo{};
	depthallocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	depthallocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (vmaCreateImage(mvkobjs.rdallocator, &depthimginfo, &depthallocinfo, &mvkobjs.rddepthimage, &mvkobjs.rddepthimagealloc, nullptr) != VK_SUCCESS) {
		return false;
	}

	VkImageViewCreateInfo depthimgviewinfo{};
	depthimgviewinfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	depthimgviewinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	depthimgviewinfo.image = mvkobjs.rddepthimage;
	depthimgviewinfo.format = mvkobjs.rddepthformat;
	depthimgviewinfo.subresourceRange.baseMipLevel = 0;
	depthimgviewinfo.subresourceRange.levelCount = 1;
	depthimgviewinfo.subresourceRange.baseArrayLayer = 0;
	depthimgviewinfo.subresourceRange.layerCount = 1;
	depthimgviewinfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

	if (vkCreateImageView(mvkobjs.rdvkbdevice, &depthimgviewinfo, nullptr, &mvkobjs.rddepthimageview) != VK_SUCCESS) {
		return false;
	}


	return true;
}

bool vkrenderer::createswapchain() {
	vkb::SwapchainBuilder swapchainbuild{ mvkobjs.rdvkbdevice };
	auto swapchainbuilret = swapchainbuild.set_old_swapchain(mvkobjs.rdvkbswapchain).set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR).build();
	if (!swapchainbuilret) {
		return false;
	}
	vkb::destroy_swapchain(mvkobjs.rdvkbswapchain);
	mvkobjs.rdvkbswapchain = swapchainbuilret.value();
	return true;
}
bool vkrenderer::recreateswapchain() {
	glfwGetFramebufferSize(mvkobjs.rdwind, &mvkobjs.rdwidth, &mvkobjs.rdheight);
	while (mvkobjs.rdwidth == 0 || mvkobjs.rdheight == 0) {
		glfwGetFramebufferSize(mvkobjs.rdwind, &mvkobjs.rdwidth, &mvkobjs.rdheight);
		glfwWaitEvents();
	}
	vkDeviceWaitIdle(mvkobjs.rdvkbdevice.device);
	framebuffer::cleanup(mvkobjs);
	vkDestroyImageView(mvkobjs.rdvkbdevice.device, mvkobjs.rddepthimageview, nullptr);
	vmaDestroyImage(mvkobjs.rdallocator, mvkobjs.rddepthimage, mvkobjs.rddepthimagealloc);

	mvkobjs.rdvkbswapchain.destroy_image_views(mvkobjs.rdswapchainimageviews);


	if (!createswapchain()) {
		return false;
	}if (!createdepthbuffer()) {
		return false;
	}if (!createframebuffer()) {
		return false;
	}

	return true;
}
bool vkrenderer::createrenderpass() {
	if (!renderpass::init(mvkobjs))return false;
	return true;
}
bool vkrenderer::createframebuffer() {
	if (!framebuffer::init(mvkobjs))return false;
	return true;
}
bool vkrenderer::createcommandpool() {
	if (!commandpool::init(mvkobjs, mvkobjs.rdcommandpool[0]))return false;
	if (!commandpool::init(mvkobjs, mvkobjs.rdcommandpool[1]))return false;
	return true;
}
bool vkrenderer::createcommandbuffer() {
	if (!commandbuffer::init(mvkobjs,mvkobjs.rdcommandpool[0], mvkobjs.rdcommandbuffer[0]))return false;
	if (!commandbuffer::init(mvkobjs,mvkobjs.rdcommandpool[1], mvkobjs.rdcommandbuffer[1]))return false;
	return true;
}
bool vkrenderer::createsyncobjects() {
	if (!vksyncobjects::init(mvkobjs))return false;
	return true;
}
bool vkrenderer::initvma() {
	VmaAllocatorCreateInfo allocinfo{};
	allocinfo.physicalDevice = mvkobjs.rdvkbphysdev.physical_device;
	allocinfo.device = mvkobjs.rdvkbdevice.device;
	allocinfo.instance = mvkobjs.rdvkbinstance.instance;
	if (vmaCreateAllocator(&allocinfo, &mvkobjs.rdallocator) != VK_SUCCESS) {
		return false;
	}
	return true;
}

bool vkrenderer::initui() {
	if(!mui.init(mvkobjs))return false;
	return true;
}

bool vkrenderer::initgameui(){
	if (!mui.init(mvkobjs)) {
		return false;
	}
	return true;
}


void vkrenderer::cleanup() {
	vkDeviceWaitIdle(mvkobjs.rdvkbdevice.device);


	for (size_t i{ 0 }; i < mpgltf.size(); i++) {
		mpgltf[i]->cleanupmodels(mvkobjs);
	}

	for (size_t i{ 0 }; i < mstatic0.size(); i++) {
		mstatic0[i]->cleanupmodels(mvkobjs);
	}
	if(mplayer)
		mplayer->cleanupmodels(mvkobjs);
	if (mground)
		mground->cleanupmodels(mvkobjs);

	if(lifebar)
	lifebar->cleanupmodels(mvkobjs);


	mui.cleanup(mvkobjs);


	//cleanmainmenu();
	//cleanloading();

	vksyncobjects::cleanup(mvkobjs);
	commandbuffer::cleanup(mvkobjs, mvkobjs.rdcommandpool[0], mvkobjs.rdcommandbuffer[0]);
	commandpool::cleanup(mvkobjs, mvkobjs.rdcommandpool[0]);
	framebuffer::cleanup(mvkobjs);


	for (size_t i{ 0 }; i < mpgltf.size(); i++) {
		mpgltf[i]->cleanuplines(mvkobjs);
	}

	for (size_t i{ 0 }; i < mstatic0.size(); i++) {
		mstatic0[i]->cleanuplines(mvkobjs);
	}

	if (mplayer)
		mplayer->cleanuplines(mvkobjs);
	if (mground)
		mground->cleanuplines(mvkobjs);
	if(lifebar)
	lifebar->cleanuplines(mvkobjs);


	renderpass::cleanup(mvkobjs);

	for (size_t i{ 0 }; i < mpgltf.size(); i++) {
		mpgltf[i]->cleanupbuffers(mvkobjs);
	}

	for (size_t i{ 0 }; i < mstatic0.size(); i++) {
		mstatic0[i]->cleanupbuffers(mvkobjs);
	}

	if (mplayer)
	mplayer->cleanupbuffers(mvkobjs);
	if (mground)
		mground->cleanupbuffers(mvkobjs);

	vkDestroyImageView(mvkobjs.rdvkbdevice.device, mvkobjs.rddepthimageview, nullptr);
	vmaDestroyImage(mvkobjs.rdallocator, mvkobjs.rddepthimage, mvkobjs.rddepthimagealloc);
	vmaDestroyAllocator(mvkobjs.rdallocator);

	mvkobjs.rdvkbswapchain.destroy_image_views(mvkobjs.rdswapchainimageviews);
	vkb::destroy_swapchain(mvkobjs.rdvkbswapchain);

	vkb::destroy_device(mvkobjs.rdvkbdevice);
	vkb::destroy_surface(mvkobjs.rdvkbinstance.instance, msurface);
	vkb::destroy_instance(mvkobjs.rdvkbinstance);


}



void vkrenderer::handlekeymenu(int key, int scancode, int action, int mods) {

	if (glfwGetKey(mvkobjs.rdwind, GLFW_KEY_F4) == GLFW_PRESS) {
		if (mvkobjs.rdfullscreen)glfwSetWindowMonitor(mvkobjs.rdwind, nullptr, 100, 200, 900, 600, GLFW_DONT_CARE);
		else {
			glfwSetWindowMonitor(mvkobjs.rdwind, mvkobjs.rdmonitor, 0, 0, mvkobjs.rdmode->width, mvkobjs.rdmode->height, mvkobjs.rdmode->refreshRate);
		}
		mvkobjs.rdfullscreen = !mvkobjs.rdfullscreen;
	}
	if (glfwGetKey(mvkobjs.rdwind, GLFW_KEY_F3) == GLFW_PRESS) {
		mui.setnetwork = !mui.setnetwork;
	}

	if (glfwGetKey(mvkobjs.rdwind, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(mvkobjs.rdwind, true);
	}
	//if (glfwGetKey(mvkobjs.rdwind, GLFW_KEY_ENTER) == GLFW_PRESS) {
	//	ImGui_ImplGlfw_InstallCallbacks(mvkobjs.rdwind);
	//}

}


void vkrenderer::handlekey(int key, int scancode, int action, int mods)
{
	if (glfwGetKey(mvkobjs.rdwind, GLFW_KEY_SPACE) == GLFW_PRESS) {
		switchshader = !switchshader;
	}
	if (glfwGetKey(mvkobjs.rdwind, GLFW_KEY_BACKSPACE) == GLFW_PRESS) {
		mui.backspace();
	}
	//if (glfwGetKey(mvkobjs.rdwind, GLFW_KEY_ENTER) == GLFW_PRESS) {
	//}
	if (glfwGetKey(mvkobjs.rdwind, GLFW_KEY_F4) == GLFW_PRESS) {
		if (mvkobjs.rdfullscreen)glfwSetWindowMonitor(mvkobjs.rdwind, nullptr, 100, 200, 900, 600, GLFW_DONT_CARE);
		else {
			glfwSetWindowMonitor(mvkobjs.rdwind, mvkobjs.rdmonitor, 0, 0, mvkobjs.rdmode->width, mvkobjs.rdmode->height, mvkobjs.rdmode->refreshRate);
		}
		mvkobjs.rdfullscreen = !mvkobjs.rdfullscreen;
	}


	if (glfwGetKey(mvkobjs.rdwind, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		if (paused) {
			paused = false;
		} else {
			paused = true;
			pausebgntime = glfwGetTime();
		}
		//glfwSetWindowShouldClose(mvkobjs.rdwind, true);
	}
	if (glfwGetKey(mvkobjs.rdwind, GLFW_KEY_R) == GLFW_PRESS) {
		if (!decaying) {
			mplayer->freezedecay();
			decaying = true;
			decaystart = glfwGetTime();
			mplayer->createdecayinstances(mvkobjs);
			{
				double x;
				double y;
				glfwGetCursorPos(mvkobjs.rdwind, &x, &y);
				//if (glm::abs(x - lastmousexy.x) > 100.0 || glm::abs(y - lastmousexy.y) > 100.0) {
				//	x += 1.0;
				//	y -= 1.0;
				//}
				lastmousexy = glm::vec<2, double>{ x,y };
				x = (2.0 * (x / (double)mvkobjs.rdwidth)) - 1.0;
				y = (2.0 * (y / (double)mvkobjs.rdheight)) - 1.0;
				float d{ 0.0f };
				glm::vec3 cfor{ mcam.mforward };
				float rotangx{ (float)(1.5*x * -0.523599f) };
				float rotangy{ (float)(1.5*y * (-0.523599f * mvkobjs.rdheight / mvkobjs.rdwidth)) };
				cfor = glm::rotate(cfor, rotangx, mcam.mup);
				cfor = glm::rotate(cfor, rotangy, mcam.mright);
				for (int i{ 0 }; i < 100000; i++) {
					float dt = map2(mvkobjs.rdcamwpos + cfor * d);
					d += dt;
					if (dt < 0.00001f || d>10000.0f)break;
				}


				if (d < 10000.0f) {
					modelsettings s = mplayer->getinst(0)->getinstancesettings();
					decaypos = s.msworldpos;
					playerlookto = glm::normalize(mvkobjs.rdcamwpos + cfor * d - s.msworldpos);
					movediff = glm::vec2(glm::abs(glm::vec3((mvkobjs.rdcamwpos + cfor * d) - s.msworldpos)).x, glm::abs(glm::vec3((mvkobjs.rdcamwpos + cfor * d) - s.msworldpos)).z);

					if (movediff.x > 2.1f || movediff.y > 2.1f) {
						s.msworldpos = mvkobjs.rdcamwpos + cfor * d;


						s.msworldrot.y = glm::degrees(glm::atan(playerlookto.x, playerlookto.z));

						s.msanimclip = 9;


						playermoving = false;

						mplayer->getinst(0)->setinstancesettings(s);
						mplayer->getinst(0)->checkforupdates();
					}
				}
			}
		}
	}
}
int clickz{ 0 };

void vkrenderer::startmoving() {
	modelsettings s = mplayer->getinst(0)->getinstancesettings();
	// = playermoveto- s.msworldpos;
	mplayer->getinst(0)->setinstancesettings(s);
	mplayer->getinst(0)->checkforupdates();
}

void vkrenderer::moveplayer(){
	modelsettings s = mplayer->getinst(0)->getinstancesettings();
	// = playermoveto- s.msworldpos;
	glm::vec3 diff = playermoveto - s.msworldpos;
	//if ((diff.x > 0.001f || diff.x < -0.001f) || (diff.y > 0.001f||diff.y <-0.001f)) {
		s.msworldpos += glm::normalize(diff)*2.0f;
	//}
	/*else if (diff.y > 0.001f || diff.y < -0.001f) {
		s.msworldpos += (glm::normalize(diff) * 10.0f);
	} else if (diff.x > 0.001f || diff.x < -0.001f) {
		s.msworldpos += (glm::normalize(diff) * 10.0f);
	}*/ 
	//else {
		if(glm::abs(diff.x)<2.1f && glm::abs(diff.z)< 2.1f){
			s.msworldpos = playermoveto;
			s.msanimclip = 9;
			playermoving = false;
		}
	//}
	mplayer->getinst(0)->setinstancesettings(s);
	mplayer->getinst(0)->checkforupdates();
}

void vkrenderer::moveenemies(){
	modelsettings ps = mplayer->getinst(0)->getinstancesettings();
	if (!decaying) {
		for (int i{ 0 }; i < mpgltf[0]->getnuminstances(); i++) {
			modelsettings es = mpgltf[0]->getinst(i)->getinstancesettings();
			glm::vec3 diff = ps.msworldpos - es.msworldpos;
			if (glm::abs(diff.x) < 100.0f && glm::abs(diff.z) < 100.0f) {
				if (es.msanimclip > 1) {
					es.msanimclip = 1;
					es.msanimtimepos = 0.0f;
				}
				if (es.msanimtimepos > 1.2f) {
					playerhp -= 0.001f;
				}
				mpgltf[0]->getinst(i)->setinstancesettings(es);
				mpgltf[0]->getinst(i)->checkforupdates();
			} else {
				es.msworldpos += glm::normalize(diff) * 1.0f;
				es.msanimclip = 14;
				es.msworldrot.y = glm::degrees(glm::atan(diff.x, diff.z));
				mpgltf[0]->getinst(i)->setinstancesettings(es);
				mpgltf[0]->getinst(i)->checkforupdates();
			}
		}
	} else {
		for (int i{ 0 }; i < mpgltf[0]->getnuminstances(); i++) {
			modelsettings es = mpgltf[0]->getinst(i)->getinstancesettings();
			glm::vec3 diff = decaypos - es.msworldpos;
			if (glm::abs(diff.x) < 100.0f && glm::abs(diff.z) < 100.0f) {
				if (es.msanimclip > 1) {
					es.msanimclip = 1;
				}
				mpgltf[0]->getinst(i)->setinstancesettings(es);
				mpgltf[0]->getinst(i)->checkforupdates();
			} else {
				es.msworldpos += glm::normalize(diff) * 1.0f;
				es.msanimclip = 14;
				es.msworldrot.y = glm::degrees(glm::atan(diff.x, diff.z));
				mpgltf[0]->getinst(i)->setinstancesettings(es);
				mpgltf[0]->getinst(i)->checkforupdates();
			}
		}
	}
}

void vkrenderer::handleclick(int key, int action, int mods)
{
	ImGuiIO& io = ImGui::GetIO();
	if (key >= 0 && key < ImGuiMouseButton_COUNT) {
		io.AddMouseButtonEvent(key, action == GLFW_PRESS);
	}
	if (io.WantCaptureMouse)return;
	


	if (key == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
		mlock = !mlock;
		if (mlock) {
			glfwSetInputMode(mvkobjs.rdwind, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			if (glfwRawMouseMotionSupported()) {
				glfwSetInputMode(mvkobjs.rdwind, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
			}
		} else {
			glfwSetInputMode(mvkobjs.rdwind, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
	}
}
void vkrenderer::handlemouse(double x, double y){
	ImGuiIO& io = ImGui::GetIO();
	io.AddMousePosEvent((float)x, (float)y);
	if (io.WantCaptureMouse) {
		return;
	}

	int relativex = static_cast<int>(x) - mousex;
	int relativey = static_cast<int>(y) - mousey;

	//if (glfwGetMouseButton(mvkobjs.rdwind, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {

	//}


	if (!paused) {
		if (mlock) {

			mpersviewmats.at(0) = mcam.getview(mvkobjs);
			//mpersviewmats.at(1) = glm::perspective(glm::radians(static_cast<float>(mvkobjs.rdfov)), static_cast<float>(mvkobjs.rdwidth) / static_cast<float>(mvkobjs.rdheight), 0.01f, 6000.0f);

			mvkobjs.rdazimuth += relativex / 10.0;

			mvkobjs.rdelevation -= relativey / 10.0;

			if (mvkobjs.rdelevation > 89.0) {
				mvkobjs.rdelevation = 89.0;
			}

			if (mvkobjs.rdelevation < -89.0) {
				mvkobjs.rdelevation = -89.0;
			}


			if (mvkobjs.rdazimuth > 90.0) {
				mvkobjs.rdazimuth = 90.0;
			}
			if (mvkobjs.rdazimuth < -90.0) {
				mvkobjs.rdazimuth = -90.0;
			}


			mousex = static_cast<int>(x);
			mousey = static_cast<int>(y);
		}
	}
	
}
void vkrenderer::setsize(unsigned int w, unsigned int h) {
	mvkobjs.rdwidth = w;
	mvkobjs.rdheight = h;
	if(!w||!h)
		mpersviewmats.at(1) = glm::perspective(glm::radians(static_cast<float>(mvkobjs.rdvkbswapchain.extent.width)), static_cast<float>(mvkobjs.rdvkbswapchain.extent.height) / static_cast<float>(mvkobjs.rdheight), 0.01f, 60000.0f);
	else
		mpersviewmats.at(1) = glm::perspective(mvkobjs.rdfov, static_cast<float>(mvkobjs.rdwidth) / static_cast<float>(mvkobjs.rdheight), 1.0f, 6000.0f);

}
void vkrenderer::toggleshader() {
	mvkobjs.rdswitchshader = !mvkobjs.rdswitchshader;
}
void vkrenderer::movecam() {


	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureMouse) {
		return;
	}

	mpersviewmats.at(0) = mcam.getview(mvkobjs);

	mvkobjs.rdcamforward = 0;
	if (glfwGetKey(mvkobjs.rdwind, GLFW_KEY_W) == GLFW_PRESS) {
		mvkobjs.rdcamforward += 200;
	}
	if (glfwGetKey(mvkobjs.rdwind, GLFW_KEY_S) == GLFW_PRESS) {
		mvkobjs.rdcamforward -= 200;
	}
	mvkobjs.rdcamright = 0;
	if (glfwGetKey(mvkobjs.rdwind, GLFW_KEY_A) == GLFW_PRESS) {
		mvkobjs.rdcamright += 200;
	}
	if (glfwGetKey(mvkobjs.rdwind, GLFW_KEY_D) == GLFW_PRESS) {
		mvkobjs.rdcamright -= 200;
	}
	mvkobjs.rdcamup = 0;
	if (glfwGetKey(mvkobjs.rdwind, GLFW_KEY_E) == GLFW_PRESS) {
		mvkobjs.rdcamup += 200;
	}
	if (glfwGetKey(mvkobjs.rdwind, GLFW_KEY_Q) == GLFW_PRESS) {
		mvkobjs.rdcamup -= 200;
	}

	//if ((glfwGetKey(mvkobjs.rdwind, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)) {
	//	mvkobjs.rdcamforward *= 400;
	//	mvkobjs.rdcamright *= 400;
	//	mvkobjs.rdcamup *= 400;
	//}
	if (glfwGetMouseButton(mvkobjs.rdwind, GLFW_MOUSE_BUTTON_LEFT) != GLFW_RELEASE) {
		if (!paused) {
			double x;
			double y;
			glfwGetCursorPos(mvkobjs.rdwind, &x, &y);
			//if (glm::abs(x - lastmousexy.x) > 100.0 || glm::abs(y - lastmousexy.y) > 100.0) {
			//	x += 1.0;
			//	y -= 1.0;
			//}
			lastmousexy = glm::vec<2, double>{ x,y };
			x = (2.0 * (x / (double)mvkobjs.rdwidth)) - 1.0;
			y = (2.0 * (y / (double)mvkobjs.rdheight)) - 1.0;
			//std::cout << x << "   " << y << std::endl;
			float d{ 0.0f };
			glm::vec3 cfor{ mcam.mforward };
			float rotangx{ (float)(1.5 * x * -0.523599f) };
			float rotangy{ (float)(1.5 * y * (-0.523599f * mvkobjs.rdheight / mvkobjs.rdwidth)) };
			cfor = glm::rotate(cfor, rotangx, mcam.mup);
			cfor = glm::rotate(cfor, rotangy, mcam.mright);
			for (int i{ 0 }; i < 100000; i++) {
				float dt = map2(mvkobjs.rdcamwpos + cfor * d);
				d += dt;
				if (dt < 0.00001f || d>10000.0f)break;
			}

			if (d < 10000.0f) {
				modelsettings s = mplayer->getinst(0)->getinstancesettings();

				playerlookto = glm::normalize(mvkobjs.rdcamwpos + cfor * d - s.msworldpos);
				movediff = glm::vec2(glm::abs(glm::vec3((mvkobjs.rdcamwpos + cfor * d) - s.msworldpos)).x, glm::abs(glm::vec3((mvkobjs.rdcamwpos + cfor * d) - s.msworldpos)).z);

				if (movediff.x>2.1f||movediff.y>2.1f) {
					playermoveto = mvkobjs.rdcamwpos + cfor * d;

					s.msworldrot.y = glm::degrees(glm::atan(playerlookto.x, playerlookto.z));

					s.msanimclip = 15;

					mplayer->getinst(0)->setinstancesettings(s);
					mplayer->getinst(0)->checkforupdates();

					playermoving = true;
				}
			}
		}
	}


}

bool vkrenderer::checkphp() {
	if (playerhp < 0.0f)return false;
	return true;
}
bool vkrenderer::draw() {

	if (!paused) {

		if (!checkphp())paused = true;

		double tick = glfwGetTime();
		mvkobjs.rdtickdiff = tick - mlasttick;
		mvkobjs.rdframetime = mframetimer.stop();
		mframetimer.start();


		if (vkWaitForFences(mvkobjs.rdvkbdevice.device, 1, &mvkobjs.rdrenderfence, VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
			return false;
		}
		if (vkResetFences(mvkobjs.rdvkbdevice.device, 1, &mvkobjs.rdrenderfence) != VK_SUCCESS)return false;

		uint32_t imgidx = 0;
		VkResult res = vkAcquireNextImageKHR(mvkobjs.rdvkbdevice.device, mvkobjs.rdvkbswapchain.swapchain, UINT64_MAX, mvkobjs.rdpresentsemaphore, VK_NULL_HANDLE, &imgidx);
		if (res == VK_ERROR_OUT_OF_DATE_KHR) {
			return recreateswapchain();
		} else {
			if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) {
				return false;
			}
		}



		VkClearValue colorclearvalue;
		colorclearvalue.color = { {0.12f,0.12f,0.12f,1.0f } };

		VkClearValue depthvalue;
		depthvalue.depthStencil.depth = 1.0f;

		VkClearValue clearvals[] = { colorclearvalue,depthvalue };


		VkRenderPassBeginInfo rpinfo{};
		rpinfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		rpinfo.renderPass = mvkobjs.rdrenderpass;

		rpinfo.renderArea.offset.x = 0;
		rpinfo.renderArea.offset.y = 0;
		rpinfo.renderArea.extent = mvkobjs.rdvkbswapchain.extent;
		rpinfo.framebuffer = mvkobjs.rdframebuffers[imgidx];

		rpinfo.clearValueCount = 2;
		rpinfo.pClearValues = clearvals;

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = static_cast<float>(mvkobjs.rdvkbswapchain.extent.height);
		viewport.width = static_cast<float>(mvkobjs.rdvkbswapchain.extent.width);
		viewport.height = -static_cast<float>(mvkobjs.rdvkbswapchain.extent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0,0 };
		scissor.extent = mvkobjs.rdvkbswapchain.extent;





		mmatgentimer.start();


		mvkobjs.rdiktime = 0.0f;



		mplayer->updateanims();

		for (size_t i{ 0 }; i < mpgltf.size(); i++) {
			mpgltf[i]->updateanims();
		}


		int selectedInstance = 0;

		mvkobjs.rdmatrixgeneratetime = mmatgentimer.stop();




		if (vkResetCommandBuffer(mvkobjs.rdcommandbuffer[0], 0) != VK_SUCCESS)return false;

		VkCommandBufferBeginInfo cmdbgninfo{};
		cmdbgninfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdbgninfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		if (vkBeginCommandBuffer(mvkobjs.rdcommandbuffer[0], &cmdbgninfo) != VK_SUCCESS)return false;

		muploadtovbotimer.start();

		mplayer->uploadvboebo(mvkobjs);

		for (size_t i{ 0 }; i < mpgltf.size(); i++) {
			mpgltf[i]->uploadvboebo(mvkobjs);
		}

		for (size_t i{ 0 }; i < mstatic0.size(); i++) {
			mstatic0[i]->uploadvboebo(mvkobjs);
		}
		mground->uploadvboebo(mvkobjs);

		lifebar->uploadvboebo(mvkobjs);

		mvkobjs.rduploadtovbotime = muploadtovbotimer.stop();


		mplayer->updatemats();

		for (size_t i{ 0 }; i < mpgltf.size(); i++) {
			mpgltf[i]->updatemats();
		}






		vkCmdBeginRenderPass(mvkobjs.rdcommandbuffer[0], &rpinfo, VK_SUBPASS_CONTENTS_INLINE);





		vkCmdSetViewport(mvkobjs.rdcommandbuffer[0], 0, 1, &viewport);
		vkCmdSetScissor(mvkobjs.rdcommandbuffer[0], 0, 1, &scissor);

		mground->draw(mvkobjs);

		for (size_t i{ 0 }; i < mstatic0.size(); i++) {
			mstatic0[i]->draw(mvkobjs);
		}

		for (size_t i{ 0 }; i < mpgltf.size(); i++) {
			mpgltf[i]->draw(mvkobjs);
		}

		mplayer->draw(mvkobjs);

		if (decaying) {
			decaytime = glfwGetTime() - decaystart-pausetime;
			mplayer->drawdecays(mvkobjs, decaytime, &decaying);
		}
		if (!decaying)pausetime = 0.0;

		lifetime = glfwGetTime();
		lifetime2 = glfwGetTime();

		lifebar->draw(mvkobjs,lifetime, lifetime2,playerhp);

		muigentimer.start();
		muidrawtimer.start();
		modelsettings settings = mplayer->getinstsettings();
		//mui.createchat(mvkobjs);
		//mui.render(mvkobjs,mvkobjs.rdcommandbuffer[0]);
		mui.createdbgframe(mvkobjs, settings, mnobjs);
		mplayer->getinst(0)->setinstancesettings(settings);
		mplayer->getinst(0)->checkforupdates();
		mui.render(mvkobjs, mvkobjs.rdcommandbuffer[0]);
		mvkobjs.rduigeneratetime = muigentimer.stop();
		mvkobjs.rduidrawtime = muidrawtimer.stop();



		vkCmdEndRenderPass(mvkobjs.rdcommandbuffer[0]);



		muploadtoubotimer.start();

		mplayer->uploadubossbo(mvkobjs, mpersviewmats);


		for (size_t i{ 0 }; i < mpgltf.size(); i++) {
			mpgltf[i]->uploadubossbo(mvkobjs, mpersviewmats);
		}

		for (size_t i{ 0 }; i < mstatic0.size(); i++) {
			mstatic0[i]->uploadubossbo(mvkobjs, mpersviewmats);
		}

		mground->uploadubossbo(mvkobjs, mpersviewmats);


		mvkobjs.rduploadtoubotime = muploadtoubotimer.stop();


		if (vkEndCommandBuffer(mvkobjs.rdcommandbuffer[0]) != VK_SUCCESS)return false;


		movecam();//////////////////////////////////////
		moveenemies();

		if (playermoving)moveplayer();


		VkSubmitInfo submitinfo{};
		submitinfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkPipelineStageFlags waitstage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		submitinfo.pWaitDstStageMask = &waitstage;

		submitinfo.waitSemaphoreCount = 1;
		submitinfo.pWaitSemaphores = &mvkobjs.rdpresentsemaphore;

		submitinfo.signalSemaphoreCount = 1;
		submitinfo.pSignalSemaphores = &mvkobjs.rdrendersemaphore;

		submitinfo.commandBufferCount = 1;
		submitinfo.pCommandBuffers = &mvkobjs.rdcommandbuffer.at(0);


		mvkobjs.mtx2->lock();
		if (vkQueueSubmit(mvkobjs.rdgraphicsqueue, 1, &submitinfo, mvkobjs.rdrenderfence) != VK_SUCCESS) {
			return false;
		}
		mvkobjs.mtx2->unlock();

		VkPresentInfoKHR presentinfo{};
		presentinfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentinfo.waitSemaphoreCount = 1;
		presentinfo.pWaitSemaphores = &mvkobjs.rdrendersemaphore;

		presentinfo.swapchainCount = 1;
		presentinfo.pSwapchains = &mvkobjs.rdvkbswapchain.swapchain;

		presentinfo.pImageIndices = &imgidx;


		mvkobjs.mtx2->lock();
		res = vkQueuePresentKHR(mvkobjs.rdpresentqueue, &presentinfo);

		mvkobjs.mtx2->unlock();

		if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
			return recreateswapchain();
		} else {
			if (res != VK_SUCCESS) {
				return false;
			}
		}
		mlasttick = tick;

		return true;
		} 
	else {


		if (vkWaitForFences(mvkobjs.rdvkbdevice.device, 1, &mvkobjs.rdrenderfence, VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
			return false;
		}
		if (vkResetFences(mvkobjs.rdvkbdevice.device, 1, &mvkobjs.rdrenderfence) != VK_SUCCESS)return false;

		uint32_t imgidx = 0;
		VkResult res = vkAcquireNextImageKHR(mvkobjs.rdvkbdevice.device, mvkobjs.rdvkbswapchain.swapchain, UINT64_MAX, mvkobjs.rdpresentsemaphore, VK_NULL_HANDLE, &imgidx);
		if (res == VK_ERROR_OUT_OF_DATE_KHR) {
			return recreateswapchain();
		} else {
			if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) {
				return false;
			}
		}




			VkClearValue colorclearvalue;
			colorclearvalue.color = { {0.0048f,0.0048f,0.0048f,1.0f } };

			VkClearValue depthvalue;
			depthvalue.depthStencil.depth = 1.0f;

			VkClearValue clearvals[] = { colorclearvalue,depthvalue };


			VkRenderPassBeginInfo rpinfo{};
			rpinfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			rpinfo.renderPass = mvkobjs.rdrenderpass;

			rpinfo.renderArea.offset.x = 0;
			rpinfo.renderArea.offset.y = 0;
			rpinfo.renderArea.extent = mvkobjs.rdvkbswapchain.extent;
			rpinfo.framebuffer = mvkobjs.rdframebuffers[imgidx];

			rpinfo.clearValueCount = 2;
			rpinfo.pClearValues = clearvals;

			VkViewport viewport{};
			viewport.x = 0.0f;
			viewport.y = static_cast<float>(mvkobjs.rdvkbswapchain.extent.height);
			viewport.width = static_cast<float>(mvkobjs.rdvkbswapchain.extent.width);
			viewport.height = -static_cast<float>(mvkobjs.rdvkbswapchain.extent.height);
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;

			VkRect2D scissor{};
			scissor.offset = { 0,0 };
			scissor.extent = mvkobjs.rdvkbswapchain.extent;




			if (vkResetCommandBuffer(mvkobjs.rdcommandbuffer[0], 0) != VK_SUCCESS)return false;

			VkCommandBufferBeginInfo cmdbgninfo{};
			cmdbgninfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdbgninfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			if (vkBeginCommandBuffer(mvkobjs.rdcommandbuffer[0], &cmdbgninfo) != VK_SUCCESS)return false;


			mplayer->uploadvboebo(mvkobjs);

			for (size_t i{ 0 }; i < mpgltf.size(); i++) {
				mpgltf[i]->uploadvboebo(mvkobjs);
			}

			for (size_t i{ 0 }; i < mstatic0.size(); i++) {
				mstatic0[i]->uploadvboebo(mvkobjs);
			}
			mground->uploadvboebo(mvkobjs);


			vkCmdBeginRenderPass(mvkobjs.rdcommandbuffer[0], &rpinfo, VK_SUBPASS_CONTENTS_INLINE);





			vkCmdSetViewport(mvkobjs.rdcommandbuffer[0], 0, 1, &viewport);
			vkCmdSetScissor(mvkobjs.rdcommandbuffer[0], 0, 1, &scissor);





			mground->draw(mvkobjs);

			for (size_t i{ 0 }; i < mstatic0.size(); i++) {
				mstatic0[i]->draw(mvkobjs);
			}

			for (size_t i{ 0 }; i < mpgltf.size(); i++) {
				mpgltf[i]->draw(mvkobjs);
			}

			mplayer->draw(mvkobjs);

			if (decaying) {
				decaytime = glfwGetTime() - decaystart-pausetime;
				mplayer->drawdecays(mvkobjs, decaytime, &decaying);
			}

			lifetime = glfwGetTime() - pausetime;
			lifetime2 = glfwGetTime();

			lifebar->draw(mvkobjs,lifetime, lifetime2,playerhp);







			if(mui.createpausebuttons(mvkobjs))paused=false;
			mui.render(mvkobjs, mvkobjs.rdcommandbuffer[0]);





			vkCmdEndRenderPass(mvkobjs.rdcommandbuffer[0]);


			mplayer->uploadubossbo(mvkobjs, mpersviewmats);


			for (size_t i{ 0 }; i < mpgltf.size(); i++) {
				mpgltf[i]->uploadubossbo(mvkobjs, mpersviewmats);
			}

			for (size_t i{ 0 }; i < mstatic0.size(); i++) {
				mstatic0[i]->uploadubossbo(mvkobjs, mpersviewmats);
			}

			mground->uploadubossbo(mvkobjs, mpersviewmats);





			if (vkEndCommandBuffer(mvkobjs.rdcommandbuffer[0]) != VK_SUCCESS)return false;




			VkSubmitInfo submitinfo{};
			submitinfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

			VkPipelineStageFlags waitstage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			submitinfo.pWaitDstStageMask = &waitstage;

			submitinfo.waitSemaphoreCount = 1;
			submitinfo.pWaitSemaphores = &mvkobjs.rdpresentsemaphore;

			submitinfo.signalSemaphoreCount = 1;
			submitinfo.pSignalSemaphores = &mvkobjs.rdrendersemaphore;

			submitinfo.commandBufferCount = 1;
			submitinfo.pCommandBuffers = &mvkobjs.rdcommandbuffer[0];

			mvkobjs.mtx2->lock();
			if (vkQueueSubmit(mvkobjs.rdgraphicsqueue, 1, &submitinfo, mvkobjs.rdrenderfence) != VK_SUCCESS) {
				return false;
			}
			mvkobjs.mtx2->unlock();


			VkPresentInfoKHR presentinfo{};
			presentinfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			presentinfo.waitSemaphoreCount = 1;
			presentinfo.pWaitSemaphores = &mvkobjs.rdrendersemaphore;

			presentinfo.swapchainCount = 1;
			presentinfo.pSwapchains = &mvkobjs.rdvkbswapchain.swapchain;

			presentinfo.pImageIndices = &imgidx;

			mvkobjs.mtx2->lock();
			res = vkQueuePresentKHR(mvkobjs.rdpresentqueue, &presentinfo);
			mvkobjs.mtx2->unlock();
			if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
				return recreateswapchain();
			} else {
				if (res != VK_SUCCESS) {
					return false;
				}
			}
			pausetime = glfwGetTime() - pausebgntime;


			return true;
	}
}


bool vkrenderer::drawmainmenu() {

	if (vkWaitForFences(mvkobjs.rdvkbdevice.device, 1, &mvkobjs.rdrenderfence, VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
		return false;
	}
	if (vkResetFences(mvkobjs.rdvkbdevice.device, 1, &mvkobjs.rdrenderfence) != VK_SUCCESS)return false;

	uint32_t imgidx = 0;
	VkResult res = vkAcquireNextImageKHR(mvkobjs.rdvkbdevice.device, mvkobjs.rdvkbswapchain.swapchain, UINT64_MAX, mvkobjs.rdpresentsemaphore, VK_NULL_HANDLE, &imgidx);
	if (res == VK_ERROR_OUT_OF_DATE_KHR) {
		return recreateswapchain();
	}
	else {
		if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) {
			return false;
		}
	}



	VkClearValue colorclearvalue;
	colorclearvalue.color = { {0.012f,0.012f,0.012f,1.0f } };

	VkClearValue depthvalue;
	depthvalue.depthStencil.depth = 1.0f;

	VkClearValue clearvals[] = { colorclearvalue,depthvalue };


	VkRenderPassBeginInfo rpinfo{};
	rpinfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpinfo.renderPass = mvkobjs.rdrenderpass;

	rpinfo.renderArea.offset.x = 0;
	rpinfo.renderArea.offset.y = 0;
	rpinfo.renderArea.extent = mvkobjs.rdvkbswapchain.extent;
	rpinfo.framebuffer = mvkobjs.rdframebuffers[imgidx];

	rpinfo.clearValueCount = 2;
	rpinfo.pClearValues = clearvals;

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = static_cast<float>(mvkobjs.rdvkbswapchain.extent.height);
	viewport.width = static_cast<float>(mvkobjs.rdvkbswapchain.extent.width);
	viewport.height = -static_cast<float>(mvkobjs.rdvkbswapchain.extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0,0 };
	scissor.extent = mvkobjs.rdvkbswapchain.extent;



	//mbackground->updateanims();


	if (vkResetCommandBuffer(mvkobjs.rdcommandbuffer[0], 0) != VK_SUCCESS)return false;

	VkCommandBufferBeginInfo cmdbgninfo{};
	cmdbgninfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdbgninfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	if (vkBeginCommandBuffer(mvkobjs.rdcommandbuffer[0], &cmdbgninfo) != VK_SUCCESS)return false;

	mbackground->uploadvboebo(mvkobjs);
	mmenubg->uploadvboebo(mvkobjs);

	//staticsettings s{};
	staticsettings s = mbackground->getinst(0)->getinstancesettings();
	//s.msworldpos.y = -20.0f;
	//s.msworldrot.y -= 1.57f;
	s.rotang += 0.0006f;
	mbackground->getinst(0)->setinstancesettings(s);
	mbackground->updatemats();

	vkCmdBeginRenderPass(mvkobjs.rdcommandbuffer[0], &rpinfo, VK_SUBPASS_CONTENTS_INLINE);




	vkCmdSetViewport(mvkobjs.rdcommandbuffer[0], 0, 1, &viewport);
	vkCmdSetScissor(mvkobjs.rdcommandbuffer[0], 0, 1, &scissor);


	mbackground->draw(mvkobjs);

	lifetime = glfwGetTime();

	mmenubg->draw(mvkobjs,lifetime,lifetime2,playerhp);

	rdscene = mui.createmainmenuframe(mvkobjs,mnobjs);
	mui.render(mvkobjs, mvkobjs.rdcommandbuffer[0]);




	vkCmdEndRenderPass(mvkobjs.rdcommandbuffer[0]);


	mbackground->uploadubossbo(mvkobjs, mpersviewmats);

	if (vkEndCommandBuffer(mvkobjs.rdcommandbuffer[0]) != VK_SUCCESS)return false;




	VkSubmitInfo submitinfo{};
	submitinfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkPipelineStageFlags waitstage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submitinfo.pWaitDstStageMask = &waitstage;

	submitinfo.waitSemaphoreCount = 1;
	submitinfo.pWaitSemaphores = &mvkobjs.rdpresentsemaphore;

	submitinfo.signalSemaphoreCount = 1;
	submitinfo.pSignalSemaphores = &mvkobjs.rdrendersemaphore;

	submitinfo.commandBufferCount = 1;
	submitinfo.pCommandBuffers = &mvkobjs.rdcommandbuffer[0];


mvkobjs.mtx2->lock();
	if (vkQueueSubmit(mvkobjs.rdgraphicsqueue, 1, &submitinfo, mvkobjs.rdrenderfence) != VK_SUCCESS) {
		return false;
	}
mvkobjs.mtx2->unlock();

	VkPresentInfoKHR presentinfo{};
	presentinfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentinfo.waitSemaphoreCount = 1;
	presentinfo.pWaitSemaphores = &mvkobjs.rdrendersemaphore;

	presentinfo.swapchainCount = 1;
	presentinfo.pSwapchains = &mvkobjs.rdvkbswapchain.swapchain;

	presentinfo.pImageIndices = &imgidx;

mvkobjs.mtx2->lock();
	res = vkQueuePresentKHR(mvkobjs.rdpresentqueue, &presentinfo);

mvkobjs.mtx2->unlock();

	if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
		return recreateswapchain();
	}
	else {
		if (res != VK_SUCCESS) {
			return false;
		}
	}



	return rdscene;
}
bool vkrenderer::drawloading() {

	if (vkWaitForFences(mvkobjs.rdvkbdevice.device, 1, &mvkobjs.rdrenderfence, VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
		return false;
	}
	if (vkResetFences(mvkobjs.rdvkbdevice.device, 1, &mvkobjs.rdrenderfence) != VK_SUCCESS)return false;

	uint32_t imgidx = 0;
	VkResult res = vkAcquireNextImageKHR(mvkobjs.rdvkbdevice.device, mvkobjs.rdvkbswapchain.swapchain, UINT64_MAX, mvkobjs.rdpresentsemaphore, VK_NULL_HANDLE, &imgidx);
	if (res == VK_ERROR_OUT_OF_DATE_KHR) {
		return recreateswapchain();
	}
	else {
		if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) {
			return false;
		}
	}



	VkClearValue colorclearvalue;
	colorclearvalue.color = { {0.0048f,0.0048f,0.0048f,1.0f } };

	VkClearValue depthvalue;
	depthvalue.depthStencil.depth = 1.0f;

	VkClearValue clearvals[] = { colorclearvalue,depthvalue };


	VkRenderPassBeginInfo rpinfo{};
	rpinfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpinfo.renderPass = mvkobjs.rdrenderpass;

	rpinfo.renderArea.offset.x = 0;
	rpinfo.renderArea.offset.y = 0;
	rpinfo.renderArea.extent = mvkobjs.rdvkbswapchain.extent;
	rpinfo.framebuffer = mvkobjs.rdframebuffers[imgidx];

	rpinfo.clearValueCount = 2;
	rpinfo.pClearValues = clearvals;

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = static_cast<float>(mvkobjs.rdvkbswapchain.extent.height);
	viewport.width = static_cast<float>(mvkobjs.rdvkbswapchain.extent.width);
	viewport.height = -static_cast<float>(mvkobjs.rdvkbswapchain.extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0,0 };
	scissor.extent = mvkobjs.rdvkbswapchain.extent;




	if (vkResetCommandBuffer(mvkobjs.rdcommandbuffer[1], 0) != VK_SUCCESS)return false;

	VkCommandBufferBeginInfo cmdbgninfo{};
	cmdbgninfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdbgninfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	if (vkBeginCommandBuffer(mvkobjs.rdcommandbuffer[1], &cmdbgninfo) != VK_SUCCESS)return false;



	vkCmdBeginRenderPass(mvkobjs.rdcommandbuffer[1], &rpinfo, VK_SUBPASS_CONTENTS_INLINE);





	vkCmdSetViewport(mvkobjs.rdcommandbuffer[1], 0, 1, &viewport);
	vkCmdSetScissor(mvkobjs.rdcommandbuffer[1], 0, 1, &scissor);



	rdscene = mui.createloadingscreen(mvkobjs);
	mui.render(mvkobjs, mvkobjs.rdcommandbuffer[1]);




	vkCmdEndRenderPass(mvkobjs.rdcommandbuffer[1]);

	if (vkEndCommandBuffer(mvkobjs.rdcommandbuffer[1]) != VK_SUCCESS)return false;




	VkSubmitInfo submitinfo{};
	submitinfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkPipelineStageFlags waitstage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submitinfo.pWaitDstStageMask = &waitstage;

	submitinfo.waitSemaphoreCount = 1;
	submitinfo.pWaitSemaphores = &mvkobjs.rdpresentsemaphore;

	submitinfo.signalSemaphoreCount = 1;
	submitinfo.pSignalSemaphores = &mvkobjs.rdrendersemaphore;

	submitinfo.commandBufferCount = 1;
	submitinfo.pCommandBuffers = &mvkobjs.rdcommandbuffer[1];


	//if (vkResetFences(mvkobjs.rdvkbdevice.device, 1, &mvkobjs.rdrenderfence) != VK_SUCCESS) {
	//	return false;
	//}
mvkobjs.mtx2->lock();
	if (vkQueueSubmit(mvkobjs.rdgraphicsqueue, 1, &submitinfo, mvkobjs.rdrenderfence) != VK_SUCCESS) {
		return false;
	}
mvkobjs.mtx2->unlock();

	//if (vkWaitForFences(mvkobjs.rdvkbdevice.device, 1, &mvkobjs.rdrenderfence, VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
	//	return false;
	//}

	VkPresentInfoKHR presentinfo{};
	presentinfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentinfo.waitSemaphoreCount = 1;
	presentinfo.pWaitSemaphores = &mvkobjs.rdrendersemaphore;

	presentinfo.swapchainCount = 1;
	presentinfo.pSwapchains = &mvkobjs.rdvkbswapchain.swapchain;

	presentinfo.pImageIndices = &imgidx;

mvkobjs.mtx2->lock();
	res = vkQueuePresentKHR(mvkobjs.rdpresentqueue, &presentinfo);
mvkobjs.mtx2->unlock();
	if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
		return recreateswapchain();
	}
	else {
		if (res != VK_SUCCESS) {
			return false;
		}
	}



	return rdscene;
}

bool vkrenderer::drawblank(){
	if (vkWaitForFences(mvkobjs.rdvkbdevice.device, 1, &mvkobjs.rdrenderfence, VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
		return false;
	}
	if (vkResetFences(mvkobjs.rdvkbdevice.device, 1, &mvkobjs.rdrenderfence) != VK_SUCCESS)return false;

	uint32_t imgidx = 0;
	VkResult res = vkAcquireNextImageKHR(mvkobjs.rdvkbdevice.device, mvkobjs.rdvkbswapchain.swapchain, UINT64_MAX, mvkobjs.rdpresentsemaphore, VK_NULL_HANDLE, &imgidx);
	if (res == VK_ERROR_OUT_OF_DATE_KHR) {
		return recreateswapchain();
	}
	else {
		if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) {
			return false;
		}
	}



	VkClearValue colorclearvalue;
	colorclearvalue.color = { {0.012f,0.012f,0.012f,1.0f } };

	VkClearValue depthvalue;
	depthvalue.depthStencil.depth = 1.0f;

	VkClearValue clearvals[] = { colorclearvalue,depthvalue };


	VkRenderPassBeginInfo rpinfo{};
	rpinfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpinfo.renderPass = mvkobjs.rdrenderpass;

	rpinfo.renderArea.offset.x = 0;
	rpinfo.renderArea.offset.y = 0;
	rpinfo.renderArea.extent = mvkobjs.rdvkbswapchain.extent;
	rpinfo.framebuffer = mvkobjs.rdframebuffers[imgidx];

	rpinfo.clearValueCount = 2;
	rpinfo.pClearValues = clearvals;

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = static_cast<float>(mvkobjs.rdvkbswapchain.extent.height);
	viewport.width = static_cast<float>(mvkobjs.rdvkbswapchain.extent.width);
	viewport.height = -static_cast<float>(mvkobjs.rdvkbswapchain.extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0,0 };
	scissor.extent = mvkobjs.rdvkbswapchain.extent;



	if (vkResetCommandBuffer(mvkobjs.rdcommandbuffer[0], 0) != VK_SUCCESS)return false;

	VkCommandBufferBeginInfo cmdbgninfo{};
	cmdbgninfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdbgninfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	if (vkBeginCommandBuffer(mvkobjs.rdcommandbuffer[0], &cmdbgninfo) != VK_SUCCESS)return false;



	vkCmdBeginRenderPass(mvkobjs.rdcommandbuffer[0], &rpinfo, VK_SUBPASS_CONTENTS_INLINE);




	vkCmdSetViewport(mvkobjs.rdcommandbuffer[0], 0, 1, &viewport);
	vkCmdSetScissor(mvkobjs.rdcommandbuffer[0], 0, 1, &scissor);






	vkCmdEndRenderPass(mvkobjs.rdcommandbuffer[0]);


	if (vkEndCommandBuffer(mvkobjs.rdcommandbuffer[0]) != VK_SUCCESS)return false;




	VkSubmitInfo submitinfo{};
	submitinfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkPipelineStageFlags waitstage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submitinfo.pWaitDstStageMask = &waitstage;

	submitinfo.waitSemaphoreCount = 1;
	submitinfo.pWaitSemaphores = &mvkobjs.rdpresentsemaphore;

	submitinfo.signalSemaphoreCount = 1;
	submitinfo.pSignalSemaphores = &mvkobjs.rdrendersemaphore;

	submitinfo.commandBufferCount = 1;
	submitinfo.pCommandBuffers = &mvkobjs.rdcommandbuffer[0];


mvkobjs.mtx2->lock();
	if (vkQueueSubmit(mvkobjs.rdgraphicsqueue, 1, &submitinfo, mvkobjs.rdrenderfence) != VK_SUCCESS) {
		return false;
	}
mvkobjs.mtx2->unlock();

	VkPresentInfoKHR presentinfo{};
	presentinfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentinfo.waitSemaphoreCount = 1;
	presentinfo.pWaitSemaphores = &mvkobjs.rdrendersemaphore;

	presentinfo.swapchainCount = 1;
	presentinfo.pSwapchains = &mvkobjs.rdvkbswapchain.swapchain;

	presentinfo.pImageIndices = &imgidx;

mvkobjs.mtx2->lock();
	res = vkQueuePresentKHR(mvkobjs.rdpresentqueue, &presentinfo);

mvkobjs.mtx2->unlock();

	if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
		return recreateswapchain();
	}
	else {
		if (res != VK_SUCCESS) {
			return false;
		}
	}
	return true;
}

void vkrenderer::cleanloading() {
	commandbuffer::cleanup(mvkobjs, mvkobjs.rdcommandpool[1], mvkobjs.rdcommandbuffer[1]);
	commandpool::cleanup(mvkobjs, mvkobjs.rdcommandpool[1]);
}

void vkrenderer::cleanmainmenu(){
	mbackground->cleanupmodels(mvkobjs);
	mbackground->cleanupbuffers(mvkobjs);
	mbackground->cleanuplines(mvkobjs);
	mmenubg->cleanupmodels(mvkobjs);
	mmenubg->cleanuplines(mvkobjs);
}
