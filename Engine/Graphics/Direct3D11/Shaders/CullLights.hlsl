#include "Common.hlsli"

#if USE_BOUNDING_SPHERES
static const uint           MaxLightsPerGroup = 1024;

groupshared uint                                _minDepthVS;
groupshared uint                                _maxDepthVS;
groupshared uint                                _lightCount;
groupshared uint                                _lightIndexStartOffset;
groupshared uint                                _lightIndexList[MaxLightsPerGroup];
groupshared uint                                _lightFlagsOpaque[MaxLightsPerGroup];
groupshared uint                                _spotLightStartOffset;
groupshared uint2                               _opaqueLightIndex;

cbuffer                                         b00                     :       register(b0) { GlobalShaderData GlobalData; };
cbuffer                                         b01                     :       register(b1) { LightCullingDispatchParameters ShaderParams; }
StructuredBuffer<Frustum>                       Frustums                :       register(t0);
StructuredBuffer<LightCullingLightInfo>         Lights                  :       register(t1);
StructuredBuffer<Sphere>                        BoundingSpheres         :       register(t2);
Texture2D                                       GPassDepth              :       register(t3);

RWStructuredBuffer<uint>                        LightIndexCounter       :       register(u0);
RWStructuredBuffer<uint2>                       LightGrid_Opaque        :       register(u1);
RWStructuredBuffer<uint>                        LightIndexList_Opaque   :       register(u3);

Sphere GetConeBoundingSphere(float3 tip, float range, float3 direction, float cosPenumbra)
{
    Sphere sphere;
    sphere.Radius = range / (2.f * cosPenumbra);
    sphere.Center = tip + sphere.Radius * direction;
    
    if(cosPenumbra < 0.707107f)
    {
        const float coneSin = sqrt(1.f - cosPenumbra * cosPenumbra);
        sphere.Center = tip + cosPenumbra * range * direction;
        sphere.Radius = coneSin * range;
    }
    
    return sphere;
}

