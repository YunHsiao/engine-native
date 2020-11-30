#include "GLES3Std.h"

#include "GLES3Buffer.h"
#include "GLES3DescriptorSet.h"
#include "GLES3Device.h"
#include "GLES3Framebuffer.h"
#include "GLES3InputAssembler.h"
#include "GLES3PipelineState.h"
#include "GLES3PrimaryCommandBuffer.h"
#include "GLES3RenderPass.h"
#include "GLES3Texture.h"

namespace cc {
namespace gfx {

namespace {
const GLenum GLES3_CMP_FUNCS[] = {
    GL_NEVER,
    GL_LESS,
    GL_EQUAL,
    GL_LEQUAL,
    GL_GREATER,
    GL_NOTEQUAL,
    GL_GEQUAL,
    GL_ALWAYS,
};

const GLenum GLES3_STENCIL_OPS[] = {
    GL_ZERO,
    GL_KEEP,
    GL_REPLACE,
    GL_INCR,
    GL_DECR,
    GL_INVERT,
    GL_INCR_WRAP,
    GL_DECR_WRAP,
};

const GLenum GLES3_BLEND_OPS[] = {
    GL_FUNC_ADD,
    GL_FUNC_SUBTRACT,
    GL_FUNC_REVERSE_SUBTRACT,
    GL_MIN,
    GL_MAX,
};

const GLenum GLES3_BLEND_FACTORS[] = {
    GL_ZERO,
    GL_ONE,
    GL_SRC_ALPHA,
    GL_DST_ALPHA,
    GL_ONE_MINUS_SRC_ALPHA,
    GL_ONE_MINUS_DST_ALPHA,
    GL_SRC_COLOR,
    GL_DST_COLOR,
    GL_ONE_MINUS_SRC_COLOR,
    GL_ONE_MINUS_DST_COLOR,
    GL_SRC_ALPHA_SATURATE,
    GL_CONSTANT_COLOR,
    GL_ONE_MINUS_CONSTANT_COLOR,
    GL_CONSTANT_ALPHA,
    GL_ONE_MINUS_CONSTANT_ALPHA,
};
}

GLES3PrimaryCommandBuffer::GLES3PrimaryCommandBuffer(Device *device)
: GLES3CommandBuffer(device) {
}

GLES3PrimaryCommandBuffer::~GLES3PrimaryCommandBuffer() {
}

void GLES3PrimaryCommandBuffer::beginRenderPass(RenderPass *renderPass, Framebuffer *fbo, const Rect &renderArea, const Color *colors, float depth, int stencil) {
    _isInRenderPass = true;
    GLES3GPURenderPass *gpuRenderPass = ((GLES3RenderPass *)renderPass)->gpuRenderPass();
    GLES3GPUFramebuffer *gpuFramebuffer = ((GLES3Framebuffer *)fbo)->gpuFBO();

    GLES3CmdFuncBeginRenderPass((GLES3Device *)_device, gpuRenderPass, gpuFramebuffer,
                                renderArea, gpuRenderPass->colorAttachments.size(), colors, depth, stencil);
}

void GLES3PrimaryCommandBuffer::endRenderPass() {
    GLES3CmdFuncEndRenderPass((GLES3Device *)_device);
    _isInRenderPass = false;
}

void GLES3PrimaryCommandBuffer::bindPipelineState(PipelineState *pso) {
    GLES3Device *device = ((GLES3Device *)_device);
    GLES3GPUStateCache *cache = device->stateCache();
    GLES3ObjectCache &gfxStateCache = cache->gfxStateCache;

    GLES3GPUPipelineState *gpuPipelineState = ((GLES3PipelineState *)pso)->gpuPipelineState();
    if (gpuPipelineState && gpuPipelineState != gfxStateCache.gpuPipelineState) {
        gfxStateCache.gpuPipelineState = gpuPipelineState;
        gfxStateCache.glPrimitive = gpuPipelineState->glPrimitive;

        if (gpuPipelineState->gpuShader) {
            if (cache->glProgram != gpuPipelineState->gpuShader->glProgram) {
                GL_CHECK(glUseProgram(gpuPipelineState->gpuShader->glProgram));
                cache->glProgram = gpuPipelineState->gpuShader->glProgram;
                _isShaderChanged = true;
            }
        }

        // bind rasterizer state
        if (cache->rs.cullMode != gpuPipelineState->rs.cullMode) {
            switch (gpuPipelineState->rs.cullMode) {
                case CullMode::NONE: {
                    if (cache->isCullFaceEnabled) {
                        GL_CHECK(glDisable(GL_CULL_FACE));
                        cache->isCullFaceEnabled = false;
                    }
                } break;
                case CullMode::FRONT: {
                    if (!cache->isCullFaceEnabled) {
                        GL_CHECK(glEnable(GL_CULL_FACE));
                        cache->isCullFaceEnabled = true;
                    }
                    GL_CHECK(glCullFace(GL_FRONT));
                } break;
                case CullMode::BACK: {
                    if (!cache->isCullFaceEnabled) {
                        GL_CHECK(glEnable(GL_CULL_FACE));
                        cache->isCullFaceEnabled = true;
                    }
                    GL_CHECK(glCullFace(GL_BACK));
                } break;
                default:
                    break;
            }
            cache->rs.cullMode = gpuPipelineState->rs.cullMode;
        }
        bool isFrontFaceCCW = gpuPipelineState->rs.isFrontFaceCCW != gfxStateCache.reverseCW;
        if (cache->rs.isFrontFaceCCW != isFrontFaceCCW) {
            GL_CHECK(glFrontFace(isFrontFaceCCW ? GL_CCW : GL_CW));
            cache->rs.isFrontFaceCCW = isFrontFaceCCW;
        }
        if ((cache->rs.depthBias != gpuPipelineState->rs.depthBias) ||
            (cache->rs.depthBiasSlop != gpuPipelineState->rs.depthBiasSlop)) {
            GL_CHECK(glPolygonOffset(cache->rs.depthBias, cache->rs.depthBiasSlop));
            cache->rs.depthBiasSlop = gpuPipelineState->rs.depthBiasSlop;
        }
        if (cache->rs.lineWidth != gpuPipelineState->rs.lineWidth) {
            GL_CHECK(glLineWidth(gpuPipelineState->rs.lineWidth));
            cache->rs.lineWidth = gpuPipelineState->rs.lineWidth;
        }

        // bind depth-stencil state
        if (cache->dss.depthTest != gpuPipelineState->dss.depthTest) {
            if (gpuPipelineState->dss.depthTest) {
                GL_CHECK(glEnable(GL_DEPTH_TEST));
            } else {
                GL_CHECK(glDisable(GL_DEPTH_TEST));
            }
            cache->dss.depthTest = gpuPipelineState->dss.depthTest;
        }
        if (cache->dss.depthWrite != gpuPipelineState->dss.depthWrite) {
            GL_CHECK(glDepthMask(gpuPipelineState->dss.depthWrite));
            cache->dss.depthWrite = gpuPipelineState->dss.depthWrite;
        }
        if (cache->dss.depthFunc != gpuPipelineState->dss.depthFunc) {
            GL_CHECK(glDepthFunc(GLES3_CMP_FUNCS[(int)gpuPipelineState->dss.depthFunc]));
            cache->dss.depthFunc = gpuPipelineState->dss.depthFunc;
        }

        // bind depth-stencil state - front
        if (gpuPipelineState->dss.stencilTestFront || gpuPipelineState->dss.stencilTestBack) {
            if (!cache->isStencilTestEnabled) {
                GL_CHECK(glEnable(GL_STENCIL_TEST));
                cache->isStencilTestEnabled = true;
            }
        } else {
            if (cache->isStencilTestEnabled) {
                GL_CHECK(glDisable(GL_STENCIL_TEST));
                cache->isStencilTestEnabled = false;
            }
        }
        if (cache->dss.stencilFuncFront != gpuPipelineState->dss.stencilFuncFront ||
            cache->dss.stencilRefFront != gpuPipelineState->dss.stencilRefFront ||
            cache->dss.stencilReadMaskFront != gpuPipelineState->dss.stencilReadMaskFront) {
            GL_CHECK(glStencilFuncSeparate(GL_FRONT,
                                  GLES3_CMP_FUNCS[(int)gpuPipelineState->dss.stencilFuncFront],
                                  gpuPipelineState->dss.stencilRefFront,
                                  gpuPipelineState->dss.stencilReadMaskFront));
            cache->dss.stencilFuncFront = gpuPipelineState->dss.stencilFuncFront;
            cache->dss.stencilRefFront = gpuPipelineState->dss.stencilRefFront;
            cache->dss.stencilReadMaskFront = gpuPipelineState->dss.stencilReadMaskFront;
        }
        if (cache->dss.stencilFailOpFront != gpuPipelineState->dss.stencilFailOpFront ||
            cache->dss.stencilZFailOpFront != gpuPipelineState->dss.stencilZFailOpFront ||
            cache->dss.stencilPassOpFront != gpuPipelineState->dss.stencilPassOpFront) {
            GL_CHECK(glStencilOpSeparate(GL_FRONT,
                                GLES3_STENCIL_OPS[(int)gpuPipelineState->dss.stencilFailOpFront],
                                GLES3_STENCIL_OPS[(int)gpuPipelineState->dss.stencilZFailOpFront],
                                GLES3_STENCIL_OPS[(int)gpuPipelineState->dss.stencilPassOpFront]));
            cache->dss.stencilFailOpFront = gpuPipelineState->dss.stencilFailOpFront;
            cache->dss.stencilZFailOpFront = gpuPipelineState->dss.stencilZFailOpFront;
            cache->dss.stencilPassOpFront = gpuPipelineState->dss.stencilPassOpFront;
        }
        if (cache->dss.stencilWriteMaskFront != gpuPipelineState->dss.stencilWriteMaskFront) {
            GL_CHECK(glStencilMaskSeparate(GL_FRONT, gpuPipelineState->dss.stencilWriteMaskFront));
            cache->dss.stencilWriteMaskFront = gpuPipelineState->dss.stencilWriteMaskFront;
        }

        // bind depth-stencil state - back
        if (cache->dss.stencilFuncBack != gpuPipelineState->dss.stencilFuncBack ||
            cache->dss.stencilRefBack != gpuPipelineState->dss.stencilRefBack ||
            cache->dss.stencilReadMaskBack != gpuPipelineState->dss.stencilReadMaskBack) {
            GL_CHECK(glStencilFuncSeparate(GL_BACK,
                                  GLES3_CMP_FUNCS[(int)gpuPipelineState->dss.stencilFuncBack],
                                  gpuPipelineState->dss.stencilRefBack,
                                  gpuPipelineState->dss.stencilReadMaskBack));
            cache->dss.stencilFuncBack = gpuPipelineState->dss.stencilFuncBack;
            cache->dss.stencilRefBack = gpuPipelineState->dss.stencilRefBack;
            cache->dss.stencilReadMaskBack = gpuPipelineState->dss.stencilReadMaskBack;
        }
        if (cache->dss.stencilFailOpBack != gpuPipelineState->dss.stencilFailOpBack ||
            cache->dss.stencilZFailOpBack != gpuPipelineState->dss.stencilZFailOpBack ||
            cache->dss.stencilPassOpBack != gpuPipelineState->dss.stencilPassOpBack) {
            GL_CHECK(glStencilOpSeparate(GL_BACK,
                                GLES3_STENCIL_OPS[(int)gpuPipelineState->dss.stencilFailOpBack],
                                GLES3_STENCIL_OPS[(int)gpuPipelineState->dss.stencilZFailOpBack],
                                GLES3_STENCIL_OPS[(int)gpuPipelineState->dss.stencilPassOpBack]));
            cache->dss.stencilFailOpBack = gpuPipelineState->dss.stencilFailOpBack;
            cache->dss.stencilZFailOpBack = gpuPipelineState->dss.stencilZFailOpBack;
            cache->dss.stencilPassOpBack = gpuPipelineState->dss.stencilPassOpBack;
        }
        if (cache->dss.stencilWriteMaskBack != gpuPipelineState->dss.stencilWriteMaskBack) {
            GL_CHECK(glStencilMaskSeparate(GL_BACK, gpuPipelineState->dss.stencilWriteMaskBack));
            cache->dss.stencilWriteMaskBack = gpuPipelineState->dss.stencilWriteMaskBack;
        }

        // bind blend state
        if (cache->bs.isA2C != gpuPipelineState->bs.isA2C) {
            if (cache->bs.isA2C) {
                GL_CHECK(glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE));
            } else {
                GL_CHECK(glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE));
            }
            cache->bs.isA2C = gpuPipelineState->bs.isA2C;
        }
        if (cache->bs.blendColor.x != gpuPipelineState->bs.blendColor.x ||
            cache->bs.blendColor.y != gpuPipelineState->bs.blendColor.y ||
            cache->bs.blendColor.z != gpuPipelineState->bs.blendColor.z ||
            cache->bs.blendColor.w != gpuPipelineState->bs.blendColor.w) {

            GL_CHECK(glBlendColor(gpuPipelineState->bs.blendColor.x,
                         gpuPipelineState->bs.blendColor.y,
                         gpuPipelineState->bs.blendColor.z,
                         gpuPipelineState->bs.blendColor.w));
            cache->bs.blendColor = gpuPipelineState->bs.blendColor;
        }

