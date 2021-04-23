/*
** Copyright (c) 2021 LunarG, Inc.
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to
** deal in the Software without restriction, including without limitation the
** rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
** sell copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
** FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
** IN THE SOFTWARE.
*/

#include "encode/custom_dx12_struct_unwrappers.h"

#include "encode/dx12_object_wrapper_util.h"
#include "format/dx12_subobject_types.h"
#include "generated/generated_dx12_struct_unwrappers.h"
#include "generated/generated_dx12_wrappers.h"
#include "util/defines.h"
#include "util/logging.h"

GFXRECON_BEGIN_NAMESPACE(gfxrecon)
GFXRECON_BEGIN_NAMESPACE(encode)

void UnwrapStructObjects(D3D12_RESOURCE_BARRIER* value, HandleUnwrapMemory* unwrap_memory)
{
    if (value != nullptr)
    {
        switch (value->Type)
        {
            case D3D12_RESOURCE_BARRIER_TYPE_TRANSITION:
                UnwrapStructObjects(&value->Transition, unwrap_memory);
                break;
            case D3D12_RESOURCE_BARRIER_TYPE_ALIASING:
                UnwrapStructObjects(&value->Aliasing, unwrap_memory);
                break;
            case D3D12_RESOURCE_BARRIER_TYPE_UAV:
                UnwrapStructObjects(&value->UAV, unwrap_memory);
                break;
            default:
                break;
        }
    }
}

void UnwrapStructObjects(D3D12_TEXTURE_COPY_LOCATION* value, HandleUnwrapMemory* unwrap_memory)
{
    GFXRECON_UNREFERENCED_PARAMETER(unwrap_memory);

    if (value != nullptr)
    {
        value->pResource = encode::GetWrappedObject<ID3D12Resource>(value->pResource);
    }
}

void UnwrapStructObjects(D3D12_RENDER_PASS_ENDING_ACCESS* value, HandleUnwrapMemory* unwrap_memory)
{
    if (value != nullptr)
    {
        switch (value->Type)
        {
            case D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE:
                UnwrapStructObjects(&value->Resolve, unwrap_memory);
            default:
                break;
        }
    }
}

void UnwrapStructObjects(D3D12_PIPELINE_STATE_STREAM_DESC* value, HandleUnwrapMemory* unwrap_memory)
{
    if ((value != nullptr) && (value->SizeInBytes > 0) && (value->pPipelineStateSubobjectStream != nullptr))
    {
        // Allocate memory for and copy the subobject stream prior to modifying its contents.
        auto start = reinterpret_cast<uint8_t*>(unwrap_memory->GetFilledBuffer(
            reinterpret_cast<uint8_t*>(value->pPipelineStateSubobjectStream), value->SizeInBytes));

        value->pPipelineStateSubobjectStream = start;

        size_t offset = 0;
        while (offset < value->SizeInBytes)
        {
            auto current = start + offset;
            auto type    = *reinterpret_cast<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE*>(current);

            if (type == D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE)
            {
                // The only subobject in the stream that contains a handle to be unwrapped is the root signature
                // subobject, so we can unwrap it and then break from the loop.  There will only be one root signature
                // per-stream, so there is no need to continue processing the stream after it has been unwrapped.
                auto subobject   = reinterpret_cast<format::Dx12SignatureSubobject*>(current);
                subobject->value = encode::GetWrappedObject<ID3D12RootSignature>(subobject->value);
                offset += sizeof(*subobject);
                break;
            }

            // Only the root signature subobject requires object unwrapping.  The rest of the subobjects only require
            // an offset increment.
            switch (type)
            {
                case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS:
                case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS:
                case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS:
                case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS:
                case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS:
                case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS:
                case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS:
                case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS:
                    offset += sizeof(format::Dx12ShaderBytecodeSubobject);
                    break;
                case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_STREAM_OUTPUT:
                    offset += sizeof(format::Dx12StreamOutputSubobject);
                    break;
                case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND:
                    offset += sizeof(format::Dx12BlendSubobject);
                    break;
                case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK:
                case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_NODE_MASK:
                    offset += sizeof(format::Dx12UIntSubobject);
                    break;
                case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER:
                    offset += sizeof(format::Dx12RasterizerSubobject);
                    break;
                case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL:
                    offset += sizeof(format::Dx12DepthStencilSubobject);
                    break;
                case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT:
                    offset += sizeof(format::Dx12InputLayoutSubobject);
                    break;
                case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_IB_STRIP_CUT_VALUE:
                    offset += sizeof(format::Dx12StripCutSubobject);
                    break;
                case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY:
                    offset += sizeof(format::Dx12PrimitiveTopologySubobject);
                    break;
                case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS:
                    offset += sizeof(format::Dx12RenderTargetFormatsSubobject);
                    break;
                case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT:
                    offset += sizeof(format::Dx12FormatSubobject);
                    break;
                case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC:
                    offset += sizeof(format::Dx12SampleDescSubobject);
                    break;
                case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CACHED_PSO:
                    offset += sizeof(format::Dx12CachedPsoSubobject);
                    break;
                case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_FLAGS:
                    offset += sizeof(format::Dx12TypeFlagsSubobject);
                    break;
                case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL1:
                    offset += sizeof(format::Dx12DepthStencil1Subobject);
                    break;
                case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING:
                    offset += sizeof(format::Dx12ViewInstancingSubobject);
                    break;
                default:
                    // Type is unrecognized.  Write an invalid enum value so the decoder know the data is incomplete,
                    // and log a warning.
                    offset = value->SizeInBytes;

                    GFXRECON_LOG_WARNING(
                        "A pipeline state subobject with unrecognized D3D12_PIPELINE_STATE_SUBOBJECT_TYPE = %d was "
                        "omitted from handle unwrapping, which may cause capture to fail.",
                        type);

                    break;
            }
        }
    }
}

GFXRECON_END_NAMESPACE(encode)
GFXRECON_END_NAMESPACE(gfxrecon)
