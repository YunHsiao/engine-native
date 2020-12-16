#include "VKStd.h"

#include "VKCommandBuffer.h"
#include "VKCommands.h"
#include "VKDevice.h"
#include "VKFence.h"
#include "VKQueue.h"

namespace cc {
namespace gfx {

CCVKQueue::CCVKQueue(Device *device)
: Queue(device) {
}

CCVKQueue::~CCVKQueue() {
}

bool CCVKQueue::initialize(const QueueInfo &info) {
    _type = info.type;
    _isAsync = true;

    _gpuQueue = CC_NEW(CCVKGPUQueue);
    _gpuQueue->type = _type;
    CCVKCmdFuncGetDeviceQueue((CCVKDevice *)_device, _gpuQueue);

    return true;
}

void CCVKQueue::destroy() {
    if (_gpuQueue) {
        _gpuQueue->vkQueue = VK_NULL_HANDLE;
        CC_DELETE(_gpuQueue);
        _gpuQueue = nullptr;
    }
}

void CCVKQueue::submit(const CommandBuffer *const *cmdBuffs, uint count, Fence *fence) {
    CCVKDevice *device = (CCVKDevice *)_device;
    _gpuQueue->commandBuffers.clear();
    device->gpuTransportHub()->depart();

    for (uint i = 0u; i < count; ++i) {
        CCVKCommandBuffer *cmdBuffer = (CCVKCommandBuffer *)cmdBuffs[i];
        _gpuQueue->commandBuffers.push(cmdBuffer->_gpuCommandBuffer->vkCommandBuffer);
        _numDrawCalls += cmdBuffer->_numDrawCalls;
        _numInstances += cmdBuffer->_numInstances;
        _numTriangles += cmdBuffer->_numTriangles;
        cmdBuffer->_gpuCommandBuffer->vkCommandBuffer = VK_NULL_HANDLE;
    }

    count = _gpuQueue->commandBuffers.size();
    _gpuQueue->waitSemaphores.reserve(count);
    _gpuQueue->waitSemaphores.clear();
    _gpuQueue->signalSemaphores.reserve(count);
    _gpuQueue->signalSemaphores.clear();
    _gpuQueue->submitInfos.resize(count, {VK_STRUCTURE_TYPE_SUBMIT_INFO});
    for (uint i = 0u; i < count; ++i) {
        _gpuQueue->waitSemaphores.push(_gpuQueue->nextWaitSemaphore);
        _gpuQueue->signalSemaphores.push(_gpuQueue->nextSignalSemaphore);
        _gpuQueue->nextWaitSemaphore = _gpuQueue->nextSignalSemaphore;
        _gpuQueue->nextSignalSemaphore = device->gpuSemaphorePool()->alloc();

        VkSubmitInfo &submitInfo = _gpuQueue->submitInfos[i];
        submitInfo.waitSemaphoreCount = _gpuQueue->waitSemaphores[i] ? 1 : 0;
        submitInfo.pWaitSemaphores = &_gpuQueue->waitSemaphores[i];
        submitInfo.pWaitDstStageMask = &_gpuQueue->submitStageMask;
        submitInfo.commandBufferCount = _gpuQueue->commandBuffers[i] ? 1 : 0;
        submitInfo.pCommandBuffers = &_gpuQueue->commandBuffers[i];
        submitInfo.signalSemaphoreCount = _gpuQueue->signalSemaphores[i] ? 1 : 0;
        submitInfo.pSignalSemaphores = &_gpuQueue->signalSemaphores[i];
    }

    VkFence vkFence = fence ? ((CCVKFence *)fence)->gpuFence()->vkFence : device->gpuFencePool()->alloc();
    VK_CHECK(vkQueueSubmit(_gpuQueue->vkQueue, count, _gpuQueue->submitInfos.data(), vkFence));
}

} // namespace gfx
} // namespace cc
