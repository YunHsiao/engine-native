#ifndef CC_GFXGLES3_PRIMARY_COMMAND_BUFFER_H_
#define CC_GFXGLES3_PRIMARY_COMMAND_BUFFER_H_

#include "GLES3CommandBuffer.h"

namespace cc {
namespace gfx {

class CC_GLES3_API GLES3PrimaryCommandBuffer : public GLES3CommandBuffer {
    friend class GLES3Queue;

public:
    GLES3PrimaryCommandBuffer(Device *device);
    ~GLES3PrimaryCommandBuffer();

    virtual void beginRenderPass(RenderPass *renderPass, Framebuffer *fbo, const Rect &renderArea, const Color *colors, float depth, int stencil) override;
    virtual void endRenderPass() override;
    virtual void bindPipelineState(PipelineState *pso) override;
    virtual void bindDescriptorSet(uint set, DescriptorSet *descriptorSet, uint dynamicOffsetCount, const uint *dynamicOffsets) override;
    virtual void bindInputAssembler(InputAssembler *ia) override;
    virtual void setViewport(const Viewport &vp) override;
    virtual void setScissor(const Rect &rect) override;
    virtual void setLineWidth(const float width) override;
    virtual void setDepthBias(float constant, float clamp, float slope) override;
    virtual void setBlendConstants(const Color &constants) override;
    virtual void setDepthBound(float minBounds, float maxBounds) override;
    virtual void setStencilWriteMask(StencilFace face, uint mask) override;
    virtual void setStencilCompareMask(StencilFace face, int ref, uint mask) override;
    virtual void draw(InputAssembler *ia) override;
    virtual void updateBuffer(Buffer *buff, const void *data, uint size, uint offset) override;
    virtual void copyBuffersToTexture(const uint8_t *const *buffers, Texture *texture, const BufferTextureCopy *regions, uint count) override;
    virtual void execute(const CommandBuffer *const *cmdBuffs, uint32_t count) override;

private:
    bool _isShaderChanged = false;
};

} // namespace gfx
} // namespace cc

#endif
