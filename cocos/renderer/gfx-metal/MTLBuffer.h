#pragma once

#import <Metal/MTLBuffer.h>
#import <Metal/MTLStageInputOutputDescriptor.h>

NS_CC_BEGIN

class CCMTLBuffer: public GFXBuffer
{
public:
    CCMTLBuffer(GFXDevice* device);
    ~CCMTLBuffer();
    
    virtual bool initialize(const GFXBufferInfo& info) override;
    virtual void destroy() override;
    virtual void resize(uint size) override;
    virtual void update(void* buffer, uint offset, uint size) override;
    
    CC_INLINE id<MTLBuffer> getMTLBuffer() const { return _mtlBuffer; }
    CC_INLINE uint8_t* getTransferBuffer() const { return _transferBuffer; }
    CC_INLINE MTLIndexType getIndexType() const { return _indexType; }
    CC_INLINE const GFXDrawInfoList& getIndirects() const { return _indirects; }
    CC_INLINE uint8_t* getBytes() const { return _bytes; }
    
private:
    void resizeBuffer(uint8_t**, uint, uint);
    
    id<MTLBuffer> _mtlBuffer = nullptr;
    uint8_t* _transferBuffer = nullptr;
    MTLIndexType _indexType = MTLIndexTypeUInt16;
    MTLResourceOptions _mtlResourceOptions = MTLResourceStorageModePrivate;
    GFXDrawInfoList _indirects;
    
    // to store vertex and ubo data when size is small, otherwise use MTLBuffer instead.
    uint8_t* _bytes = nullptr;
};

NS_CC_END