        BlendTarget &cacheTarget = cache->bs.targets[0];
        const BlendTarget &target = gpuPipelineState->bs.targets[0];
        if (cacheTarget.blend != target.blend) {
            if (!cacheTarget.blend) {
                GL_CHECK(glEnable(GL_BLEND));
            } else {
                GL_CHECK(glDisable(GL_BLEND));
            }
            cacheTarget.blend = target.blend;
        }
        if (cacheTarget.blendEq != target.blendEq ||
            cacheTarget.blendAlphaEq != target.blendAlphaEq) {
            GL_CHECK(glBlendEquationSeparate(GLES3_BLEND_OPS[(int)target.blendEq],
                                    GLES3_BLEND_OPS[(int)target.blendAlphaEq]));
            cacheTarget.blendEq = target.blendEq;
            cacheTarget.blendAlphaEq = target.blendAlphaEq;
        }
        if (cacheTarget.blendSrc != target.blendSrc ||
            cacheTarget.blendDst != target.blendDst ||
            cacheTarget.blendSrcAlpha != target.blendSrcAlpha ||
            cacheTarget.blendDstAlpha != target.blendDstAlpha) {
            GL_CHECK(glBlendFuncSeparate(GLES3_BLEND_FACTORS[(int)target.blendSrc],
                                GLES3_BLEND_FACTORS[(int)target.blendDst],
                                GLES3_BLEND_FACTORS[(int)target.blendSrcAlpha],
                                GLES3_BLEND_FACTORS[(int)target.blendDstAlpha]));
            cacheTarget.blendSrc = target.blendSrc;
            cacheTarget.blendDst = target.blendDst;
            cacheTarget.blendSrcAlpha = target.blendSrcAlpha;
            cacheTarget.blendDstAlpha = target.blendDstAlpha;
        }
        if (cacheTarget.blendColorMask != target.blendColorMask) {
            GL_CHECK(glColorMask((GLboolean)(target.blendColorMask & ColorMask::R),
                        (GLboolean)(target.blendColorMask & ColorMask::G),
                        (GLboolean)(target.blendColorMask & ColorMask::B),
                        (GLboolean)(target.blendColorMask & ColorMask::A)));
            cacheTarget.blendColorMask = target.blendColorMask;
        }
    }
}

