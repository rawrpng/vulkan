#define GLM_ENABLE_EXPERIMENTAL

#include <string>

#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>
#include <imgui/imgui_stdlib.h>

#include <memory>
#include <thread>
#include <future>
#include <mutex>


#include "ui.hpp"
#include "commandbuffer.hpp"

bool ui::init(vkobjs& renderData) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    //std::shared_ptr<ImGuiContext> x=std::make_shared<ImGuiContext>(ImGui::CreateContext());

    ImGuiIO& io = ImGui::GetIO();

    io.Fonts->AddFontFromFileTTF("resources/comicbd.ttf", 29.0f);
    io.Fonts->AddFontFromFileTTF("resources/bruce.ttf", 52.0f);

    VkDescriptorPoolSize imguiPoolSizes[] =
    {
      { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
      { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
      { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
      { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
      { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
      { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
      { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
      { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
      { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
      { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
      { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo imguiPoolInfo{};
    imguiPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    imguiPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    imguiPoolInfo.maxSets = 1000;
    imguiPoolInfo.poolSizeCount = std::size(imguiPoolSizes);
    imguiPoolInfo.pPoolSizes = imguiPoolSizes;

    if (vkCreateDescriptorPool(renderData.rdvkbdevice.device, &imguiPoolInfo, nullptr, &renderData.rdimguidescriptorpool)) {
        return false;
    }

    ImGui_ImplGlfw_InitForVulkan(renderData.rdwind, true);

    ImGui_ImplVulkan_InitInfo imguiIinitInfo{};
    imguiIinitInfo.Instance = renderData.rdvkbinstance.instance;
    imguiIinitInfo.PhysicalDevice = renderData.rdvkbphysdev.physical_device;
    imguiIinitInfo.Device = renderData.rdvkbdevice.device;
    imguiIinitInfo.Queue = renderData.rdgraphicsqueue;
    imguiIinitInfo.DescriptorPool = renderData.rdimguidescriptorpool;
    imguiIinitInfo.MinImageCount = 2;
    imguiIinitInfo.ImageCount = renderData.rdswapchainimages.size();
    imguiIinitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    imguiIinitInfo.RenderPass = renderData.rdrenderpass;
    
    //imguiIinitInfo.UseDynamicRendering = true;

    ImGui_ImplVulkan_Init(&imguiIinitInfo);

    VkCommandBuffer imguiCommandBuffer;


    if (!commandbuffer::init(renderData, renderData.rdcommandpool[1], imguiCommandBuffer)) {
        return false;
    }

    if (vkResetCommandBuffer(imguiCommandBuffer, 0) != VK_SUCCESS) {
        return false;
    }

    VkCommandBufferBeginInfo cmdBeginInfo{};
    cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(imguiCommandBuffer, &cmdBeginInfo) != VK_SUCCESS) {
        return false;
    }

    renderData.mtx2->lock();
    ImGui_ImplVulkan_CreateFontsTexture();
    renderData.mtx2->unlock();

    if (vkEndCommandBuffer(imguiCommandBuffer) != VK_SUCCESS) {
        return false;
    }


    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pWaitDstStageMask = nullptr;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = nullptr;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = nullptr;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &imguiCommandBuffer;

    VkFence imguiBufferFence;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateFence(renderData.rdvkbdevice.device, &fenceInfo, nullptr, &imguiBufferFence) != VK_SUCCESS) {
        return false;
    }

    if (vkResetFences(renderData.rdvkbdevice.device, 1, &imguiBufferFence) != VK_SUCCESS) {
        return false;
    }

    //if (vkWaitForFences(renderData.rdvkbdevice.device, 1, &renderData.rdrenderfence, VK_TRUE, INT64_MAX) != VK_SUCCESS) {
    //    return false;
    //}
   renderData.mtx2->lock();
    if (vkQueueSubmit(renderData.rdgraphicsqueue, 1, &submitInfo, imguiBufferFence) != VK_SUCCESS) {
        return false;
    }
   renderData.mtx2->unlock();

    if (vkWaitForFences(renderData.rdvkbdevice.device, 1, &imguiBufferFence, VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
        return false;
    }

    vkDestroyFence(renderData.rdvkbdevice.device, imguiBufferFence, nullptr);
    commandbuffer::cleanup(renderData, renderData.rdcommandpool[1], imguiCommandBuffer);

    ImGui::StyleColorsDark();




    //ImGui_ImplGlfw_RestoreCallbacks(renderData.rdwind);
    //ImGui_ImplGlfw_InstallCallbacks(renderData.rdwind);
    

    /* init plot vectors */
    mfpsvalues.reserve(mnumfpsvalues);
    mframetimevalues.reserve(mnumframetimevalues);
    mmodeluploadvalues.reserve(mnummodeluploadvalues);
    mmatrixgenvalues.reserve(mnummatrixgenvalues);
    mikvalues.reserve(mnumikvalues);
    mmatrixuploadvalues.reserve(mnummatrixuploadvalues);
    muigenvalues.reserve(mnumuigenvalues);
    mmuidrawvalues.reserve(mnummuidrawvalues);
    mfpsvalues.resize(mnumfpsvalues);
    mframetimevalues.resize(mnumframetimevalues);
    mmodeluploadvalues.resize(mnummodeluploadvalues);
    mmatrixgenvalues.resize(mnummatrixgenvalues);
    mikvalues.resize(mnumikvalues);
    mmatrixuploadvalues.resize(mnummatrixuploadvalues);
    muigenvalues.resize(mnumuigenvalues);
    mmuidrawvalues.resize(mnummuidrawvalues);

    return true;
}

void ui::createdbgframe(vkobjs& renderData, modelsettings& settings,netobjs& nobjs) {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        ImGuiWindowFlags imguiWindowFlags = 0;

        ImGui::SetNextWindowBgAlpha(0.8f);

        ImGui::Begin("debug", nullptr, imguiWindowFlags);

        static float newFps = 0.0f;
        /* avoid inf values (division by zero) */
        if (renderData.frametime > 0.0) {
            newFps = 1.0f / renderData.frametime * 1000.f;
        }
        /* make an averge value to avoid jumps */
        //mfps = (mavgalpha * mfps) + (1.0f - mavgalpha) * newFps;
        mfps = newFps;

        /* clamp manual input on all sliders to min/max */
        //ImGuiSliderFlags flags = ImGuiSliderFlags_ClampOnInput;
        ImGuiSliderFlags flags = ImGuiSliderFlags_None;

        static double updateTime = 0.0;

        /* avoid double compares */
        if (updateTime < 0.000001) {
            updateTime = ImGui::GetTime();
        }

        static int fpsOffset = 0;
        static int frameTimeOffset = 0;
        static int modelUploadOffset = 0;
        static int matrixGenOffset = 0;
        static int ikOffset = 0;
        static int matrixUploadOffset = 0;
        static int uiGenOffset = 0;
        static int uiDrawOffset = 0;

        if (updateTime < ImGui::GetTime()) {
            mfpsvalues.at(fpsOffset) = mfps;
            fpsOffset = ++fpsOffset % mnumfpsvalues;

            mframetimevalues.at(frameTimeOffset) = renderData.frametime;
            frameTimeOffset = ++frameTimeOffset % mnumframetimevalues;

            mmodeluploadvalues.at(modelUploadOffset) = renderData.uploadubossbotime;
            modelUploadOffset = ++modelUploadOffset % mnummodeluploadvalues;

            mmatrixgenvalues.at(matrixGenOffset) = renderData.updateanimtime;
            matrixGenOffset = ++matrixGenOffset % mnummatrixgenvalues;

            mikvalues.at(ikOffset) = renderData.updatemattime;
            ikOffset = ++ikOffset % mnumikvalues;

            mmatrixuploadvalues.at(matrixUploadOffset) = renderData.iksolvetime;
            matrixUploadOffset = ++matrixUploadOffset % mnummatrixuploadvalues;

            muigenvalues.at(uiGenOffset) = renderData.rduigeneratetime;
            uiGenOffset = ++uiGenOffset % mnumuigenvalues;

            mmuidrawvalues.at(uiDrawOffset) = renderData.rduidrawtime;
            uiDrawOffset = ++uiDrawOffset % mnummuidrawvalues;

            updateTime += 1.0 / 30.0;
        }

        ImGui::BeginGroup();
        ImGui::Text("FPS:");
        ImGui::SameLine();
        ImGui::Text("%s", std::to_string(1.0f/renderData.frametime).c_str());
        ImGui::EndGroup();

        if (ImGui::CollapsingHeader("General")) {
            ImGui::Text("Triangles:");
            ImGui::SameLine();
            ImGui::Text("%s", std::to_string(renderData.rdtricount + renderData.rdgltftricount).c_str());

            std::string windowDims = std::to_string(renderData.rdwidth) + "x" + std::to_string(renderData.rdheight);
            ImGui::Text("Window Dimensions:");
            ImGui::SameLine();
            ImGui::Text("%s", windowDims.c_str());

            std::string imgWindowPos = std::to_string(static_cast<int>(ImGui::GetWindowPos().x)) + "/" + std::to_string(static_cast<int>(ImGui::GetWindowPos().y));
            ImGui::Text("ImGui Window Position:");
            ImGui::SameLine();
            ImGui::Text("%s", imgWindowPos.c_str());
        }

        if (ImGui::CollapsingHeader("Timers")) {
            ImGui::BeginGroup();
            ImGui::Text("Frame Time:");
            ImGui::SameLine();
            ImGui::Text("%s", std::to_string(renderData.frametime).c_str());
            ImGui::SameLine();
            ImGui::Text("ms");
            ImGui::EndGroup();

            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                float averageFrameTime = 0.0f;
                for (const auto value : mframetimevalues) {
                    averageFrameTime += value;
                }
                averageFrameTime /= static_cast<float>(mnummatrixgenvalues);
                std::string frameTimeOverlay = "now:     " + std::to_string(renderData.frametime)
                    + " ms\n30s avg: " + std::to_string(averageFrameTime) + " ms";
                ImGui::Text("Frame Time       ");
                ImGui::SameLine();
                ImGui::PlotLines("##FrameTime", mframetimevalues.data(), mframetimevalues.size(), frameTimeOffset,
                    frameTimeOverlay.c_str(), 0.0f, FLT_MAX, ImVec2(0, 80));
                ImGui::EndTooltip();
            }

            ImGui::BeginGroup();
            ImGui::Text("Upload Time:");
            ImGui::SameLine();
            ImGui::Text("%s", std::to_string(renderData.uploadubossbotime).c_str());
            ImGui::SameLine();
            ImGui::Text("ms");
            ImGui::EndGroup();

            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                float averageModelUpload = 0.0f;
                for (const auto value : mmodeluploadvalues) {
                    averageModelUpload += value;
                }
                averageModelUpload /= static_cast<float>(mnummodeluploadvalues);
                std::string modelUploadOverlay = "now:     " + std::to_string(renderData.uploadubossbotime)
                    + " ms\n30s avg: " + std::to_string(averageModelUpload) + " ms";
                ImGui::Text("VBO Upload");
                ImGui::SameLine();
                ImGui::PlotLines("##ModelUploadTimes", mmodeluploadvalues.data(), mmodeluploadvalues.size(), modelUploadOffset,
                    modelUploadOverlay.c_str(), 0.0f, FLT_MAX, ImVec2(0, 80));
                ImGui::EndTooltip();
            }

            ImGui::BeginGroup();
            ImGui::Text("Update Animations Time:");
            ImGui::SameLine();
            ImGui::Text("%s", std::to_string(renderData.updateanimtime).c_str());
            ImGui::SameLine();
            ImGui::Text("ms");
            ImGui::EndGroup();

            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                float averageMatGen = 0.0f;
                for (const auto value : mmatrixgenvalues) {
                    averageMatGen += value;
                }
                averageMatGen /= static_cast<float>(mnummatrixgenvalues);
                std::string matrixGenOverlay = "now:     " + std::to_string(renderData.updateanimtime)
                    + " ms\n30s avg: " + std::to_string(averageMatGen) + " ms";
                ImGui::Text("Matrix Generation");
                ImGui::SameLine();
                ImGui::PlotLines("##MatrixGenTimes", mmatrixgenvalues.data(), mmatrixgenvalues.size(), matrixGenOffset,
                    matrixGenOverlay.c_str(), 0.0f, FLT_MAX, ImVec2(0, 80));
                ImGui::EndTooltip();
            }

            ImGui::BeginGroup();
            ImGui::Text("Update TRS Mats :");
            ImGui::SameLine();
            ImGui::Text("%s", std::to_string(renderData.updatemattime).c_str());
            ImGui::SameLine();
            ImGui::Text("ms");
            ImGui::EndGroup();

            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                float averageIKTime = 0.0f;
                for (const auto value : mikvalues) {
                    averageIKTime += value;
                }
                averageIKTime /= static_cast<float>(mnumikvalues);
                std::string ikOverlay = "now:     " + std::to_string(renderData.updatemattime)
                    + " ms\n30s avg: " + std::to_string(averageIKTime) + " ms";
                ImGui::Text("(IK Generation)");
                ImGui::SameLine();
                ImGui::PlotLines("##IKTimes", mikvalues.data(), mikvalues.size(), ikOffset,
                    ikOverlay.c_str(), 0.0f, FLT_MAX, ImVec2(0, 80));
                ImGui::EndTooltip();
            }

            ImGui::BeginGroup();
            ImGui::Text("Matrix Upload Time:");
            ImGui::SameLine();
            ImGui::Text("%s", std::to_string(renderData.iksolvetime).c_str());
            ImGui::SameLine();
            ImGui::Text("ms");
            ImGui::EndGroup();

            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                float averageMatrixUpload = 0.0f;
                for (const auto value : mmatrixuploadvalues) {
                    averageMatrixUpload += value;
                }
                averageMatrixUpload /= static_cast<float>(mnummatrixuploadvalues);
                std::string matrixUploadOverlay = "now:     " + std::to_string(renderData.uploadubossbotime)
                    + " ms\n30s avg: " + std::to_string(averageMatrixUpload) + " ms";
                ImGui::Text("UBO Upload");
                ImGui::SameLine();
                ImGui::PlotLines("##MatrixUploadTimes", mmatrixuploadvalues.data(), mmatrixuploadvalues.size(), matrixUploadOffset,
                    matrixUploadOverlay.c_str(), 0.0f, FLT_MAX, ImVec2(0, 80));
                ImGui::EndTooltip();
            }

            ImGui::BeginGroup();
            ImGui::Text("UI Generation Time:");
            ImGui::SameLine();
            ImGui::Text("%s", std::to_string(renderData.rduigeneratetime).c_str());
            ImGui::SameLine();
            ImGui::Text("ms");
            ImGui::EndGroup();

            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                float averageUiGen = 0.0f;
                for (const auto value : muigenvalues) {
                    averageUiGen += value;
                }
                averageUiGen /= static_cast<float>(mnumuigenvalues);
                std::string uiGenOverlay = "now:     " + std::to_string(renderData.rduigeneratetime)
                    + " ms\n30s avg: " + std::to_string(averageUiGen) + " ms";
                ImGui::Text("UI Generation");
                ImGui::SameLine();
                ImGui::PlotLines("##ModelUpload", muigenvalues.data(), muigenvalues.size(), uiGenOffset,
                    uiGenOverlay.c_str(), 0.0f, FLT_MAX, ImVec2(0, 80));
                ImGui::EndTooltip();
            }

            ImGui::BeginGroup();
            ImGui::Text("UI Draw Time:");
            ImGui::SameLine();
            ImGui::Text("%s", std::to_string(renderData.rduidrawtime).c_str());
            ImGui::SameLine();
            ImGui::Text("ms");
            ImGui::EndGroup();

            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                float averageUiDraw = 0.0f;
                for (const auto value : mmuidrawvalues) {
                    averageUiDraw += value;
                }
                averageUiDraw /= static_cast<float>(mnummuidrawvalues);
                std::string uiDrawOverlay = "now:     " + std::to_string(renderData.rduidrawtime)
                    + " ms\n30s avg: " + std::to_string(averageUiDraw) + " ms";
                ImGui::Text("UI Draw");
                ImGui::SameLine();
                ImGui::PlotLines("##UIDrawTimes", mmuidrawvalues.data(), mmuidrawvalues.size(), uiGenOffset,
                    uiDrawOverlay.c_str(), 0.0f, FLT_MAX, ImVec2(0, 80));
                ImGui::EndTooltip();
            }
        }

        if (ImGui::CollapsingHeader("Camera")) {
            ImGui::Text("Camera Position:");
            ImGui::SameLine();
            ImGui::Text("%s", glm::to_string(renderData.rdcamwpos).c_str());

            ImGui::Text("View Azimuth:");
            ImGui::SameLine();
            ImGui::Text("%s", std::to_string(renderData.rdazimuth).c_str());

            ImGui::Text("View Elevation:");
            ImGui::SameLine();
            ImGui::Text("%s", std::to_string(renderData.rdelevation).c_str());

            ImGui::Text("Field of View");
            ImGui::SameLine();
            ImGui::Text("%s", std::to_string(renderData.rdfov).c_str());
            //ImGui::SliderFloat("##FOV", &renderData.rdfov, 40.0f, 150.0f, "%f", flags);
        }

        if (ImGui::CollapsingHeader("instances")) {
            ImGui::Text("count  : %d", renderData.rdnumberofinstances);

            ImGui::Text("selected :");
            ImGui::SameLine();
            ImGui::PushButtonRepeat(true);
            if (ImGui::ArrowButton("##LEFT", ImGuiDir_Left) &&
                renderData.rdcurrentselectedinstance > 0) {
                renderData.rdcurrentselectedinstance--;
            }
            ImGui::SameLine();
            ImGui::PushItemWidth(30);
            ImGui::DragInt("##SELINST", &renderData.rdcurrentselectedinstance, 1, 0,
                renderData.rdnumberofinstances - 1, "%3d", flags);
            ImGui::PopItemWidth();
            ImGui::SameLine();
            if (ImGui::ArrowButton("##RIGHT", ImGuiDir_Right) &&
                renderData.rdcurrentselectedinstance < (renderData.rdnumberofinstances - 1)) {
                renderData.rdcurrentselectedinstance++;
            }
            ImGui::PopButtonRepeat();

            ImGui::Text("position (x,z)  :");
            ImGui::SameLine();
            ImGui::SliderFloat3("##WORLDPOS", glm::value_ptr(settings.msworldpos),
                -75.0f, 75.0f, "%.1f", flags);

            ImGui::Text("rotation - yaw   :");
            ImGui::SameLine();
            ImGui::SliderFloat("##WORLDROT", &settings.msworldrot.y,
                -180.0f, 180.0f, "%.0f", flags);
        }

        if (ImGui::CollapsingHeader("model")) {
            ImGui::Checkbox("draw", &settings.msdrawmodel);
            //ImGui::Checkbox("Draw Skeleton", &settings.msdrawskeleton);

            ImGui::Text("skinning:");
            ImGui::SameLine();
            if (ImGui::RadioButton("linear",
                settings.mvertexskinningmode == skinningmode::linear)) {
                settings.mvertexskinningmode = skinningmode::linear;
            }
            //ImGui::SameLine();
            //if (ImGui::RadioButton("dual quats",
            //    settings.mvertexskinningmode == skinningmode::dualquat)) {
            //    settings.mvertexskinningmode = skinningmode::dualquat;
            //}
        }

        if (ImGui::CollapsingHeader("animation")) {
            ImGui::Checkbox("play", &settings.msplayanimation);

            if (!settings.msplayanimation) {
                ImGui::BeginDisabled();
            }

            ImGui::Text("playback direction:");
            ImGui::SameLine();
            if (ImGui::RadioButton("forward",
                settings.msanimationplaydirection == replaydirection::forward)) {
                settings.msanimationplaydirection = replaydirection::forward;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("backward",
                settings.msanimationplaydirection == replaydirection::backward)) {
                settings.msanimationplaydirection = replaydirection::backward;
            }

            if (!settings.msplayanimation) {
                ImGui::EndDisabled();
            }

            ImGui::Text("clip   ");
            ImGui::SameLine();
            if (ImGui::BeginCombo("##ClipCombo",
                settings.msclipnames.at(settings.msanimclip).c_str())) {
                for (int i = 0; i < settings.msclipnames.size(); ++i) {
                    const bool isSelected = (settings.msanimclip == i);
                    if (ImGui::Selectable(settings.msclipnames.at(i).c_str(), isSelected)) {
                        settings.msanimclip = i;
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            if (settings.msplayanimation) {
                ImGui::Text("speed  ");
                ImGui::SameLine();
                ImGui::SliderFloat("##ClipSpeed", &settings.msanimspeed, 0.0f, 2.0f, "%.3f", flags);
            }
            else {
                ImGui::Text("timepos");
                ImGui::SameLine();
                ImGui::SliderFloat("##ClipPos", &settings.msanimtimepos, 0.0f,
                    settings.msanimendtime, "%.3f", flags);
            }
        }

        if (ImGui::CollapsingHeader("blending")) {
            ImGui::Text("blend type:");
            ImGui::SameLine();
            if (ImGui::RadioButton("Fade In/Out",
                settings.msblendingmode == blendmode::fadeinout)) {
                settings.msblendingmode = blendmode::fadeinout;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("crossfading",
                settings.msblendingmode == blendmode::crossfade)) {
                settings.msblendingmode = blendmode::crossfade;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("additive",
                settings.msblendingmode == blendmode::additive)) {
                settings.msblendingmode = blendmode::additive;
            }

            if (settings.msblendingmode == blendmode::fadeinout) {
                ImGui::Text("factor");
                ImGui::SameLine();
                ImGui::SliderFloat("##BlendFactor", &settings.msanimblendfactor, 0.0f, 1.0f, "%.3f",
                    flags);
            }

            if (settings.msblendingmode == blendmode::crossfade ||
                settings.msblendingmode == blendmode::additive) {
                ImGui::Text("Dest Clip   ");
                ImGui::SameLine();
                if (ImGui::BeginCombo("##DestClipCombo",
                    settings.msclipnames.at(settings.mscrossblenddestanimclip).c_str())) {
                    for (int i = 0; i < settings.msclipnames.size(); ++i) {
                        const bool isSelected = (settings.mscrossblenddestanimclip == i);
                        if (ImGui::Selectable(settings.msclipnames.at(i).c_str(), isSelected)) {
                            settings.mscrossblenddestanimclip = i;
                        }
                        if (isSelected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }

                ImGui::Text("Cross Blend ");
                ImGui::SameLine();
                ImGui::SliderFloat("##CrossBlendFactor", &settings.msanimcrossblendfactor, 0.0f, 1.0f,
                    "%.3f", flags);
            }

            if (settings.msblendingmode == blendmode::additive) {
                ImGui::Text("Split Node  ");
                ImGui::SameLine();
                if (ImGui::BeginCombo("##SplitNodeCombo",
                    settings.msskelnodenames.at(settings.msskelsplitnode).c_str())) {
                    for (int i = 0; i < settings.msskelnodenames.size(); ++i) {
                        if (settings.msskelnodenames.at(i).compare("(invalid)") != 0) {
                            const bool isSelected = (settings.msskelsplitnode == i);
                            if (ImGui::Selectable(settings.msskelnodenames.at(i).c_str(), isSelected)) {
                                settings.msskelsplitnode = i;
                            }
                            if (isSelected) {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                    }
                    ImGui::EndCombo();
                }
            }
        }

        if (ImGui::CollapsingHeader("inverse kinematics")) {
            if (ImGui::RadioButton("Off",
                settings.msikmode == ikmode::off)) {
                settings.msikmode = ikmode::off;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("CCD",
                settings.msikmode == ikmode::ccd)) {
                settings.msikmode = ikmode::ccd;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("FABRIK",
                settings.msikmode == ikmode::fabrik)) {
                settings.msikmode = ikmode::fabrik;
            }

            if (settings.msikmode == ikmode::ccd ||
                settings.msikmode == ikmode::fabrik) {
                ImGui::Text("IK Iterations  :");
                ImGui::SameLine();
                ImGui::SliderInt("##IKITER", &settings.msikiterations, 0, 15, "%d", flags);

                ImGui::Text("Target Position:");
                ImGui::SameLine();
                ImGui::SliderFloat3("##IKTargetPOS", glm::value_ptr(settings.msiktargetpos), -10.0f,
                    10.0f, "%.3f", flags);
                ImGui::Text("Effector Node  :");
                ImGui::SameLine();
                if (ImGui::BeginCombo("##EffectorNodeCombo",
                    settings.msskelnodenames.at(settings.msikeffectornode).c_str())) {
                    for (int i = 0; i < settings.msskelnodenames.size(); ++i) {
                        if (settings.msskelnodenames.at(i).compare("(invalid)") != 0) {
                            const bool isSelected = (settings.msikeffectornode == i);
                            if (ImGui::Selectable(settings.msskelnodenames.at(i).c_str(), isSelected)) {
                                settings.msikeffectornode = i;
                            }

                            if (isSelected) {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                    }
                    ImGui::EndCombo();
                }
                ImGui::Text("IK Root Node   :");
                ImGui::SameLine();
                if (ImGui::BeginCombo("##RootNodeCombo",
                    settings.msskelnodenames.at(settings.msikrootnode).c_str())) {
                    for (int i = 0; i < settings.msskelnodenames.size(); ++i) {
                        if (settings.msskelnodenames.at(i).compare("(invalid)") != 0) {
                            const bool isSelected = (settings.msikrootnode == i);
                            if (ImGui::Selectable(settings.msskelnodenames.at(i).c_str(), isSelected)) {
                                settings.msikrootnode = i;
                            }

                            if (isSelected) {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                    }
                    ImGui::EndCombo();
                }
            }
        }

        ImGui::End();
    }


    //chatbox
    if(!nobjs.offlineplay){
        ImGuiWindowFlags imguiWindowFlags = 0;
        imguiWindowFlags |= ImGuiWindowFlags_NoBackground;
        imguiWindowFlags |= ImGuiWindowFlags_NoResize;
        imguiWindowFlags |= ImGuiWindowFlags_NoMove;
        imguiWindowFlags |= ImGuiWindowFlags_NoSavedSettings;
        imguiWindowFlags |= ImGuiWindowFlags_NoCollapse;
        imguiWindowFlags |= ImGuiWindowFlags_NoTitleBar;
        imguiWindowFlags |= ImGuiWindowFlags_AlwaysAutoResize;

        ImVec2 wsize{ ImGui::GetMainViewport()->Size };
        ImVec2 wpos{ ImGui::GetMainViewport()->Pos };


        ImGui::SetNextWindowPos({ wpos.x + wsize.x * 0.0f , wpos.y + wsize.y * 1.0f - 140.0f }, 1, { 0.0f,1.0f });
        ImGui::SetNextWindowSizeConstraints({ 600.0f,0.0f }, { 600.0f,400.0f });

        ImGui::Begin("chat", nullptr, imguiWindowFlags);



        for (const std::string& i : chattxts) {
            //std::cout << i << std::endl;
            ImGui::Text(i.c_str());
        }


        ImGui::End();
        ImGui::SetNextWindowPos({ wpos.x + wsize.x * 0.0f , wpos.y + wsize.y * 1.0f -100.0f }, 1, { 0.0f,1.0f });
        ImGui::SetNextWindowSizeConstraints({ 600.0f,0.0f }, { 600.0f,400.0f });

        ImGui::Begin("chatinput", nullptr, imguiWindowFlags);
        ImGui::InputText(" ", &inputxt);
        ImGui::SameLine();
        if ((ImGui::Button("send >")||chatfocus)) {
            if (!inputxt.empty()) {
                if (nobjs.rdserverclient) {
                    std::string i2 = "SERVER : " + inputxt;
                    for (const auto i : nobjs.nserver->GetConnectedClients()) {
                        nobjs.nserver->SendStringToClient(i.first,0, i2);
                    }
                    chattxts.push_back(i2);
                } else {
                    const std::string i2 = inputxt;
                    nobjs.nclient->SendString(i2);
                }
            }
            inputxt.clear();
            chatfocus = false;
        }


        ImGui::End(); 
    }
    {
        ImGuiWindowFlags imguiWindowFlags = 0;
        imguiWindowFlags |= ImGuiWindowFlags_NoBackground;
        imguiWindowFlags |= ImGuiWindowFlags_NoResize;
        imguiWindowFlags |= ImGuiWindowFlags_NoMove;
        imguiWindowFlags |= ImGuiWindowFlags_NoSavedSettings;
        imguiWindowFlags |= ImGuiWindowFlags_NoCollapse;
        imguiWindowFlags |= ImGuiWindowFlags_NoTitleBar;
        imguiWindowFlags |= ImGuiWindowFlags_AlwaysAutoResize;

        ImVec2 wsize{ ImGui::GetMainViewport()->Size };
        ImVec2 wpos{ ImGui::GetMainViewport()->Pos };

        ImGui::SetNextWindowPos({ wpos.x + wsize.x * 1.0f, wpos.y + wsize.y * 0.0f }, 1, { 1.0f,0.0f });

        ImGui::Begin("Scores", nullptr, imguiWindowFlags);
        if (!nobjs.offlineplay) {
            for (const auto& i : gamestate::astages) {
                ImGui::Text(" ");
                ImGui::SameLine();
                ImGui::Text(std::to_string(i.first).c_str());
                ImGui::Text(" is at ");
                ImGui::SameLine();
                ImGui::Text(" wave :");
                ImGui::SameLine();
                ImGui::Text(std::to_string(i.second).c_str());
            }
        } else {
            ImGui::Text(" wave :");
            ImGui::SameLine();
            ImGui::Text(std::to_string(playerwave[0]).c_str());
        }


        ImGui::End();
    }
    {
        //minigame
        ImGuiWindowFlags imguiWindowFlags = 0;
        imguiWindowFlags |= ImGuiWindowFlags_NoBackground;
        imguiWindowFlags |= ImGuiWindowFlags_NoResize;
        imguiWindowFlags |= ImGuiWindowFlags_NoMove;
        imguiWindowFlags |= ImGuiWindowFlags_NoSavedSettings;
        imguiWindowFlags |= ImGuiWindowFlags_NoCollapse;
        imguiWindowFlags |= ImGuiWindowFlags_NoTitleBar;
        imguiWindowFlags |= ImGuiWindowFlags_AlwaysAutoResize;
        ImVec2 wsize{ ImGui::GetMainViewport()->Size };
        ImVec2 wpos{ ImGui::GetMainViewport()->Pos };

        ImGui::SetNextWindowPos({ wpos.x + wsize.x * 1.0f, wpos.y + wsize.y * 1.0f}, 1, { 1.0f,1.0f });

        ImGui::Begin("controls", nullptr, imguiWindowFlags);
        if (*renderData.decaying)
            ImGui::PushStyleColor(ImGuiCol_Button, { 0.2f,0.2f,0.2f,1.0f });
        else
            ImGui::PushStyleColor(ImGuiCol_Button, { 0.8f,0.2f,0.8f,1.0f });
        if (ImGui::Button("Drop Vessel")) {
            *renderData.decaying = true;
        };
        ImGui::PopStyleColor();
        ImGui::End();
    }


}

bool ui::createmainmenuframe(vkobjs& mvkobjs,netobjs& nobjs) {

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGuiIO& io = ImGui::GetIO();


    ImGuiWindowFlags imguiWindowFlags = 0;
    imguiWindowFlags |= ImGuiWindowFlags_MenuBar;
    imguiWindowFlags |= ImGuiWindowFlags_NoBackground;
    imguiWindowFlags |= ImGuiWindowFlags_NoResize;
    imguiWindowFlags |= ImGuiWindowFlags_NoMove;
    imguiWindowFlags |= ImGuiWindowFlags_NoSavedSettings;
    imguiWindowFlags |= ImGuiWindowFlags_NoCollapse;
    imguiWindowFlags |= ImGuiWindowFlags_NoTitleBar;
    imguiWindowFlags |= ImGuiWindowFlags_AlwaysAutoResize;

    


    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), 1, { 0.5f,0.5f });


    ImGui::Begin("Menu", nullptr, imguiWindowFlags);



    ImGui::PushStyleColor(ImGuiCol_Button, { 0.0f,0.0f,0.0f,1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.4f,0.4f,0.4f,1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.9f,0.9f,0.9f,1.0f });


    ImGui::PushFont(io.Fonts->Fonts[0]);


    ImGui::PushStyleColor(ImGuiCol_MenuBarBg, { 0.0f,0.0f,0.0f,0.2f });
    ImGui::PushStyleColor(ImGuiCol_PopupBg, { 0.0f,0.0f,0.0f,0.2f });
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, { 1.0f,1.0f,1.0f,0.4f });
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, { 1.0f,1.0f,1.0f,1.0f });

    //ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, { 400,600 });

    

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("settings")) {
            if (mvkobjs.rdfullscreen) {
                if (ImGui::MenuItem("windowed", "F4")) {
                    glfwSetWindowMonitor(mvkobjs.rdwind, nullptr, 100, 200, 900, 600, GLFW_DONT_CARE);
                    mvkobjs.rdfullscreen = !mvkobjs.rdfullscreen;
                }
            }
            else {
                if (ImGui::MenuItem("fullscreen", "F4")) {
                    glfwSetWindowMonitor(mvkobjs.rdwind, mvkobjs.rdmonitor, 0, 0, mvkobjs.rdmode->width, mvkobjs.rdmode->height, mvkobjs.rdmode->refreshRate);
                    mvkobjs.rdfullscreen = !mvkobjs.rdfullscreen;
                }
            }
            if (ImGui::MenuItem("network settings", "F3")) {
                setnetwork = !setnetwork;
                //nobjs.rdserverclient = !nobjs.rdserverclient;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    if (setnetwork) {
        {
            ImGuiWindowFlags imguiWindowFlags = 0;
            //imguiWindowFlags |= ImGuiWindowFlags_NoBackground;
            imguiWindowFlags |= ImGuiWindowFlags_NoResize;
            //imguiWindowFlags |= ImGuiWindowFlags_NoMove;
            imguiWindowFlags |= ImGuiWindowFlags_NoSavedSettings;
            imguiWindowFlags |= ImGuiWindowFlags_NoCollapse;
            //imguiWindowFlags |= ImGuiWindowFlags_NoTitleBar;
            imguiWindowFlags |= ImGuiWindowFlags_AlwaysAutoResize;

            ImVec2 wsize{ ImGui::GetMainViewport()->Size };
            ImVec2 wpos{ ImGui::GetMainViewport()->Pos };

            ImGui::SetNextWindowScroll({ 1.0f,1.0f });

            ImGui::SetNextWindowPos({ wpos.x + wsize.x * 0.0f , wpos.y + wsize.y * 1.0f - 40.0f }, 1, { 0.0f,1.0f });
            ImGui::SetNextWindowSizeConstraints({ 600.0f,300.0f }, { 600.0f,300.0f });

            ImGui::Begin("network settings", nullptr, imguiWindowFlags);


            if (ImGui::RadioButton("play offline", &selectednetwork, 0)) {
                offline = true;
                hosting = false;
                connectingtohost = false;
                nobjs.offlineplay = true;
                nobjs.rdserverclient = false;
            }
            if (ImGui::RadioButton("host server", &selectednetwork, 1)) {
                offline = false;
                hosting = true;
                connectingtohost = false;
                nobjs.offlineplay = false;
                nobjs.rdserverclient = true;
            }
            if (hosting) {
                ImGui::Text("port :");
                ImGui::SameLine();
                ImGui::InputInt(" ", &nobjs.port);
            }
            if (ImGui::RadioButton("connect to a host", &selectednetwork, 2)) {
                offline = false;
                hosting = false;
                connectingtohost = true;
                nobjs.offlineplay = false;
                nobjs.rdserverclient = false;
            }
            if (connectingtohost) {

                ImGui::Text("please format address_of_host:port_of_host");
                ImGui::Text("and remember to forward the port!");
                ImGui::Text("e.g 127.0.0.1:21122");
                ImGui::Text("host address :");
                ImGui::SameLine();
                ImGui::InputText(" ", &nobjs.serveraddress);
            }

            ImGui::End();
        }
    }









    ImGui::PopFont();

    ImGui::PushFont(io.Fonts->Fonts[1]);


    ImGui::BeginGroup();
    //ImGui::ImageButton(io.Fonts->TexID,{400,200});

    bool p = ImGui::Button("START", { 400,200 });
    ImGui::PushFont(io.Fonts->Fonts[0]);
    if (ImGui::IsItemHovered())ImGui::SetTooltip("loading models might take a while!");
    ImGui::PopFont();
    if (ImGui::Button("EXIT", { 400,120 }))glfwSetWindowShouldClose(mvkobjs.rdwind, true);
    if (ImGui::IsItemHovered())ImGui::SetTooltip(":(");
    ImGui::PopStyleColor(7);
    ImGui::PopStyleVar(0);
    ImGui::EndGroup();

    ImGui::PopFont();

    ImGui::End();

    return !p;
}
bool ui::createloadingscreen(vkobjs& mvkobjs) {

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGuiIO& io = ImGui::GetIO();


    ImGuiWindowFlags imguiWindowFlags = 0;
    imguiWindowFlags |= ImGuiWindowFlags_NoBackground;
    imguiWindowFlags |= ImGuiWindowFlags_NoResize;
    imguiWindowFlags |= ImGuiWindowFlags_NoMove;
    imguiWindowFlags |= ImGuiWindowFlags_NoSavedSettings;
    imguiWindowFlags |= ImGuiWindowFlags_NoCollapse;
    imguiWindowFlags |= ImGuiWindowFlags_NoTitleBar;


    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), 1, { 0.5f,0.5f });


    ImGui::Begin("loading", nullptr, imguiWindowFlags);

    //ImGui::ProgressBar(static_cast<float>(std::rand()%100)/100.0f, {600,100});
    ImGui::ProgressBar(mvkobjs.loadingprog, {600,100});

    ImGui::End();

    return true;
}

bool ui::createpausebuttons(vkobjs& mvkobjs){

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGuiIO& io = ImGui::GetIO();


    ImGuiWindowFlags imguiWindowFlags = 0;
    imguiWindowFlags |= ImGuiWindowFlags_MenuBar;
    imguiWindowFlags |= ImGuiWindowFlags_NoBackground;
    imguiWindowFlags |= ImGuiWindowFlags_NoResize;
    imguiWindowFlags |= ImGuiWindowFlags_NoMove;
    imguiWindowFlags |= ImGuiWindowFlags_NoSavedSettings;
    imguiWindowFlags |= ImGuiWindowFlags_NoCollapse;
    imguiWindowFlags |= ImGuiWindowFlags_NoTitleBar;
    imguiWindowFlags |= ImGuiWindowFlags_AlwaysAutoResize;




    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), 1, { 0.5f,0.5f });


    ImGui::Begin("pause", nullptr, imguiWindowFlags);



    ImGui::PushStyleColor(ImGuiCol_Button, { 0.0f,0.0f,0.0f,1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.4f,0.4f,0.4f,1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.9f,0.9f,0.9f,1.0f });


    ImGui::PushFont(io.Fonts->Fonts[0]);


    ImGui::PushStyleColor(ImGuiCol_MenuBarBg, { 0.0f,0.0f,0.0f,0.2f });
    ImGui::PushStyleColor(ImGuiCol_PopupBg, { 0.0f,0.0f,0.0f,0.2f });
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, { 1.0f,1.0f,1.0f,0.4f });
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, { 1.0f,1.0f,1.0f,1.0f });

    //ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, { 400,600 });



    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("settings")) {
            if (mvkobjs.rdfullscreen) {
                if (ImGui::MenuItem("windowed", "F4")) {
                    glfwSetWindowMonitor(mvkobjs.rdwind, nullptr, 100, 200, 900, 600, GLFW_DONT_CARE);
                    mvkobjs.rdfullscreen = !mvkobjs.rdfullscreen;
                }
            } else {
                if (ImGui::MenuItem("fullscreen", "F4")) {
                    glfwSetWindowMonitor(mvkobjs.rdwind, mvkobjs.rdmonitor, 0, 0, mvkobjs.rdmode->width, mvkobjs.rdmode->height, mvkobjs.rdmode->refreshRate);
                    mvkobjs.rdfullscreen = !mvkobjs.rdfullscreen;
                }
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::PopFont();

    ImGui::PushFont(io.Fonts->Fonts[1]);


    ImGui::BeginGroup();

    bool p = ImGui::Button("continue", { 400,200 });
    ImGui::PushFont(io.Fonts->Fonts[0]);
    ImGui::PopFont();
    if (ImGui::Button("EXIT", { 400,120 }))glfwSetWindowShouldClose(mvkobjs.rdwind, true);
    ImGui::PopStyleColor(7);
    ImGui::PopStyleVar(0);
    ImGui::EndGroup();

    ImGui::PopFont();

    ImGui::End();

    ImGui::EndFrame();

    return p;
}

void ui::addchat(std::string s){
    //ImGui::SetNextWindowScroll({10000.0f,10000.0f});
    chattxts.push_back(s);
}

void ui::render(vkobjs& renderData,VkCommandBuffer& cbuffer) {
    ImGui::Render();

    //renderData.mtx2.lock();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cbuffer);
    //renderData.mtx2.unlock();
}

void ui::cleanup(vkobjs& renderData) {
    ImGui_ImplVulkan_Shutdown();
    vkDestroyDescriptorPool(renderData.rdvkbdevice.device, renderData.rdimguidescriptorpool, nullptr);
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ui::backspace(){
    if(!inputxt.empty())
    inputxt.pop_back();
}
