#include "Fractals.hlsli"
#include "Common.hlsli"

cbuffer ShaderConstants : register(b0)
{
    GlobalShaderData ShaderConstants;
};

#define SAMPLES 4

float4 FillColorPS(in noperspective float4 Position : SV_Position, in noperspective float2 UV : TEXCOORD) : SV_Target0
{   
    const float offset = 0.2f;
    const float2 offsets[4] =
    {
        float2(-offset, offset),
        float2(offset, offset),
        float2(offset, -offset),
        float2(-offset, -offset)
    };
    const float2 invDim = float2(1.f / ShaderConstants.ViewWidth, 1.f / ShaderConstants.ViewHeight);
    float3 color = 0.f;
    for (int i = 0; i < SAMPLES; ++i)
    {
        const float2 uv = (Position.xy + offsets[i]) * invDim;
        //color += DrawMandelbrot(uv);
        color += DrawJuliaSet(uv, 0x00);
    }
    
    return float4(float3(color.z, color.x, 1.f) * color.x / SAMPLES, 1.f);
}