void GLES3PrimaryCommandBuffer::bindDescriptorSet(uint set, DescriptorSet *descriptorSet, uint dynamicOffsetCount, const uint *dynamicOffsets) {
    GLES3Device *device = ((GLES3Device *)_device);
    GLES3GPUStateCache *cache = device->stateCache();
    GLES3ObjectCache &gfxStateCache = cache->gfxStateCache;

    GLES3GPUDescriptorSet *gpuDescriptorSet = ((GLES3DescriptorSet *)descriptorSet)->gpuDescriptorSet();
    GLES3GPUPipelineState *gpuPipelineState = gfxStateCache.gpuPipelineState;
    if (gpuPipelineState && gpuPipelineState->gpuShader && gpuPipelineState->gpuPipelineLayout) {

        size_t blockLen = gpuPipelineState->gpuShader->glBlocks.size();
        const vector<vector<int>> &dynamicOffsetIndices = gpuPipelineState->gpuPipelineLayout->dynamicOffsetIndices;

        for (size_t j = 0; j < blockLen; j++) {
            const GLES3GPUUniformBlock &glBlock = gpuPipelineState->gpuShader->glBlocks[j];
            if (glBlock.set != set) continue;
            const uint descriptorIndex = gpuDescriptorSet->descriptorIndices->at(glBlock.binding);
            const GLES3GPUDescriptor &gpuDescriptor = gpuDescriptorSet->gpuDescriptors[descriptorIndex];

            if (!gpuDescriptor.gpuBuffer) {
                CC_LOG_ERROR("Buffer binding '%s' at set %d binding %d is not bounded",
                             glBlock.name.c_str(), glBlock.set, glBlock.binding);
                continue;
            }

            uint offset = gpuDescriptor.gpuBuffer->glOffset;

            const vector<int> &dynamicOffsetSetIndices = dynamicOffsetIndices[glBlock.set];
            int dynamicOffsetIndex = glBlock.binding < dynamicOffsetSetIndices.size() ? dynamicOffsetSetIndices[glBlock.binding] : -1;
            if (dynamicOffsetIndex >= 0) offset += dynamicOffsets[dynamicOffsetIndex];

            if (cache->glBindUBOs[glBlock.glBinding] != gpuDescriptor.gpuBuffer->glBuffer ||
                cache->glBindUBOOffsets[glBlock.glBinding] != offset) {
                if (offset) {
                    GL_CHECK(glBindBufferRange(GL_UNIFORM_BUFFER, glBlock.glBinding, gpuDescriptor.gpuBuffer->glBuffer,
                                      offset, gpuDescriptor.gpuBuffer->size));
                } else {
                    GL_CHECK(glBindBufferBase(GL_UNIFORM_BUFFER, glBlock.glBinding, gpuDescriptor.gpuBuffer->glBuffer));
                }
                cache->glUniformBuffer = cache->glBindUBOs[glBlock.glBinding] = gpuDescriptor.gpuBuffer->glBuffer;
                cache->glBindUBOOffsets[glBlock.glBinding] = offset;
            }
        }

        size_t samplerLen = gpuPipelineState->gpuShader->glSamplers.size();
        for (size_t j = 0; j < samplerLen; j++) {
            const GLES3GPUUniformSampler &glSampler = gpuPipelineState->gpuShader->glSamplers[j];
            if (glSampler.set != set) continue;
            const uint descriptorIndex = gpuDescriptorSet->descriptorIndices->at(glSampler.binding);
            const GLES3GPUDescriptor *gpuDescriptor = &gpuDescriptorSet->gpuDescriptors[descriptorIndex];

            for (size_t u = 0; u < glSampler.units.size(); u++, gpuDescriptor++) {
                uint unit = (uint)glSampler.units[u];

                if (!gpuDescriptor->gpuTexture || !gpuDescriptor->gpuSampler) {
                    CC_LOG_ERROR("Sampler binding '%s' at set %d binding %d index %d is not bounded",
                                 glSampler.name.c_str(), glSampler.set, glSampler.binding, u);
                    continue;
                }

                if (gpuDescriptor->gpuTexture->size > 0) {
                    GLuint glTexture = gpuDescriptor->gpuTexture->glTexture;
                    if (cache->glTextures[unit] != glTexture) {
                        if (cache->texUint != unit) {
                            GL_CHECK(glActiveTexture(GL_TEXTURE0 + unit));
                            cache->texUint = unit;
                        }
                        GL_CHECK(glBindTexture(gpuDescriptor->gpuTexture->glTarget, glTexture));
                        cache->glTextures[unit] = glTexture;
                    }

                    if (cache->glSamplers[unit] != gpuDescriptor->gpuSampler->glSampler) {
                        GL_CHECK(glBindSampler(unit, gpuDescriptor->gpuSampler->glSampler));
                        cache->glSamplers[unit] = gpuDescriptor->gpuSampler->glSampler;
                    }
                }
            }
        }
    }
}

