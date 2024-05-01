#include "VulkanCommands.hpp"
#include "VulkanDevice.hpp"
#include "VulkanUtils.hpp"

namespace Fyrion
{
    VulkanCommands::VulkanCommands(VulkanDevice& vulkanDevice) : vulkanDevice(vulkanDevice)
    {
        VkCommandPoolCreateInfo commandPoolInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        commandPoolInfo.queueFamilyIndex = vulkanDevice.graphicsFamily;
        vkCreateCommandPool(vulkanDevice.device, &commandPoolInfo, nullptr, &commandPool);

        VkCommandBufferAllocateInfo tempAllocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        tempAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        tempAllocInfo.commandPool = commandPool;
        tempAllocInfo.commandBufferCount = 1;

        vkAllocateCommandBuffers(vulkanDevice.device, &tempAllocInfo, &commandBuffer);
    }

    void VulkanCommands::Begin()
    {
        VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        vkBeginCommandBuffer(commandBuffer, &beginInfo);
    }

    void VulkanCommands::End()
    {
        vkEndCommandBuffer(commandBuffer);
    }

    void VulkanCommands::BeginRenderPass(const BeginRenderPassInfo& beginRenderPassInfo)
    {
        VulkanRenderPass* vulkanRenderPass = static_cast<VulkanRenderPass*>(beginRenderPassInfo.renderPass.handler);

        VkRenderPassBeginInfo renderPassBeginInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        renderPassBeginInfo.renderPass = vulkanRenderPass->renderPass;
        renderPassBeginInfo.framebuffer = vulkanRenderPass->framebuffer;
        renderPassBeginInfo.renderArea.offset = {0, 0};
        renderPassBeginInfo.renderArea.extent = {vulkanRenderPass->extent.width, vulkanRenderPass->extent.height};

        for (int i = 0; i < vulkanRenderPass->clearValues.Size(); ++i)
        {
            VkClearValue& clearValue = vulkanRenderPass->clearValues[i];
            if (beginRenderPassInfo.clearValues.Size() > i)
            {
                Vec4 color = beginRenderPassInfo.clearValues[i];
                clearValue.color = {color.x, color.y, color.z, color.w};
            }
            else
            {
                clearValue.depthStencil = {.depth = beginRenderPassInfo.depthStencil.depth, .stencil = beginRenderPassInfo.depthStencil.stencil};
            }
        }

        renderPassBeginInfo.clearValueCount = vulkanRenderPass->clearValues.Size();
        renderPassBeginInfo.pClearValues = vulkanRenderPass->clearValues.Data();
        vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    void VulkanCommands::EndRenderPass()
    {
        vkCmdEndRenderPass(commandBuffer);
    }

    void VulkanCommands::SetViewport(const ViewportInfo& viewportInfo)
    {
        VkViewport vkViewport;
        vkViewport.x = viewportInfo.x;
        vkViewport.y = viewportInfo.y;
        vkViewport.width = viewportInfo.width;
        vkViewport.height = viewportInfo.height;
        vkViewport.minDepth = viewportInfo.minDepth;
        vkViewport.maxDepth = viewportInfo.maxDepth;
        vkCmdSetViewport(commandBuffer, 0, 1, &vkViewport);
    }

    void VulkanCommands::SetScissor(const Rect& rect)
    {
        VkRect2D rect2D;
        rect2D.offset.x = static_cast<i32>(rect.x);
        rect2D.offset.y = static_cast<i32>(rect.y);
        rect2D.extent.width = static_cast<u32>(rect.width);
        rect2D.extent.height = static_cast<u32>(rect.height);
        vkCmdSetScissor(commandBuffer, 0, 1, &rect2D);
    }


    void VulkanCommands::BindVertexBuffer(const Buffer& gpuBuffer)
    {
        VkBuffer     vertexBuffers[] = {static_cast<const VulkanBuffer*>(gpuBuffer.handler)->buffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    }

    void VulkanCommands::BindIndexBuffer(const Buffer& gpuBuffer)
    {
        vkCmdBindIndexBuffer(commandBuffer, static_cast<const VulkanBuffer*>(gpuBuffer.handler)->buffer, 0, VK_INDEX_TYPE_UINT32);
    }

    void VulkanCommands::DrawIndexed(u32 indexCount, u32 instanceCount, u32 firstIndex, i32 vertexOffset, u32 firstInstance)
    {
        vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    void VulkanCommands::Draw(u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance)
    {
        vkCmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void VulkanCommands::PushConstants(const PipelineState& pipeline, ShaderStage stages, const void* data, usize size)
    {
        VulkanPipelineState& pipelineState = *static_cast<VulkanPipelineState*>(pipeline.handler);
        vkCmdPushConstants(commandBuffer, pipelineState.layout, Vulkan::CastStage(stages), 0, size, data);
    }

    void VulkanCommands::BindBindingSet(const PipelineState& pipeline, const BindingSet& bindingSet)
    {
    }

    void VulkanCommands::DrawIndexedIndirect(const Buffer& buffer, usize offset, u32 drawCount, u32 stride)
    {
        const VulkanBuffer& vulkanBuffer = *static_cast<const VulkanBuffer*>(buffer.handler);
        vkCmdDrawIndexedIndirect(commandBuffer, vulkanBuffer.buffer, offset, drawCount, stride);
    }

    void VulkanCommands::BindPipelineState(const PipelineState& pipeline)
    {
        auto& pipelineState = *static_cast<VulkanPipelineState*>(pipeline.handler);
        vkCmdBindPipeline(commandBuffer, pipelineState.bindingPoint, pipelineState.pipeline);
    }

    void VulkanCommands::Dispatch(u32 x, u32 y, u32 z)
    {
        vkCmdDispatch(commandBuffer, Math::Max(x, 1u), Math::Max(y, 1u), Math::Max(z, 1u));
    }

    void VulkanCommands::TraceRays(PipelineState pipeline, u32 x, u32 y, u32 z)
    {
    }

    void VulkanCommands::BeginLabel(const StringView& name, const Vec4& color)
    {
        if (vulkanDevice.validationLayersAvailable)
        {
            VkDebugUtilsLabelEXT vkDebugUtilsLabelExt{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
            vkDebugUtilsLabelExt.pLabelName = name.CStr();
            vkDebugUtilsLabelExt.color[0] = color.r;
            vkDebugUtilsLabelExt.color[1] = color.g;
            vkDebugUtilsLabelExt.color[2] = color.b;
            vkDebugUtilsLabelExt.color[3] = color.a;
            vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &vkDebugUtilsLabelExt);
        }
    }

    void VulkanCommands::EndLabel()
    {
        if (vulkanDevice.validationLayersAvailable)
        {
            vkCmdEndDebugUtilsLabelEXT(commandBuffer);
        }
    }

    void VulkanCommands::ResourceBarrier(const ResourceBarrierInfo& resourceBarrierInfo)
    {
        VkImageSubresourceRange subresourceRange = {};
        if (resourceBarrierInfo.isDepth)
        {
            subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        }
        else
        {
            subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }

        auto vulkanTexture = static_cast<VulkanTexture*>(resourceBarrierInfo.texture.handler);

        subresourceRange.baseMipLevel = resourceBarrierInfo.mipLevel;
        subresourceRange.levelCount = Math::Max(resourceBarrierInfo.levelCount, 1u);
        subresourceRange.layerCount = Math::Max(resourceBarrierInfo.layerCount, 1u);

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange = subresourceRange;
        barrier.oldLayout = Vulkan::CastLayout(resourceBarrierInfo.oldLayout);
        barrier.newLayout = Vulkan::CastLayout(resourceBarrierInfo.newLayout);
        barrier.image = vulkanTexture->image;

        switch (barrier.oldLayout)
        {
            case VK_IMAGE_LAYOUT_UNDEFINED:
                barrier.srcAccessMask = 0;
                break;

            case VK_IMAGE_LAYOUT_PREINITIALIZED:
                barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                break;
            default:
                // Other source layouts aren't handled (yet)
                break;
        }

        switch (barrier.newLayout)
        {
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                break;

            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                barrier.dstAccessMask = barrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                if (barrier.srcAccessMask == 0)
                {
                    barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
                }
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                break;
            default:
                // Other source layouts aren't handled (yet)
                break;
        }

        vkCmdPipelineBarrier(commandBuffer,
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);
    }
}