bool Intersects(Frustum frustum, Sphere sphere, float minDepth, float maxDepth)
{
    if ((sphere.Center.z - sphere.Radius > minDepth) || (sphere.Center.z + sphere.Radius < maxDepth)) return false;
    
    const float3 lightRejection = sphere.Center - dot(sphere.Center, frustum.ConeDirection) * frustum.ConeDirection;
    const float distSq = dot(lightRejection, lightRejection);
    const float radius = sphere.Center.z * frustum.UintRadius + sphere.Radius;
    const float radiusSq = radius * radius;
    
    return distSq <= radiusSq;
}

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void CullLightsCS(ComputeShaderInput csIn)
{
    //INITIALIZATION SECTION
    const float depth = GPassDepth[csIn.DispatchThreadID.xy].r;
    const float C = GlobalData.Projection._m22;
    const float D = GlobalData.Projection._m23;
    const uint gridIndex = csIn.GroupID.x + (csIn.GroupID.y * ShaderParams.NumThreadGroups.x);
    const Frustum frustum = Frustums[gridIndex];
    
    if (csIn.GroupIndex == 0)
    {
        _minDepthVS = 0x7f7fffff;
        _maxDepthVS = 0;
        _lightCount = 0;
        _opaqueLightIndex = 0;
    }
    
    uint i = 0, index = 0;
    
    for(i = csIn.GroupIndex; i < MaxLightsPerGroup; i += TILE_SIZE * TILE_SIZE)
    {
        _lightFlagsOpaque[i] = 0;
    }
    
    //DEPTH MIN/MAX SECTION
    GroupMemoryBarrierWithGroupSync();
    
    const float depthVS = ClipToView(float4(0.f, 0.f, depth, 1.f), GlobalData.InvProjection).z;
    const uint z = asuint(-depthVS);
    
    if (depth != 0)
    {
        InterlockedMin(_minDepthVS, z);
        InterlockedMax(_maxDepthVS, z);
    }
    
    //LIGHT CULLING SECTION
    GroupMemoryBarrierWithGroupSync();
    
    const float minDepthVS = -asfloat(_minDepthVS);
    const float maxDepthVS = -asfloat(_maxDepthVS);
    
    for (i = csIn.GroupIndex; i < ShaderParams.NumLights; i += TILE_SIZE * TILE_SIZE)
    {
        Sphere sphere = BoundingSpheres[i];
        sphere.Center = mul(GlobalData.View, float4(sphere.Center, 1.f)).xyz;
        
        if (Intersects(frustum, sphere, minDepthVS, maxDepthVS))
        {
            InterlockedAdd(_lightCount, 1, index);
            if (index < MaxLightsPerGroup)
                _lightIndexList[index] = i;
        }
    }
    
    //LIGHT PRUNING SECTION
    GroupMemoryBarrierWithGroupSync();
    
    const uint lightCount = min(_lightCount, MaxLightsPerGroup);
    const float2 invViewDimensions = 1.f / float2(GlobalData.ViewWidth, GlobalData.ViewHeight);
    const float3 pos = UnProjectUV(csIn.DispatchThreadID.xy * invViewDimensions, depth, GlobalData.InvViewProjection).xyz;
    
    for (i = 0; i < lightCount; ++i)
    {
        index = _lightIndexList[i];
        const LightCullingLightInfo light = Lights[index];
        const float3 d = pos - light.Position;
        const float distSq = dot(d, d);
        
        if (distSq <= light.Range * light.Range)
        {
            const bool isPointLight = light.CosPenumbra == -1.f;
            if (isPointLight || (dot(d * rsqrt(distSq), light.Direction) >= light.CosPenumbra))
            {
                _lightFlagsOpaque[i] = 2 - uint(isPointLight);
            }
        }
    }

    //UPDATE LIGHT GRID SECTION
    GroupMemoryBarrierWithGroupSync();
    if (csIn.GroupIndex == 0)
    {
        uint numPointLights = 0;
        uint numSpotLights = 0;
        
        for (i = 0; i < lightCount; ++i)
        {
            numPointLights += (_lightFlagsOpaque[i] & 1);
            numSpotLights += (_lightFlagsOpaque[i] >> 1);
        }
        
        InterlockedAdd(LightIndexCounter[0], numPointLights + numSpotLights, _lightIndexStartOffset);
        _spotLightStartOffset = _lightIndexStartOffset + numPointLights;
        LightGrid_Opaque[gridIndex] = uint2(_lightIndexStartOffset, (numPointLights << 16) | numSpotLights);
    }
    
    //UPDATE LIGHT INDEX LIST SECTION
    GroupMemoryBarrierWithGroupSync();
    
    uint pointIndex, spotIndex;
    
    for (i = csIn.GroupIndex; i < lightCount; i += TILE_SIZE * TILE_SIZE)
    {
        if (_lightFlagsOpaque[i] == 1)
        {
            InterlockedAdd(_opaqueLightIndex.x, 1, pointIndex);
            LightIndexList_Opaque[_lightIndexStartOffset + pointIndex] = _lightIndexList[i];
        }
        else if (_lightFlagsOpaque[i] == 2)
        {
            InterlockedAdd(_opaqueLightIndex.y, 1, spotIndex);
            LightIndexList_Opaque[_spotLightStartOffset + spotIndex] = _lightIndexList[i];
        }
    }
}

#else

static const uint           MaxLightsPerGroup = 1024;

groupshared uint            _minDepthVS;
groupshared uint            _maxDepthVS;
groupshared uint            _lightCount;
groupshared uint            _lightIndexStartOffset;
groupshared uint            _lightIndexList[MaxLightsPerGroup];