void GLES3PrimaryCommandBuffer::bindInputAssembler(InputAssembler *ia) {
    GLES3Device *device = ((GLES3Device *)_device);
    GLES3GPUStateCache *cache = device->stateCache();
    GLES3ObjectCache &gfxStateCache = cache->gfxStateCache;

    GLES3GPUInputAssembler *gpuInputAssembler = ((GLES3InputAssembler *)ia)->gpuInputAssembler();
    GLES3GPUPipelineState *gpuPipelineState = gfxStateCache.gpuPipelineState;
    if (gpuInputAssembler && gpuPipelineState && gpuPipelineState->gpuShader &&
        (_isShaderChanged || gpuInputAssembler != gfxStateCache.gpuInputAssembler)) {
        gfxStateCache.gpuInputAssembler = gpuInputAssembler;
        _isShaderChanged = false;
        if (device->useVAO()) {
            GLuint hash = gpuPipelineState->gpuShader->glProgram ^ device->getThreadID();
            GLuint glVAO = gpuInputAssembler->glVAOs[hash];
            if (!glVAO) {
                GL_CHECK(glGenVertexArrays(1, &glVAO));
                gpuInputAssembler->glVAOs[hash] = glVAO;
                GL_CHECK(glBindVertexArray(glVAO));
                GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));
                GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));

                for (size_t j = 0; j < gpuPipelineState->gpuShader->glInputs.size(); ++j) {
                    const GLES3GPUInput &gpuInput = gpuPipelineState->gpuShader->glInputs[j];
                    for (size_t a = 0; a < gpuInputAssembler->attributes.size(); ++a) {
                        const GLES3GPUAttribute &gpuAttribute = gpuInputAssembler->glAttribs[a];
                        if (gpuAttribute.name == gpuInput.name) {
                            GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, gpuAttribute.glBuffer));

                            for (uint c = 0; c < gpuAttribute.componentCount; ++c) {
                                GLint glLoc = gpuInput.glLoc + c;
                                uint attribOffset = gpuAttribute.offset + gpuAttribute.size * c;
                                GL_CHECK(glEnableVertexAttribArray(glLoc));

                                cache->glEnabledAttribLocs[glLoc] = true;
                                GL_CHECK(glVertexAttribPointer(glLoc, gpuAttribute.count, gpuAttribute.glType, gpuAttribute.isNormalized, gpuAttribute.stride, reinterpret_cast<GLvoid*>(attribOffset)));
                                GL_CHECK(glVertexAttribDivisor(glLoc, gpuAttribute.isInstanced ? 1 : 0));
                            }
                            break;
                        }
                    }
                }

                if (gpuInputAssembler->gpuIndexBuffer) {
                    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpuInputAssembler->gpuIndexBuffer->glBuffer));
                }

                GL_CHECK(glBindVertexArray(0));
                GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));
                GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
                cache->glVAO = 0;
                cache->glArrayBuffer = 0;
                cache->glElementArrayBuffer = 0;
            }

            if (cache->glVAO != glVAO) {
                GL_CHECK(glBindVertexArray(glVAO));
                cache->glVAO = glVAO;
            }
        } else {
            for (uint a = 0; a < cache->glCurrentAttribLocs.size(); ++a) {
                cache->glCurrentAttribLocs[a] = false;
            }

            for (size_t j = 0; j < gpuPipelineState->gpuShader->glInputs.size(); ++j) {
                const GLES3GPUInput &gpuInput = gpuPipelineState->gpuShader->glInputs[j];
                for (size_t a = 0; a < gpuInputAssembler->attributes.size(); ++a) {
                    const GLES3GPUAttribute &gpuAttribute = gpuInputAssembler->glAttribs[a];
                    if (gpuAttribute.name == gpuInput.name) {
                        if (cache->glArrayBuffer != gpuAttribute.glBuffer) {
                            GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, gpuAttribute.glBuffer));
                            cache->glArrayBuffer = gpuAttribute.glBuffer;
                        }

                        for (uint c = 0; c < gpuAttribute.componentCount; ++c) {
                            GLint glLoc = gpuInput.glLoc + c;
                            uint attribOffset = gpuAttribute.offset + gpuAttribute.size * c;
                            GL_CHECK(glEnableVertexAttribArray(glLoc));
                            cache->glCurrentAttribLocs[glLoc] = true;
                            cache->glEnabledAttribLocs[glLoc] = true;
                            GL_CHECK(glVertexAttribPointer(glLoc, gpuAttribute.count, gpuAttribute.glType, gpuAttribute.isNormalized, gpuAttribute.stride, reinterpret_cast<GLvoid*>(attribOffset)));
                            GL_CHECK(glVertexAttribDivisor(glLoc, gpuAttribute.isInstanced ? 1 : 0));
                        }
                        break;
                    }
                }
            }

            if (gpuInputAssembler->gpuIndexBuffer) {
                if (cache->glElementArrayBuffer != gpuInputAssembler->gpuIndexBuffer->glBuffer) {
                    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpuInputAssembler->gpuIndexBuffer->glBuffer));
                    cache->glElementArrayBuffer = gpuInputAssembler->gpuIndexBuffer->glBuffer;
                }
            }

            for (uint a = 0; a < cache->glCurrentAttribLocs.size(); ++a) {
                if (cache->glEnabledAttribLocs[a] != cache->glCurrentAttribLocs[a]) {
                    GL_CHECK(glDisableVertexAttribArray(a));
                    cache->glEnabledAttribLocs[a] = false;
                }
            }
        }
    }
}

