#include "Common.hlsli"

cbuffer b00 : register(b0)
{
    GlobalShaderData GlobalData;
};
Texture2D GPassMain                     : register(t0);
Texture2D GPassDepth                    : register(t1);
StructuredBuffer<Frustum> Frustums      : register(t2);
StructuredBuffer<uint2> LightGridOpaque : register(t3);

TextureCube                                     AmbientDiffuse          :   register(t12);//NOT USED
TextureCube                                     AmbientSpecular         :   register(t13);
Texture2D                                       BrdfLut                 :   register(t14);//NOT USED

SamplerState                                    PointSampler            :   register(s0);
SamplerState                                    LinearSampler           :   register(s1);
SamplerState                                    AnisotropicSampler      :   register(s2);

uint GetGridIndex(float2 uv, float width)
{
    const uint2 pos = uint2(uv);
    const uint tileX = ceil(width / TILE_SIZE);
    return (pos.x / TILE_SIZE) + (tileX * (pos.y / TILE_SIZE));
}

float4 HeatMap(StructuredBuffer<uint2> buffer, float2 posXY, float blend)
{
    const float w = GlobalData.ViewWidth;
    const uint gridIndex = GetGridIndex(posXY, w);
    uint numLights = buffer[gridIndex].y;
#if USE_BOUNDING_SPHERES
    const uint numPointLights = numLights >> 16;
    const uint numSpotLights = numLights & 0xffff;
    numLights = numPointLights + numSpotLights;
#endif
    
    const float3 mapTex[] =
    {
        float3(0, 0, 0),
        float3(0, 0, 1),
        float3(0, 1, 1),
        float3(0, 1, 0),
        float3(1, 1, 0),
        float3(1, 0, 0)
    };
    
    const uint mapTexLen = 5;
    const uint maxHeat = 40;
    float l = saturate((float) numLights / maxHeat) * mapTexLen;
    float3 a = mapTex[floor(l)];
    float3 b = mapTex[ceil(l)];
    float3 heatMap = lerp(a, b, l - floor(l));
    if (numLights >= 512)
    {
        return float4(0.5f, 0.5f, 0.5f, 0.5f);
    }
    return float4(lerp(GPassMain[posXY].xyz, heatMap, blend), 1.f);
}

float4 PostProcessPS(in noperspective float4 Position : SV_Position, in noperspective float2 UV : TEXCOORD) : SV_Target0
{
#if 0
    const float w = GlobalData.ViewWidth;
    const uint gridIndex = GetGridIndex(Position.xy, w);
    const Frustum f = Frustums[gridIndex];
#if USE_BOUNDING_SPHERES    
    float3 color = abs(f.ConeDirection);
#else
    const uint halfTile = TILE_SIZE / 2;
    float3 color = abs(f.Planes[1].Normal);
    
    if (GetGridIndex(float2(Position.x + halfTile, Position.y), w) == gridIndex && GetGridIndex(float2(Position.x, Position.y + halfTile), w)  == gridIndex)
    {
        color = abs(f.Planes[0].Normal);
    }
    else if (GetGridIndex(float2(Position.x + halfTile, Position.y), w) != gridIndex && GetGridIndex(float2(Position.x, Position.y + halfTile), w)  == gridIndex)
    {
        color = abs(f.Planes[2].Normal);
    }
    else if (GetGridIndex(float2(Position.x + halfTile, Position.y), w) == gridIndex && GetGridIndex(float2(Position.x, Position.y + halfTile), w)  != gridIndex)
    {
        color = abs(f.Planes[3].Normal);
    }
#endif
    color = lerp(GPassMain[Position.xy].xyz, color, 0.5f);
    return float4(color, 1.f);
#elif 0
    const uint2 pos = uint2(Position.xy);
    const uint tileX = ceil(GlobalData.ViewWidth / TILE_SIZE);
    const uint2 idx = pos / (uint2) TILE_SIZE;
    float c = (idx.x + tileX * idx.y) * 0.00001f;
    
    if (idx.x % 2 == 0)
        c += 0.1f;
    if (idx.y % 2 == 0)
        c += 0.1f;
    
    return float4((float3) c, 1.f);
#elif 0
    return HeatMap(LightGridOpaque, Position.xy, 0.5f);
#elif 0
    return float4(GPassMain[Position.xy].xyz, 1.f);
#elif 1
    float depth = GPassDepth[Position.xy].r;
    
    if (depth > 0.f)
    {
        return float4(GPassMain[Position.xy].xyz, 1.f);
    }
    else
    {
        float4 clip = float4(2.f * UV.x - 1.f, -2.f * UV.y + 1.f, 0.f, 1.f);
        float3 view = mul(GlobalData.InvProjection, clip).xyz;
        float3 direction = mul(view, (float3x3) GlobalData.View);
        return AmbientSpecular.SampleLevel(LinearSampler, direction, 0.1f) * GlobalData.AmbientLight.Intensity;
    }
#endif
}