cbuffer                                         b00                     :       register(b0) { GlobalShaderData GlobalData; };
cbuffer                                         b01                     :       register(b1) { LightCullingDispatchParameters ShaderParams; }
StructuredBuffer<Frustum>                       Frustums                :       register(t0);
StructuredBuffer<LightCullingLightInfo>         Lights                  :       register(t1);
Texture2D                                       GPassDepth              :       register(t2);

RWStructuredBuffer<uint>                        LightIndexCounter       :       register(u0);
RWStructuredBuffer<uint2>                       LightGrid_Opaque        :       register(u1);
RWStructuredBuffer<uint>                        LightIndexList_Opaque   :       register(u3);

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void CullLightsCS(ComputeShaderInput csIn)
{
    //INITIALIZATION SECTION
    if (csIn.GroupIndex == 0)
    {
        _minDepthVS = 0x7f7fffff;
        _maxDepthVS = 0;
        _lightCount = 0;
    }
    
    uint i = 0, index = 0;
    
    //DEPTH MIN/MAX SECTION
    GroupMemoryBarrierWithGroupSync();
    
    const float depth = GPassDepth[csIn.DispatchThreadID.xy].r;
    
    const float depthVS = ClipToView(float4(0.f, 0.f, depth, 1.f), GlobalData.InvProjection).z;
    const uint z = asuint(-depthVS);
    
    if (depth != 0)
    {
        InterlockedMin(_minDepthVS, z);
        InterlockedMax(_maxDepthVS, z);
    }
    
    //LIGHT CULLING SECTION
    GroupMemoryBarrierWithGroupSync();
    
    const uint gridIndex = csIn.GroupID.x + (csIn.GroupID.y * ShaderParams.NumThreadGroups.x);
    const Frustum frustum = Frustums[gridIndex];
    const float minDepthVS = -asfloat(_minDepthVS);
    const float maxDepthVS = -asfloat(_maxDepthVS);
    
    for (i = csIn.GroupIndex; i < ShaderParams.NumLights; i += TILE_SIZE * TILE_SIZE)
    {
        const LightCullingLightInfo light = Lights[i];
        const float3 lightPositionVS = mul(GlobalData.View, float4(light.Position, 1.f)).xyz;
        
        if (light.Type == LIGHT_TYPE_POINT_LIGHT)
        {
            Sphere sphere = { lightPositionVS, light.Range };
            if (SphereInsideFrustum(sphere, frustum, minDepthVS, maxDepthVS))
            {
                InterlockedAdd(_lightCount, 1, index);
                if (index < MaxLightsPerGroup)
                    _lightIndexList[index] = i;
            }
        }
        else if (light.Type == LIGHT_TYPE_SPOTLIGHT)
        {
            const float3 lightDirectionVS = mul(GlobalData.View, float4(light.Direction, 0.f)).xyz;
            const Cone cone = { lightPositionVS, light.Range, lightDirectionVS, light.ConeRadius };
            if (ConeInsideFrustum(cone, frustum, minDepthVS, maxDepthVS))
            {
                InterlockedAdd(_lightCount, 1, index);
                if (index < MaxLightsPerGroup)
                    _lightIndexList[index] = i;
            }
        }
    }
    
    //UPDATE LIGHT GRID SECTION
    GroupMemoryBarrierWithGroupSync();
    
    const uint lightCount = min(_lightCount, MaxLightsPerGroup);

    if (csIn.GroupIndex == 0)
    {
        InterlockedAdd(LightIndexCounter[0], lightCount, _lightIndexStartOffset);
        LightGrid_Opaque[gridIndex] = uint2(_lightIndexStartOffset, lightCount);
    }
    
    //UPDATE LIGHT INDEX LIST SECTION
    GroupMemoryBarrierWithGroupSync();
    
    for (i = csIn.GroupIndex; i < lightCount; i += TILE_SIZE * TILE_SIZE)
    {
        LightIndexList_Opaque[_lightIndexStartOffset + i] = _lightIndexList[i];
    }
}
#endif