void GLES3PrimaryCommandBuffer::setViewport(const Viewport &viewport) {
    GLES3Device *device = ((GLES3Device *)_device);
    GLES3GPUStateCache *cache = device->stateCache();

    if (cache->viewport != viewport) {
        GL_CHECK(glViewport(viewport.left, viewport.top, viewport.width, viewport.height));
        cache->viewport = viewport;
    }
}

void GLES3PrimaryCommandBuffer::setScissor(const Rect &scissor) {
    GLES3Device *device = ((GLES3Device *)_device);
    GLES3GPUStateCache *cache = device->stateCache();

    if (cache->scissor != scissor) {
        GL_CHECK(glScissor(scissor.x, scissor.y, scissor.width, scissor.height));
        cache->scissor = scissor;
    }
}

void GLES3PrimaryCommandBuffer::setLineWidth(const float lineWidth) {
    GLES3Device *device = ((GLES3Device *)_device);
    GLES3GPUStateCache *cache = device->stateCache();

    if (cache->rs.lineWidth != lineWidth) {
        GL_CHECK(glLineWidth(lineWidth));
        cache->rs.lineWidth = lineWidth;
    }
}

void GLES3PrimaryCommandBuffer::setDepthBias(float constant, float clamp, float slope) {
    GLES3Device *device = ((GLES3Device *)_device);
    GLES3GPUStateCache *cache = device->stateCache();

    if ((cache->rs.depthBias != constant) ||
        (cache->rs.depthBiasSlop != slope)) {
        GL_CHECK(glPolygonOffset(constant, slope));
        cache->rs.depthBias = constant;
        cache->rs.depthBiasSlop = slope;
    }
}

