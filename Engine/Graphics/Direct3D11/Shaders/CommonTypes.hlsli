
#if !defined(PRIMAL_COMMON_HLSLI) && !defined(__cplusplus)
#error Do not include this header directly in shader files. Only include this file via Common.hlsli
#endif

#define USE_BOUNDING_SPHERES 1
#define TILE_SIZE 32

struct GlobalShaderData
{
    float4x4    View;
    float4x4    Projection;
    float4x4    InvProjection;
    float4x4    ViewProjection;
    float4x4    InvViewProjection;
    
    float3      CameraPosition;
    float       ViewWidth;
    
    float3      CameraDirection;
    float       ViewHeight;
    
    uint        NumDirectionalLights;
    float       DeltaTime;
};

struct PerObjectData
{
    float4x4 World;
    float4x4 InvWorld;
    float4x4 WorldViewProjection;
    
    float4 BaseColor;
    float3 Emissive;
    float EmissiveIntensity;
    float AmbientOcclusion;
    float Metallic;
    float Roughness;
    uint _pad;
};

struct Plane
{
    float3      Normal;
    float       Distance;
};

struct Sphere
{
    float3      Center;
    float       Radius;
};

struct Cone
{
    float3      Tip;
    float       Height;
    float3      Direction;
    float       Radius;
};

#if USE_BOUNDING_SPHERES
struct Frustum
{
    float3      ConeDirection;
    float       UintRadius;
};
#else
struct Frustum
{
    Plane       Planes[4];
};
#endif

#ifndef __cplusplus
struct ComputeShaderInput
{
    uint3       GroupID             :       SV_GroupID;
    uint3       GroupThreadID       :       SV_GroupThreadID;
    uint3       DispatchThreadID    :       SV_DispatchThreadID;
    uint        GroupIndex          :       SV_GroupIndex;
};
#endif

struct LightCullingDispatchParameters
{
    uint2       NumThreadGroups;
    uint2       NumThreads;
    uint        NumLights;
};

struct LightCullingLightInfo
{
    float3      Position;
    float       Range;
    
    float3      Direction;
#if USE_BOUNDING_SPHERES
    float       CosPenumbra;
#else
    float       ConeRadius;
    
    uint        Type;
    float3      _pad;
#endif
};

struct LightParameters
{
    float3      Position;
    float       Intensity;
 
    float3      Direction;
    float       Range;
    
    float3      Color;
    float       CosUmbra;
    
    float3      Attenuation;
    float       CosPenumbra;
    
#if !USE_BOUNDING_SPHERES
    uint        Type;
    float3      _pad;
#endif
};

struct DirectionalLightParameters
{
    float3      Direction;
    float       Intensity;
    
    float3      Color;
    float       _pad;
};

#ifdef __cplusplus
static_assert((sizeof(PerObjectData) % 16) == 0, "Make sure PerObjectData is formatted in 16 byte chunks without any implicit padding");
static_assert((sizeof(LightParameters) % 16) == 0, "Make sure LightParameters is formatted in 16 byte chunks without any implicit padding");
static_assert((sizeof(LightCullingLightInfo) % 16) == 0, "Make sure LightCullingLightInfo is formatted in 16 byte chunks without any implicit padding");
static_assert((sizeof(DirectionalLightParameters) % 16) == 0, "Make sure DirectionalLightParameters is formatted in 16 byte chunks without any implicit padding");
#endif
