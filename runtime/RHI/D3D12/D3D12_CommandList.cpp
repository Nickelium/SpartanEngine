/*
Copyright(c) 2016-2023 Panos Karabelas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

//= INCLUDES =========================
#include "pch.h"
#include "../RHI_Implementation.h"
#include "../RHI_CommandList.h"
#include "../RHI_Pipeline.h"
#include "../RHI_Device.h"
#include "../RHI_Sampler.h"
#include "../RHI_Texture.h"
#include "../RHI_Shader.h"
#include "../RHI_ConstantBuffer.h"
#include "../RHI_VertexBuffer.h"
#include "../RHI_IndexBuffer.h"
#include "../RHI_BlendState.h"
#include "../RHI_DepthStencilState.h"
#include "../RHI_RasterizerState.h"
#include "../RHI_InputLayout.h"
#include "../RHI_SwapChain.h"
#include "../RHI_PipelineState.h"
#include "../../Profiling/Profiler.h"
#include "../../Rendering/Renderer.h"
//====================================

//= NAMESPACES ===============
using namespace std;
using namespace Spartan::Math;
//============================

namespace Spartan
{
    RHI_CommandList::RHI_CommandList(const RHI_Queue_Type queue_type, const uint64_t swapchain_index, void* cmd_pool, const char* name)
    {
        SP_ASSERT(cmd_pool != nullptr);

        m_queue_type            = queue_type;
        m_object_name           = name;
        m_rhi_cmd_pool_resource = cmd_pool;
        m_timestamps.fill(0);

        // Created command list
        SP_ASSERT_MSG(
            d3d12_utility::error::check(
                RHI_Context::device->CreateCommandList(
                    0,
                    D3D12_COMMAND_LIST_TYPE_DIRECT,
                    static_cast<ID3D12CommandAllocator*>(cmd_pool),
                    nullptr,
                    IID_PPV_ARGS(reinterpret_cast<ID3D12GraphicsCommandList**>(&m_rhi_resource))
                )
            ), "Failed to create command list"
        );
    }

    RHI_CommandList::~RHI_CommandList()
    {
        // Wait in case it's still in use by the GPU
        RHI_Device::QueueWaitAll();

        // Command list
        d3d12_utility::release<ID3D12CommandQueue>(m_rhi_resource);
    }

    void RHI_CommandList::Begin()
    {
        // If the command list is in use, wait for it
        if (m_state == RHI_CommandListState::Submitted)
        {
            WaitForExecution();
        }

        // Validate a few things
        SP_ASSERT(m_rhi_resource != nullptr);
        SP_ASSERT(m_state == RHI_CommandListState::Idle);

        // Unlike Vulkan, D3D12 wraps both begin and reset under Reset().
        SP_ASSERT_MSG(d3d12_utility::error::check(static_cast<ID3D12GraphicsCommandList*>(m_rhi_resource)->Reset(
            static_cast<ID3D12CommandAllocator*>(m_rhi_cmd_pool_resource), nullptr)),
            "Failed to reset command list");

        m_state = RHI_CommandListState::Recording;
    }

    void RHI_CommandList::End()
    {
        // Verify a few things
        SP_ASSERT(m_rhi_resource != nullptr);
        SP_ASSERT(m_state == RHI_CommandListState::Recording);

        SP_ASSERT_MSG(SUCCEEDED(static_cast<ID3D12GraphicsCommandList*>(m_rhi_resource)->Close()), "Failed to end command list");

        m_state = RHI_CommandListState::Ended;
    }

    void RHI_CommandList::Submit()
    {

    }

    void RHI_CommandList::SetPipelineState(RHI_PipelineState& pso)
    {
        SP_ASSERT(pso.IsValid() && "Pipeline state is invalid");
        SP_ASSERT(m_state == RHI_CommandListState::Recording);

        SP_ASSERT_MSG(false, "Function is not implemented");
    }

    void RHI_CommandList::BeginRenderPass()
    {
        SP_ASSERT_MSG(false, "Function is not implemented");
    }
    
    void RHI_CommandList::EndRenderPass()
    {
        SP_ASSERT_MSG(false, "Function is not implemented");
    }

    void RHI_CommandList::ClearPipelineStateRenderTargets(RHI_PipelineState& pipeline_state)
    {
        SP_ASSERT_MSG(false, "Function is not implemented");
    }

    void RHI_CommandList::ClearRenderTarget(RHI_Texture* texture,
        const uint32_t color_index         /*= 0*/,
        const uint32_t depth_stencil_index /*= 0*/,
        const bool storage                 /*= false*/,
        const Color& clear_color           /*= rhi_color_load*/,
        const float clear_depth            /*= rhi_depth_load*/,
        const uint32_t clear_stencil       /*= rhi_stencil_load*/
    )
    {
        SP_ASSERT_MSG(false, "Function is not implemented");
    }

    void RHI_CommandList::Draw(const uint32_t vertex_count, uint32_t vertex_start_index /*= 0*/)
    {
        // Validate command list state
        SP_ASSERT(m_state == RHI_CommandListState::Recording);

        // Ensure correct state before attempting to draw
        OnDraw();

        // Draw
        static_cast<ID3D12GraphicsCommandList*>(m_rhi_resource)->DrawInstanced(
            vertex_count,       // VertexCountPerInstance
            1,                  // InstanceCount
            vertex_start_index, // StartVertexLocation
            0                   // StartInstanceLocation
        );

        // Profiler
        Profiler::m_rhi_draw++;
    }
    
    void RHI_CommandList::DrawIndexed(const uint32_t index_count, const uint32_t index_offset, const uint32_t vertex_offset, const uint32_t instance_count)
    {
        // Validate command list state
        SP_ASSERT(m_state == RHI_CommandListState::Recording);

        // Ensure correct state before attempting to draw
        OnDraw();

        // Draw
        static_cast<ID3D12GraphicsCommandList*>(m_rhi_resource)->DrawIndexedInstanced(
            index_count,   // IndexCountPerInstance
            1,             // InstanceCount
            index_offset,  // StartIndexLocation
            vertex_offset, // BaseVertexLocation
            0              // StartInstanceLocation
        );

        // Profile
        Profiler::m_rhi_draw++;
    }
  
    void RHI_CommandList::Dispatch(uint32_t x, uint32_t y, uint32_t z, bool async /*= false*/)
    {
        // Validate command list state
        SP_ASSERT(m_state == RHI_CommandListState::Recording);

        // Ensure correct state before attempting to draw
        OnDraw();

        // Dispatch
        static_cast<ID3D12GraphicsCommandList*>(m_rhi_resource)->Dispatch(x, y, z);

        // Profiler
        Profiler::m_rhi_dispatch++;
    }

    void RHI_CommandList::Blit(RHI_Texture* source, RHI_Texture* destination, const bool blit_mips)
    {
        SP_ASSERT_MSG(false, "Function is not implemented");
    }

    void RHI_CommandList::Blit(RHI_Texture* source, RHI_SwapChain* destination)
    {
        SP_ASSERT_MSG((source->GetFlags() & RHI_Texture_ClearBlit) != 0, "The texture needs the RHI_Texture_ClearOrBlit flag");
        SP_ASSERT_MSG(source->GetWidth() <= destination->GetWidth() && source->GetHeight() <= destination->GetHeight(),
            "The source texture dimension(s) are larger than the those of the destination texture");
    }

    void RHI_CommandList::Copy(RHI_Texture* source, RHI_Texture* destination, const bool blit_mips)
    {
        SP_ASSERT_MSG(false, "Function is not implemented");
    }

    void RHI_CommandList::Copy(RHI_Texture* source, RHI_SwapChain* destination)
    {
        SP_ASSERT_MSG((source->GetFlags() & RHI_Texture_ClearBlit) != 0, "The texture needs the RHI_Texture_ClearOrBlit flag");
        SP_ASSERT(source->GetWidth() == destination->GetWidth());
        SP_ASSERT(source->GetHeight() == destination->GetHeight());
    }

    void RHI_CommandList::SetViewport(const RHI_Viewport& viewport) const
    {
        // Validate command list state
        SP_ASSERT(m_state == RHI_CommandListState::Recording);

        D3D12_VIEWPORT d3d12_viewport = {};
        d3d12_viewport.TopLeftX       = viewport.x;
        d3d12_viewport.TopLeftY       = viewport.y;
        d3d12_viewport.Width          = viewport.width;
        d3d12_viewport.Height         = viewport.height;
        d3d12_viewport.MinDepth       = viewport.depth_min;
        d3d12_viewport.MaxDepth       = viewport.depth_max;

        static_cast<ID3D12GraphicsCommandList*>(m_rhi_resource)->RSSetViewports(1, &d3d12_viewport);
    }
    
    void RHI_CommandList::SetScissorRectangle(const Math::Rectangle& scissor_rectangle) const
    {
        // Validate command list state
        SP_ASSERT(m_state == RHI_CommandListState::Recording);

        const D3D12_RECT d3d12_rectangle =
        {
            static_cast<LONG>(scissor_rectangle.left),
            static_cast<LONG>(scissor_rectangle.top),
            static_cast<LONG>(scissor_rectangle.right),
            static_cast<LONG>(scissor_rectangle.bottom)
        };

        static_cast<ID3D12GraphicsCommandList*>(m_rhi_resource)->RSSetScissorRects(1, &d3d12_rectangle);
    }
    
    void RHI_CommandList::SetBufferVertex(const RHI_VertexBuffer* buffer, const uint32_t binding /*= 0*/)
    {
        // Validate command list state
        SP_ASSERT(m_state == RHI_CommandListState::Recording);

        // Skip if already set
        if (m_vertex_buffer_id == buffer->GetObjectId())
            return;

        D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view = {};
        vertex_buffer_view.BufferLocation           = 0;
        vertex_buffer_view.StrideInBytes            = static_cast<UINT>(buffer->GetStride());
        vertex_buffer_view.SizeInBytes              = static_cast<UINT>(buffer->GetObjectSizeGpu());

        static_cast<ID3D12GraphicsCommandList*>(m_rhi_resource)->IASetVertexBuffers(
            0,                  // StartSlot
            1,                  // NumViews
            &vertex_buffer_view // pViews
        );

        m_vertex_buffer_id = buffer->GetObjectId();

        Profiler::m_rhi_bindings_buffer_vertex++;
    }
    
    void RHI_CommandList::SetBufferIndex(const RHI_IndexBuffer* buffer)
    {
        // Validate command list state
        SP_ASSERT(m_state == RHI_CommandListState::Recording);

        // Skip if already set
        if (m_index_buffer_id == buffer->GetObjectId())
            return;

        D3D12_INDEX_BUFFER_VIEW index_buffer_view = {};
        index_buffer_view.BufferLocation          = 0;
        index_buffer_view.SizeInBytes             = static_cast<UINT>(buffer->GetObjectSizeGpu());
        index_buffer_view.Format                  = buffer->Is16Bit() ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;

        static_cast<ID3D12GraphicsCommandList*>(m_rhi_resource)->IASetIndexBuffer(
            &index_buffer_view // pView
        );

        m_index_buffer_id = buffer->GetObjectId();

        Profiler::m_rhi_bindings_buffer_index++;
    }
    
    void RHI_CommandList::SetConstantBuffer(const uint32_t slot, RHI_ConstantBuffer* constant_buffer) const
    {
        SP_ASSERT_MSG(false, "Function is not implemented");
    }

    void RHI_CommandList::PushConstants(const uint32_t offset, const uint32_t size, const void* data)
    {
        SP_ASSERT_MSG(false, "Function is not implemented");
    }

    void RHI_CommandList::SetStructuredBuffer(const uint32_t slot, RHI_StructuredBuffer* structured_buffer) const
    {
        SP_ASSERT_MSG(false, "Function is not implemented");
    }
    
    void RHI_CommandList::SetSampler(const uint32_t slot, RHI_Sampler* sampler) const
    {
        SP_ASSERT_MSG(false, "Function is not implemented");
    }

    void RHI_CommandList::SetTexture(const uint32_t slot, RHI_Texture* texture, const uint32_t mip_index /*= rhi_all_mips*/, uint32_t mip_range /*= 0*/, const bool uav /*= false*/)
    {
        SP_ASSERT_MSG(false, "Function is not implemented");
    }

    uint32_t RHI_CommandList::BeginTimestamp()
    {
        SP_ASSERT_MSG(false, "Function is not implemented");
        return 0;
    }

    void RHI_CommandList::EndTimestamp()
    {
        SP_ASSERT_MSG(false, "Function is not implemented");
    }

    float RHI_CommandList::GetTimestampDuration(const uint32_t timestamp_index)
    {
        return 0.0f;
    }

    void RHI_CommandList::BeginTimeblock(const char* name, const bool gpu_marker, const bool gpu_timing)
    {
        SP_ASSERT_MSG(false, "Function is not implemented");
    }

    void RHI_CommandList::EndTimeblock()
    {
        SP_ASSERT_MSG(false, "Function is not implemented");
    }

    void RHI_CommandList::BeginMarker(const char* name)
    {
        SP_ASSERT_MSG(false, "Function is not implemented");
    }

    void RHI_CommandList::EndMarker()
    {
        SP_ASSERT_MSG(false, "Function is not implemented");
    }

    void RHI_CommandList::OnDraw()
    {
        SP_ASSERT_MSG(false, "Function is not implemented");
    }

    void RHI_CommandList::InsertMemoryBarrierImage(void* image, const uint32_t aspect_mask,
        const uint32_t mip_index, const uint32_t mip_range, const uint32_t array_length,
        const RHI_Image_Layout layout_old, const RHI_Image_Layout layout_new
    )
    {

    }

    void RHI_CommandList::InsertMemoryBarrierImage(RHI_Texture* texture, const uint32_t mip_start, const uint32_t mip_range, const uint32_t array_length, const RHI_Image_Layout layout_old, const RHI_Image_Layout layout_new)
    {
       
    }

    void RHI_CommandList::InsertMemoryBarrierImageWaitForWrite(RHI_Texture* texture)
    {
        
    }
}
