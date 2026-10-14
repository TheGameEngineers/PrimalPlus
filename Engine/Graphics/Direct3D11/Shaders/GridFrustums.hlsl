#include "Common.hlsli"

cbuffer b00 : register(b0) { GlobalShaderData GlobalData; };
cbuffer b01 : register(b1) { LightCullingDispatchParameters ShaderParams; };
RWStructuredBuffer<Frustum>                         Frustums        :   register(u0);

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void GridFrustumsCS(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    if (DispatchThreadID.x >= ShaderParams.NumThreads.x || DispatchThreadID.y >= ShaderParams.NumThreads.y) return;
    const float2 invViewDimensions = TILE_SIZE / float2(GlobalData.ViewWidth, GlobalData.ViewHeight);
    const float2 topLeft = DispatchThreadID.xy * invViewDimensions;
    const float2 center = topLeft + (invViewDimensions * 0.5f);
    
    float3 topLeftVS = UnProjectUV(topLeft, 0, GlobalData.InvProjection).xyz;
    float3 centerVS = UnProjectUV(center, 0, GlobalData.InvProjection).xyz;
    
    const float farClipRcp = -GlobalData.InvProjection._m33;
    Frustum frustum = { normalize(centerVS), distance(centerVS, topLeftVS) * farClipRcp };
    Frustums[DispatchThreadID.x + (DispatchThreadID.y  * ShaderParams.NumThreads.x)] = frustum;
}
