#include "Common.hlsli"

cbuffer b00 : register(b0) { GlobalShaderData GlobalData; };
cbuffer b01 : register(b1) { LightCullingDispatchParameters ShaderParams; };
RWStructuredBuffer<Frustum>                         Frustums        :   register(u0);

#if USE_BOUNDING_SPHERES

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

#else

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void GridFrustumsCS(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    const uint x = DispatchThreadID.x;
    const uint y = DispatchThreadID.y;

    if (x >= ShaderParams.NumThreads.x || y >= ShaderParams.NumThreads.y) return;

    float4 screenSpace[4];
    screenSpace[0] = float4(float2(x, y) * TILE_SIZE, 0.f, 1.f);
    screenSpace[1] = float4(float2(x + 1, y) * TILE_SIZE, 0.f, 1.f);
    screenSpace[2] = float4(float2(x, y + 1) * TILE_SIZE, 0.f, 1.f);
    screenSpace[3] = float4(float2(x + 1, y + 1) * TILE_SIZE, 0.f, 1.f);
    
    const float2 invViewDimensions = 1.f / float2(GlobalData.ViewWidth, GlobalData.ViewHeight);
    float3 viewSpace[4];
    
    viewSpace[0] = ScreenToView(screenSpace[0], invViewDimensions, GlobalData.InvProjection).xyz;
    viewSpace[1] = ScreenToView(screenSpace[1], invViewDimensions, GlobalData.InvProjection).xyz;
    viewSpace[2] = ScreenToView(screenSpace[2], invViewDimensions, GlobalData.InvProjection).xyz;
    viewSpace[3] = ScreenToView(screenSpace[3], invViewDimensions, GlobalData.InvProjection).xyz;
    
    const float3 eyePos = (float3)0;
    Frustum frustum;
    frustum.Planes[0] = ComputePlane(viewSpace[0], eyePos, viewSpace[2]);
    frustum.Planes[1] = ComputePlane(viewSpace[3], eyePos, viewSpace[1]);
    frustum.Planes[2] = ComputePlane(viewSpace[1], eyePos, viewSpace[0]);
    frustum.Planes[3] = ComputePlane(viewSpace[2], eyePos, viewSpace[3]);
    
    Frustums[x + (y * ShaderParams.NumThreads.x)] = frustum;
}

#endif
