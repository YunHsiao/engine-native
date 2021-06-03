/****************************************************************************
 Copyright (c) 2020-2021 Xiamen Yaji Software Co., Ltd.

 http://www.cocos.com

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated engine source code (the "Software"), a limited,
 worldwide, royalty-free, non-assignable, revocable and non-exclusive license
 to use Cocos Creator solely to develop games on your target platforms. You shall
 not use Cocos Creator software for developing other software or tools that's
 used for developing games. You are not granted to publish, distribute,
 sublicense, and/or sell copies of Cocos Creator.

 The software or tools in this License Agreement are licensed, not sold.
 Xiamen Yaji Software Co., Ltd. reserves all rights not expressly granted to you.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
****************************************************************************/

#include "VKStd.h"

#include "VKBuffer.h"
#include "VKCommands.h"
#include "VKDescriptorSet.h"
#include "VKDescriptorSetLayout.h"
#include "VKDevice.h"
#include "VKPipelineLayout.h"
#include "VKSampler.h"
#include "VKShader.h"
#include "VKTexture.h"

namespace cc {
namespace gfx {

CCVKDescriptorSet::CCVKDescriptorSet() = default;

CCVKDescriptorSet::~CCVKDescriptorSet() {
    destroy();
}

void CCVKDescriptorSet::doInit(const DescriptorSetInfo & /*info*/) {
    CCVKGPUDescriptorSetLayout *gpuDescriptorSetLayout = static_cast<CCVKDescriptorSetLayout *>(_layout)->gpuDescriptorSetLayout();
    const uint                  bindingCount           = gpuDescriptorSetLayout->bindings.size();
    const uint                  descriptorCount        = gpuDescriptorSetLayout->descriptorCount;

    _gpuDescriptorSet = CC_NEW(CCVKGPUDescriptorSet);
    _gpuDescriptorSet->gpuDescriptors.resize(descriptorCount, {});
    _gpuDescriptorSet->layoutID = gpuDescriptorSetLayout->id;

    for (size_t i = 0U, k = 0U; i < bindingCount; ++i) {
        const DescriptorSetLayoutBinding &binding = gpuDescriptorSetLayout->bindings[i];
        for (uint j = 0; j < binding.count; ++j, ++k) {
            CCVKGPUDescriptor &gpuDescriptor = _gpuDescriptorSet->gpuDescriptors[k];
            gpuDescriptor.type               = binding.descriptorType;
            switch (binding.descriptorType) {
                case DescriptorType::UNIFORM_BUFFER:
                case DescriptorType::DYNAMIC_UNIFORM_BUFFER:
                    if (hasFlag(binding.stageFlags, ShaderStageFlags::COMPUTE)) gpuDescriptor.accessTypes.push_back(THSVS_ACCESS_COMPUTE_SHADER_READ_UNIFORM_BUFFER);
                    if (hasFlag(binding.stageFlags, ShaderStageFlags::VERTEX)) gpuDescriptor.accessTypes.push_back(THSVS_ACCESS_VERTEX_SHADER_READ_UNIFORM_BUFFER);
                    if (hasFlag(binding.stageFlags, ShaderStageFlags::FRAGMENT)) gpuDescriptor.accessTypes.push_back(THSVS_ACCESS_FRAGMENT_SHADER_READ_UNIFORM_BUFFER);
                    break;
                case DescriptorType::STORAGE_BUFFER:
                case DescriptorType::DYNAMIC_STORAGE_BUFFER:
                case DescriptorType::STORAGE_IMAGE:
                    // write accesses should be handled manually
                    if (hasFlag(binding.stageFlags, ShaderStageFlags::COMPUTE)) gpuDescriptor.accessTypes.push_back(THSVS_ACCESS_COMPUTE_SHADER_READ_OTHER);
                    if (hasFlag(binding.stageFlags, ShaderStageFlags::VERTEX)) gpuDescriptor.accessTypes.push_back(THSVS_ACCESS_VERTEX_SHADER_READ_OTHER);
                    if (hasFlag(binding.stageFlags, ShaderStageFlags::FRAGMENT)) gpuDescriptor.accessTypes.push_back(THSVS_ACCESS_FRAGMENT_SHADER_READ_OTHER);
                    break;
                case DescriptorType::SAMPLER_TEXTURE:
                case DescriptorType::TEXTURE:
                    if (hasFlag(binding.stageFlags, ShaderStageFlags::COMPUTE)) gpuDescriptor.accessTypes.push_back(THSVS_ACCESS_COMPUTE_SHADER_READ_SAMPLED_IMAGE_OR_UNIFORM_TEXEL_BUFFER);
                    if (hasFlag(binding.stageFlags, ShaderStageFlags::VERTEX)) gpuDescriptor.accessTypes.push_back(THSVS_ACCESS_VERTEX_SHADER_READ_SAMPLED_IMAGE_OR_UNIFORM_TEXEL_BUFFER);
                    if (hasFlag(binding.stageFlags, ShaderStageFlags::FRAGMENT)) gpuDescriptor.accessTypes.push_back(THSVS_ACCESS_FRAGMENT_SHADER_READ_SAMPLED_IMAGE_OR_UNIFORM_TEXEL_BUFFER);
                    break;
                case DescriptorType::INPUT_ATTACHMENT:
                    gpuDescriptor.accessTypes.push_back(THSVS_ACCESS_FRAGMENT_SHADER_READ_COLOR_INPUT_ATTACHMENT);
                    break;
                case DescriptorType::SAMPLER:
                default:
                    break;
            }
        }
    }

    CCVKGPUDevice *gpuDevice     = CCVKDevice::getInstance()->gpuDevice();
    _gpuDescriptorSet->gpuLayout = gpuDescriptorSetLayout;

    _gpuDescriptorSet->vkDescriptorSet = gpuDevice->getDescriptorSetPool(_gpuDescriptorSet->layoutID)->request(0);
    _gpuDescriptorSet->descriptorInfos.resize(descriptorCount, {});

    for (size_t i = 0U, k = 0U; i < bindingCount; ++i) {
        const DescriptorSetLayoutBinding &binding = gpuDescriptorSetLayout->bindings[i];
        for (uint j = 0; j < binding.count; ++j, ++k) {
            if (hasFlag(DESCRIPTOR_BUFFER_TYPE, binding.descriptorType)) {
                _gpuDescriptorSet->descriptorInfos[k].buffer.buffer = gpuDevice->defaultBuffer.vkBuffer;
                _gpuDescriptorSet->descriptorInfos[k].buffer.offset = gpuDevice->defaultBuffer.startOffset;
                _gpuDescriptorSet->descriptorInfos[k].buffer.range  = gpuDevice->defaultBuffer.size;
            } else if (hasFlag(DESCRIPTOR_TEXTURE_TYPE, binding.descriptorType)) {
                _gpuDescriptorSet->descriptorInfos[k].image.sampler     = gpuDevice->defaultSampler.vkSampler;
                _gpuDescriptorSet->descriptorInfos[k].image.imageView   = gpuDevice->defaultTextureView.vkImageView;
                _gpuDescriptorSet->descriptorInfos[k].image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            }
        }
    }

    if (!gpuDevice->useDescriptorUpdateTemplate) {
        vector<VkWriteDescriptorSet> &entries = _gpuDescriptorSet->descriptorUpdateEntries;
        entries.resize(descriptorCount, {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET});

        for (size_t i = 0U, j = 0U; i < bindingCount; i++) {
            const VkDescriptorSetLayoutBinding &descriptor = gpuDescriptorSetLayout->vkBindings[i];
            for (size_t k = 0U; k < descriptor.descriptorCount; k++, j++) {
                entries[j].dstSet          = _gpuDescriptorSet->vkDescriptorSet;
                entries[j].dstBinding      = descriptor.binding;
                entries[j].dstArrayElement = k;
                entries[j].descriptorCount = 1; // better not to assume that the descriptor infos would be contiguous
                entries[j].descriptorType  = descriptor.descriptorType;
                switch (entries[j].descriptorType) {
                    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
                    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                        entries[j].pBufferInfo = &_gpuDescriptorSet->descriptorInfos[j].buffer;
                        break;
                    case VK_DESCRIPTOR_TYPE_SAMPLER:
                    case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                    case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                    case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                    case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
                        entries[j].pImageInfo = &_gpuDescriptorSet->descriptorInfos[j].image;
                        break;
                    case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
                    case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
                        entries[j].pTexelBufferView = &_gpuDescriptorSet->descriptorInfos[j].texelBufferView;
                        break;
                    default: break;
                }
            }
        }
    }
}

void CCVKDescriptorSet::doDestroy() {
    if (_gpuDescriptorSet) {
        CCVKGPUDevice *       gpuDevice     = CCVKDevice::getInstance()->gpuDevice();
        CCVKGPUDescriptorHub *descriptorHub = CCVKDevice::getInstance()->gpuDescriptorHub();

        for (uint i = 0U; i < _gpuDescriptorSet->gpuDescriptors.size(); i++) {
            CCVKGPUDescriptor &binding = _gpuDescriptorSet->gpuDescriptors[i];

            CCVKDescriptorInfo &descriptorInfo = _gpuDescriptorSet->descriptorInfos[i];
            if (binding.gpuBufferView) {
                descriptorHub->disengage(binding.gpuBufferView, &descriptorInfo.buffer);
            }
            if (binding.gpuTextureView) {
                descriptorHub->disengage(binding.gpuTextureView, &descriptorInfo.image);
            }
            if (binding.gpuSampler) {
                descriptorHub->disengage(binding.gpuSampler, &descriptorInfo.image);
            }
        }

        if (_gpuDescriptorSet->vkDescriptorSet) {
            gpuDevice->getDescriptorSetPool(_gpuDescriptorSet->layoutID)->yield(_gpuDescriptorSet->vkDescriptorSet, 0);
        }

        CC_DELETE(_gpuDescriptorSet);
        _gpuDescriptorSet = nullptr;
    }
}

void CCVKDescriptorSet::update() {
    if (_isDirty && _gpuDescriptorSet) {
        CCVKGPUDescriptorHub * descriptorHub   = CCVKDevice::getInstance()->gpuDescriptorHub();
        CCVKGPUBarrierManager *layoutMgr       = CCVKDevice::getInstance()->gpuBarrierManager();
        uint                   descriptorCount = _gpuDescriptorSet->gpuDescriptors.size();

        for (size_t i = 0U; i < descriptorCount; i++) {
            CCVKGPUDescriptor &binding = _gpuDescriptorSet->gpuDescriptors[i];

            if (hasFlag(DESCRIPTOR_BUFFER_TYPE, binding.type)) {
                if (_buffers[i]) {
                    CCVKGPUBufferView *bufferView = static_cast<CCVKBuffer *>(_buffers[i])->gpuBufferView();
                    if (binding.gpuBufferView != bufferView) {
                        CCVKDescriptorInfo &descriptorInfo = _gpuDescriptorSet->descriptorInfos[i];

                        if (binding.gpuBufferView) {
                            descriptorHub->disengage(binding.gpuBufferView, &descriptorInfo.buffer);
                        }
                        if (bufferView) {
                            descriptorHub->connect(_gpuDescriptorSet, bufferView, &descriptorInfo.buffer);
                            descriptorHub->update(bufferView, &descriptorInfo.buffer);
                        }
                        binding.gpuBufferView = bufferView;
                    }
                }
            } else if (hasFlag(DESCRIPTOR_TEXTURE_TYPE, binding.type)) {
                if (_textures[i]) {
                    CCVKGPUTextureView *textureView = static_cast<CCVKTexture *>(_textures[i])->gpuTextureView();
                    if (binding.gpuTextureView != textureView) {
                        CCVKDescriptorInfo &descriptorInfo = _gpuDescriptorSet->descriptorInfos[i];
                        if (binding.gpuTextureView) {
                            descriptorHub->disengage(binding.gpuTextureView, &descriptorInfo.image);
                        }
                        if (textureView) {
                            descriptorHub->connect(_gpuDescriptorSet, textureView, &descriptorInfo.image);
                            descriptorHub->update(textureView, &descriptorInfo.image);
                            layoutMgr->checkIn(textureView->gpuTexture, binding.accessTypes.data(), binding.accessTypes.size());
                        }
                        binding.gpuTextureView = textureView;
                    }
                }
                if (_samplers[i]) {
                    CCVKGPUSampler *sampler = static_cast<CCVKSampler *>(_samplers[i])->gpuSampler();
                    if (binding.gpuSampler != sampler) {
                        CCVKDescriptorInfo &descriptorInfo = _gpuDescriptorSet->descriptorInfos[i];
                        if (binding.gpuSampler) {
                            descriptorHub->disengage(binding.gpuSampler, &descriptorInfo.image);
                        }
                        if (sampler) {
                            descriptorHub->connect(sampler, &descriptorInfo.image);
                            descriptorHub->update(sampler, &descriptorInfo.image);
                        }
                        binding.gpuSampler = sampler;
                    }
                }
            }
        }
        _gpuDescriptorSet->update(CCVKDevice::getInstance()->gpuDevice());
        _isDirty = false;
    }
}

} // namespace gfx
} // namespace cc