void GLES3PrimaryCommandBuffer::setBlendConstants(const Color &blendColor) {
    GLES3Device *device = ((GLES3Device *)_device);
    GLES3GPUStateCache *cache = device->stateCache();

    if ((cache->bs.blendColor.x != blendColor.x) ||
        (cache->bs.blendColor.y != blendColor.y) ||
        (cache->bs.blendColor.z != blendColor.z) ||
        (cache->bs.blendColor.w != blendColor.w)) {
        GL_CHECK(glBlendColor(blendColor.x,
                              blendColor.y,
                              blendColor.z,
                              blendColor.w));
        cache->bs.blendColor = blendColor;
    }
}

void GLES3PrimaryCommandBuffer::setDepthBound(float minBounds, float maxBounds) {
    // no-op
}

void GLES3PrimaryCommandBuffer::setStencilWriteMask(StencilFace face, uint writeMask) {
    GLES3Device *device = ((GLES3Device *)_device);
    GLES3GPUStateCache *cache = device->stateCache();

    switch (face) {
        case StencilFace::FRONT:
            if (cache->dss.stencilWriteMaskFront != writeMask) {
                GL_CHECK(glStencilMaskSeparate(GL_FRONT, writeMask));
                cache->dss.stencilWriteMaskFront = writeMask;
            }
            break;
        case StencilFace::BACK:
            if (cache->dss.stencilWriteMaskBack != writeMask) {
                GL_CHECK(glStencilMaskSeparate(GL_BACK, writeMask));
                cache->dss.stencilWriteMaskBack = writeMask;
            }
            break;
        case StencilFace::ALL:
            if ((cache->dss.stencilWriteMaskFront != writeMask) ||
                (cache->dss.stencilWriteMaskBack != writeMask)) {
                GL_CHECK(glStencilMask(writeMask));
                cache->dss.stencilWriteMaskFront = writeMask;
                cache->dss.stencilWriteMaskBack = writeMask;
            }
            break;
    }
}

