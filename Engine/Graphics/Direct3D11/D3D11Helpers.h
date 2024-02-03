#pragma once
#include "D3D11CommonHeaders.h"

#if PRIMAL_BUILD_D3D11

namespace primal::graphics::d3d11::d3dx {
constexpr struct {
	const D3D11_DEPTH_STENCIL_DESC disabled {
        0,                                                  //BOOL DepthEnable;
        D3D11_DEPTH_WRITE_MASK_ZERO,                        //D3D11_DEPTH_WRITE_MASK DepthWriteMask;
        D3D11_COMPARISON_LESS_EQUAL,                        //D3D11_COMPARISON_FUNC DepthFunc;
        0,                                                  //BOOL StencilEnable;
        0,                                                  //UINT8 StencilReadMask;
        0,                                                  //UINT8 StencilWriteMask;
        {},                                                 //D3D11_DEPTH_STENCILOP_DESC FrontFace;
        {}                                                  //D3D11_DEPTH_STENCILOP_DESC BackFace;
	};
    const D3D11_DEPTH_STENCIL_DESC enabled {
        1,                                                  //BOOL DepthEnable;
        D3D11_DEPTH_WRITE_MASK_ALL,                         //D3D11_DEPTH_WRITE_MASK DepthWriteMask;
        D3D11_COMPARISON_LESS_EQUAL,                        //D3D11_COMPARISON_FUNC DepthFunc;
        0,                                                  //BOOL StencilEnable;
        0,                                                  //UINT8 StencilReadMask;
        0,                                                  //UINT8 StencilWriteMask;
        {},                                                 //D3D11_DEPTH_STENCILOP_DESC FrontFace;
        {}                                                  //D3D11_DEPTH_STENCILOP_DESC BackFace;
    };
    const D3D11_DEPTH_STENCIL_DESC enabled_readonly {
        1,                                                  //BOOL DepthEnable;
        D3D11_DEPTH_WRITE_MASK_ZERO,                        //D3D11_DEPTH_WRITE_MASK DepthWriteMask;
        D3D11_COMPARISON_LESS_EQUAL,                        //D3D11_COMPARISON_FUNC DepthFunc;
        0,                                                  //BOOL StencilEnable;
        0,                                                  //UINT8 StencilReadMask;
        0,                                                  //UINT8 StencilWriteMask;
        {},                                                 //D3D11_DEPTH_STENCILOP_DESC FrontFace;
        {}                                                  //D3D11_DEPTH_STENCILOP_DESC BackFace;
    };
    const D3D11_DEPTH_STENCIL_DESC reversed{
        1,                                                  //BOOL DepthEnable;
        D3D11_DEPTH_WRITE_MASK_ALL,                         //D3D11_DEPTH_WRITE_MASK DepthWriteMask;
        D3D11_COMPARISON_GREATER_EQUAL,                     //D3D11_COMPARISON_FUNC DepthFunc;
        0,                                                  //BOOL StencilEnable;
        0,                                                  //UINT8 StencilReadMask;
        0,                                                  //UINT8 StencilWriteMask;
        {},                                                 //D3D11_DEPTH_STENCILOP_DESC FrontFace;
        {}                                                  //D3D11_DEPTH_STENCILOP_DESC BackFace;
    };
    const D3D11_DEPTH_STENCIL_DESC reversed_readonly{
        1,                                                  //BOOL DepthEnable;
        D3D11_DEPTH_WRITE_MASK_ZERO,                        //D3D11_DEPTH_WRITE_MASK DepthWriteMask;
        D3D11_COMPARISON_GREATER_EQUAL,                     //D3D11_COMPARISON_FUNC DepthFunc;
        0,                                                  //BOOL StencilEnable;
        0,                                                  //UINT8 StencilReadMask;
        0,                                                  //UINT8 StencilWriteMask;
        {},                                                 //D3D11_DEPTH_STENCILOP_DESC FrontFace;
        {}                                                  //D3D11_DEPTH_STENCILOP_DESC BackFace;
    };
} depth_state;

constexpr struct {
    const D3D11_RASTERIZER_DESC2 no_cull {
        D3D11_FILL_SOLID,                                  //D3D11_FILL_MODE FillMode;
        D3D11_CULL_NONE,                                   //D3D11_CULL_MODE CullMode;
        1,                                                 //BOOL FrontCounterClockwise;
        0,                                                 //INT DepthBias;
        0,                                                 //FLOAT DepthBiasClamp;
        0,                                                 //FLOAT SlopeScaledDepthBias;
        1,                                                 //BOOL DepthClipEnable;
        1,                                                 //BOOL ScissorEnable;
        0,                                                 //BOOL MultisampleEnable;
        0,                                                 //BOOL AntialiasedLineEnable;
        0,                                                 //UINT ForcedSampleCount;
        D3D11_CONSERVATIVE_RASTERIZATION_MODE_OFF          //D3D11_CONSERVATIVE_RASTERIZATION_MODE ConservativeRaster;
    };
    const D3D11_RASTERIZER_DESC2 backface_cull {
        D3D11_FILL_SOLID,                                  //D3D11_FILL_MODE FillMode;
        D3D11_CULL_BACK,                                   //D3D11_CULL_MODE CullMode;
        1,                                                 //BOOL FrontCounterClockwise;
        0,                                                 //INT DepthBias;
        0,                                                 //FLOAT DepthBiasClamp;
        0,                                                 //FLOAT SlopeScaledDepthBias;
        1,                                                 //BOOL DepthClipEnable;
        1,                                                 //BOOL ScissorEnable;
        0,                                                 //BOOL MultisampleEnable;
        0,                                                 //BOOL AntialiasedLineEnable;
        0,                                                 //UINT ForcedSampleCount;
        D3D11_CONSERVATIVE_RASTERIZATION_MODE_OFF          //D3D11_CONSERVATIVE_RASTERIZATION_MODE ConservativeRaster;
    };
    const D3D11_RASTERIZER_DESC2 frontface_cull{
        D3D11_FILL_SOLID,		    						//D3D11_FILL_MODE FillMode;
        D3D11_CULL_FRONT,								    //D3D11_CULL_MODE CullMode;
        1,													//BOOL FrontCounterClockwise;
        0,													//INT DepthBias;
        0,													//FLOAT DepthBiasClamp;
        0,													//FLOAT SlopeScaledDepthBias;
        1,													//BOOL DepthClipEnable;
        1,                                                  //BOOL ScissorEnable;
        0,													//BOOL MultisampleEnable;
        0,													//BOOL AntialiasedLineEnable;
        0,													//UINT ForcedSampleCount;
        D3D11_CONSERVATIVE_RASTERIZATION_MODE_OFF			//D3D11_CONSERVATIVE_RASTERIZATION_MODE ConservativeRaster;
    };
    const D3D11_RASTERIZER_DESC2 wireframe{
        D3D11_FILL_WIREFRAME,   							//D3D11_FILL_MODE FillMode;
        D3D11_CULL_NONE,								    //D3D11_CULL_MODE CullMode;
        1,													//BOOL FrontCounterClockwise;
        0,													//INT DepthBias;
        0,													//FLOAT DepthBiasClamp;
        0,													//FLOAT SlopeScaledDepthBias;
        1,													//BOOL DepthClipEnable;
        1,                                                  //BOOL ScissorEnable;
        0,													//BOOL MultisampleEnable;
        0,													//BOOL AntialiasedLineEnable;
        0,													//UINT ForcedSampleCount;
        D3D11_CONSERVATIVE_RASTERIZATION_MODE_OFF			//D3D11_CONSERVATIVE_RASTERIZATION_MODE ConservativeRaster;
    };
    const D3D11_RASTERIZER_DESC2 msaa4x{
        D3D11_FILL_SOLID,                                  //D3D11_FILL_MODE FillMode;
        D3D11_CULL_BACK,                                   //D3D11_CULL_MODE CullMode;
        1,                                                 //BOOL FrontCounterClockwise;
        0,                                                 //INT DepthBias;
        0,                                                 //FLOAT DepthBiasClamp;
        0,                                                 //FLOAT SlopeScaledDepthBias;
        1,                                                 //BOOL DepthClipEnable;
        1,                                                 //BOOL ScissorEnable;
        1,                                                 //BOOL MultisampleEnable;
        0,                                                 //BOOL AntialiasedLineEnable;
        4,                                                 //UINT ForcedSampleCount;
        D3D11_CONSERVATIVE_RASTERIZATION_MODE_OFF          //D3D11_CONSERVATIVE_RASTERIZATION_MODE ConservativeRaster;
    };
} rasterizer_state;

constexpr struct {
    const D3D11_SAMPLER_DESC standard {
        D3D11_FILTER_MIN_MAG_MIP_LINEAR,                    //D3D11_FILTER Filter;
        D3D11_TEXTURE_ADDRESS_WRAP,                         //D3D11_TEXTURE_ADDRESS_MODE AddressU;
        D3D11_TEXTURE_ADDRESS_WRAP,                         //D3D11_TEXTURE_ADDRESS_MODE AddressV;
        D3D11_TEXTURE_ADDRESS_WRAP,                         //D3D11_TEXTURE_ADDRESS_MODE AddressW;
        0.f,                                                //FLOAT MipLODBias;
        0,                                                  //UINT MaxAnisotropy;
        D3D11_COMPARISON_NEVER,                             //D3D11_COMPARISON_FUNC ComparisonFunc;
        1.f,                                                //FLOAT BorderColor[0];
        0.f,                                                //FLOAT BorderColor[1];
        1.f,                                                //FLOAT BorderColor[2];
        1.f,                                                //FLOAT BorderColor[3];
        0.f,                                                //FLOAT MinLOD;
        0.f                                                 //FLOAT MaxLOD;
    };
    const D3D11_SAMPLER_DESC debug {
        D3D11_FILTER_MIN_MAG_MIP_LINEAR,                    //D3D11_FILTER Filter;
        D3D11_TEXTURE_ADDRESS_BORDER,                       //D3D11_TEXTURE_ADDRESS_MODE AddressU;
        D3D11_TEXTURE_ADDRESS_BORDER,                       //D3D11_TEXTURE_ADDRESS_MODE AddressV;
        D3D11_TEXTURE_ADDRESS_BORDER,                       //D3D11_TEXTURE_ADDRESS_MODE AddressW;
        0.f,                                                //FLOAT MipLODBias;
        0,                                                  //UINT MaxAnisotropy;
        D3D11_COMPARISON_NEVER,                             //D3D11_COMPARISON_FUNC ComparisonFunc;
        1.f,                                                //FLOAT BorderColor[0];
        0.f,                                                //FLOAT BorderColor[1];
        1.f,                                                //FLOAT BorderColor[2];
        1.f,                                                //FLOAT BorderColor[3];
        0.f,                                                //FLOAT MinLOD;
        0.f                                                 //FLOAT MaxLOD;
    };
    const D3D11_SAMPLER_DESC quality {
        D3D11_FILTER_MAXIMUM_ANISOTROPIC,                   //D3D11_FILTER Filter;
        D3D11_TEXTURE_ADDRESS_WRAP,                         //D3D11_TEXTURE_ADDRESS_MODE AddressU;
        D3D11_TEXTURE_ADDRESS_WRAP,                         //D3D11_TEXTURE_ADDRESS_MODE AddressV;
        D3D11_TEXTURE_ADDRESS_WRAP,                         //D3D11_TEXTURE_ADDRESS_MODE AddressW;
        0.f,                                                //FLOAT MipLODBias;
        0,                                                  //UINT MaxAnisotropy;
        D3D11_COMPARISON_NEVER,                             //D3D11_COMPARISON_FUNC ComparisonFunc;
        1.f,                                                //FLOAT BorderColor[0];
        0.f,                                                //FLOAT BorderColor[1];
        1.f,                                                //FLOAT BorderColor[2];
        1.f,                                                //FLOAT BorderColor[3];
        0.f,                                                //FLOAT MinLOD;
        0.f                                                 //FLOAT MaxLOD;
    };
} sampler_state;

constexpr struct {
    const D3D11_BLEND_DESC1 disabled {
        0,                                                  //BOOL AlphaToCoverageEnable;
        0,                                                  //BOOL IndependentBlendEnable;
        {
            {
                0,                                          //BOOL BlendEnable;
                0,                                          //BOOL LogicOpEnable;
                D3D11_BLEND_SRC_ALPHA,                      //D3D11_BLEND SrcBlend;
                D3D11_BLEND_INV_DEST_ALPHA,                 //D3D11_BLEND DestBlend;
                D3D11_BLEND_OP_ADD,                         //D3D11_BLEND_OP BlendOp;
                D3D11_BLEND_ONE,                            //D3D11_BLEND SrcBlendAlpha;
                D3D11_BLEND_ONE,                            //D3D11_BLEND DestBlendAlpha;
                D3D11_BLEND_OP_ADD,                         //D3D11_BLEND_OP BlendOpAlpha;
                D3D11_LOGIC_OP_NOOP,                        //D3D11_LOGIC_OP LogicOp;
                D3D11_COLOR_WRITE_ENABLE_ALL                //UINT8 RenderTargetWriteMask;
            },
            {}, {}, {}, {}, {}, {}, {}
        }
    };
    const D3D11_BLEND_DESC1 additive{
        0,                                                  //BOOL AlphaToCoverageEnable;
        0,                                                  //BOOL IndependentBlendEnable;
        {
            {
                1,                                          //BOOL BlendEnable;
                0,                                          //BOOL LogicOpEnable;
                D3D11_BLEND_ONE,                            //D3D11_BLEND SrcBlend;
                D3D11_BLEND_ONE,                            //D3D11_BLEND DestBlend;
                D3D11_BLEND_OP_ADD,                         //D3D11_BLEND_OP BlendOp;
                D3D11_BLEND_ONE,                            //D3D11_BLEND SrcBlendAlpha;
                D3D11_BLEND_ONE,                            //D3D11_BLEND DestBlendAlpha;
                D3D11_BLEND_OP_ADD,                         //D3D11_BLEND_OP BlendOpAlpha;
                D3D11_LOGIC_OP_NOOP,                        //D3D11_LOGIC_OP LogicOp;
                D3D11_COLOR_WRITE_ENABLE_ALL                //UINT8 RenderTargetWriteMask;
            },
            {}, {}, {}, {}, {}, {}, {}
        }
    };
} blend_state;

constexpr u64
align_size_for_constant_buffer(u64 size)
{
    return math::align_size_up<PRIMAL_D3D11_CONSTANT_BUFFER_ALIGNMENT>(size);
}

constexpr u32
align_size_for_constant_buffer_offset(u64 size)
{
    return (u32)math::align_size_up<PRIMAL_D3D11_CONSTANT_BUFFER_ALIGNMENT>(size) / 16;
}
}

#endif
