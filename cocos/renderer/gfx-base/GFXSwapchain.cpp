/****************************************************************************
 Copyright (c) 2019-2021 Xiamen Yaji Software Co., Ltd.

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

#include "base/CoreStd.h"

#include "GFXSwapchain.h"
#include "gfx-base/GFXDef-common.h"

namespace cc {
namespace gfx {

Swapchain::Swapchain()
: Texture() {
    _objectType = ObjectType::SWAPCHAIN;
}

Swapchain::~Swapchain() = default;

void Swapchain::initialize(const SwapchainInfo &info) {
    CCASSERT(info.windowHandle, "Invalid window handle");

    _windowHandle       = info.windowHandle;
    _vsyncMode          = info.vsyncMode;
    _depthStencilFormat = info.depthStencilFormat;

    _type       = TextureType::TEX2D;
    _usage      = TextureUsageBit::COLOR_ATTACHMENT;
    _format     = info.format;
    _width      = info.width;
    _height     = info.height;
    _depth      = 1;
    _layerCount = 1;
    _levelCount = 1;
    _samples    = info.samples;
    _flags      = TextureFlagBit::NONE;
    _size       = formatSize(_format, _width, _height, _depth);

    doInit(info);
}

void Swapchain::destroy() {
    Texture::destroy();

    _windowHandle = nullptr;
}

} // namespace gfx
} // namespace cc