void GLES3PrimaryCommandBuffer::setStencilCompareMask(StencilFace face, int refrence, uint compareMask) {
    GLES3Device *device = ((GLES3Device *)_device);
    GLES3GPUStateCache *cache = device->stateCache();

    switch (face) {
        case StencilFace::FRONT:
            if ((cache->dss.stencilRefFront != (uint)refrence) ||
                (cache->dss.stencilReadMaskFront != compareMask)) {
                GL_CHECK(glStencilFuncSeparate(GL_FRONT,
                                        GLES3_CMP_FUNCS[(int)cache->dss.stencilFuncFront],
                                        refrence,
                                        compareMask));
                cache->dss.stencilRefFront = refrence;
                cache->dss.stencilReadMaskFront = compareMask;
            }
            break;
        case StencilFace::BACK:
            if ((cache->dss.stencilRefBack != (uint)refrence) ||
                (cache->dss.stencilReadMaskBack != compareMask)) {
                GL_CHECK(glStencilFuncSeparate(GL_BACK,
                                        GLES3_CMP_FUNCS[(int)cache->dss.stencilFuncBack],
                                        refrence,
                                        compareMask));
                cache->dss.stencilRefBack = refrence;
                cache->dss.stencilReadMaskBack = compareMask;
            }
            break;
        case StencilFace::ALL:
            if ((cache->dss.stencilRefFront != (uint)refrence) ||
                (cache->dss.stencilReadMaskFront != compareMask) ||
                (cache->dss.stencilRefBack != (uint)refrence) ||
                (cache->dss.stencilReadMaskBack != compareMask)) {
                GL_CHECK(glStencilFuncSeparate(GL_FRONT,
                                        GLES3_CMP_FUNCS[(int)cache->dss.stencilFuncFront],
                                        refrence,
                                        compareMask));
                GL_CHECK(glStencilFuncSeparate(GL_BACK,
                                        GLES3_CMP_FUNCS[(int)cache->dss.stencilFuncBack],
                                        refrence,
                                        compareMask));
                cache->dss.stencilRefFront = refrence;
                cache->dss.stencilReadMaskFront = compareMask;
                cache->dss.stencilRefBack = refrence;
                cache->dss.stencilReadMaskBack = compareMask;
            }
            break;
    }
}

void GLES3PrimaryCommandBuffer::draw(InputAssembler *ia) {
    if ((_type == CommandBufferType::PRIMARY && _isInRenderPass) ||
        (_type == CommandBufferType::SECONDARY)) {

        DrawInfo drawInfo;
        ia->extractDrawInfo(drawInfo);
        GLES3CmdFuncDraw((GLES3Device *)_device, drawInfo);

        ++_numDrawCalls;
        _numInstances += ia->getInstanceCount();
        if (_curGPUPipelineState) {
            switch (_curGPUPipelineState->glPrimitive) {
                case GL_TRIANGLES: {
                    _numTriangles += ia->getIndexCount() / 3 * std::max(ia->getInstanceCount(), 1U);
                    break;
                }
                case GL_TRIANGLE_STRIP:
                case GL_TRIANGLE_FAN: {
                    _numTriangles += (ia->getIndexCount() - 2) * std::max(ia->getInstanceCount(), 1U);
                    break;
                }
                default:
                    break;
            }
        }
    } else {
        CC_LOG_ERROR("Command 'draw' must be recorded inside a render pass.");
    }
}

void GLES3PrimaryCommandBuffer::updateBuffer(Buffer *buff, const void *data, uint size, uint offset) {
    if ((_type == CommandBufferType::PRIMARY && !_isInRenderPass) ||
        (_type == CommandBufferType::SECONDARY)) {

        GLES3GPUBuffer *gpuBuffer = ((GLES3Buffer *)buff)->gpuBuffer();
        if (gpuBuffer) {
            GLES3CmdFuncUpdateBuffer((GLES3Device *)_device, gpuBuffer, data, offset, size);
        }
    } else {
        CC_LOG_ERROR("Command 'updateBuffer' must be recorded outside a render pass.");
    }
}

void GLES3PrimaryCommandBuffer::copyBuffersToTexture(const uint8_t *const *buffers, Texture *texture, const BufferTextureCopy *regions, uint count) {
    if ((_type == CommandBufferType::PRIMARY && !_isInRenderPass) ||
        (_type == CommandBufferType::SECONDARY)) {

        GLES3GPUTexture *gpuTexture = ((GLES3Texture *)texture)->gpuTexture();
        if (gpuTexture) {
            GLES3CmdFuncCopyBuffersToTexture((GLES3Device *)_device, buffers, gpuTexture, regions, count);
        }
    } else {
        CC_LOG_ERROR("Command 'copyBuffersToTexture' must be recorded outside a render pass.");
    }
}

void GLES3PrimaryCommandBuffer::execute(const CommandBuffer *const *cmdBuffs, uint32_t count) {
    for (uint i = 0; i < count; ++i) {
        GLES3PrimaryCommandBuffer *cmdBuff = (GLES3PrimaryCommandBuffer *)cmdBuffs[i];
        GLES3CmdFuncExecuteCmds((GLES3Device *)_device, cmdBuff->_cmdPackage);

        _numDrawCalls += cmdBuff->getNumDrawCalls();
        _numInstances += cmdBuff->getNumInstances();
        _numTriangles += cmdBuff->getNumTris();
    }
}

} // namespace gfx
} // namespace cc
