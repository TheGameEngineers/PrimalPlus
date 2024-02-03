#include "Common.hlsli"

struct VertexOut
{
    float4 HomogeneousPosition : SV_POSITION;
    float3 WorldPosition : POSITION;
    float3 WorldNormal : NORMAL;
    float3 WorldTangent : TANGENT;
    float2 UV : TEXTURE;
};

struct PixelOut
{
    float4 Color : SV_TARGET0;
};

struct VertexElement
{
    uint ColorTSign;
    uint Normal;
    uint Tangent;
    float2 UV;
};

const static float InvIntervals = 2.f / ((1 << 16) - 1);

cbuffer b00 : register(b0) { GlobalShaderData GlobalData; };
cbuffer b01 : register(b1) { PerObjectData PerObjectBuffer; };
StructuredBuffer<float3>                        VertexPositions         :           register(t0);
StructuredBuffer<VertexElement>                 Elements                :           register(t1);
StructuredBuffer<DirectionalLightParameters>    DirectionalLights       :           register(t3);
StructuredBuffer<LightParameters>               CullableLights          :           register(t4);
StructuredBuffer<uint2>                         LightGrid               :           register(t5);
StructuredBuffer<uint>                          LightIndexList          :           register(t6);

VertexOut TestShaderVS(in uint VertexIdx : SV_VertexID)
{
    VertexOut vsOut;
    VertexElement element = Elements[VertexIdx];
    
    float4 position = float4(VertexPositions[VertexIdx], 1.f);
    float4 worldPosition = mul(PerObjectBuffer.World, position);
   
    uint signs = (element.ColorTSign >> 24) & 0xff;
    float nSign = float(signs & 0x02) - 1.f;
    float tSign = float((signs & 0x01) << 1) - 1.f;
    
    const uint nrm = element.Normal;
    float3 normal;
    normal.x = (nrm & 0x0000ffff) * InvIntervals - 1.f;
    normal.y = (nrm >> 16) * InvIntervals - 1.f;
    normal.z = sqrt(saturate(1.f - dot(normal.xy, normal.xy))) * nSign;
    
    const uint tng = element.Tangent;
    float3 tangent;
    tangent.x = (tng & 0x0000ffff) * InvIntervals - 1.f;
    tangent.y = (tng >> 16) * InvIntervals - 1.f;
    tangent.z = sqrt(saturate(1.f - dot(tangent.xy, tangent.xy))) * tSign;
    
    vsOut.HomogeneousPosition = mul(PerObjectBuffer.WorldViewProjection, position);
    vsOut.WorldPosition = worldPosition.xyz;
    vsOut.WorldNormal = mul(normal, (float3x3)PerObjectBuffer.InvWorld).xyz;
    vsOut.WorldTangent = mul(tangent, (float3x3)PerObjectBuffer.InvWorld).xyz;
    vsOut.UV = element.UV;

    return vsOut;
}

#define NO_LIGHT_ATTENUATION 0

float3 CalculateLighting(float3 N, float3 L, float3 V, float3 lightColor)
{
    const float NoL = dot(N, L);
    float specular = 0;

    if (NoL > 0.f)
    {
        const float3 R = reflect(-L, N);
        const float VoR = max(dot(V, R), 0.f);
        specular = NoL * pow(VoR, 4.f) * 0.5f;
    }

    return (max(0, NoL) + specular) * lightColor;
}

float3 PointLight(float3 N, float3 worldPosition, float3 V, LightParameters light)
{
    float3 L = light.Position - worldPosition;
    const float dSq = dot(L, L);
    float3 color = 0.f;
#if NO_LIGHT_ATTENUATION
    if(dSq < light.Range * light.Range)
    {
        const float dRcp = rsqrt(dSq);
        L *= dRcp;
        color = saturate(dot(N, L)) * light.Color * light.Intensity * 0.01f;
    }
#else
    if (dSq < light.Range * light.Range)
    {
        const float dRcp = rsqrt(dSq);
        L *= dRcp;
        const float attenuation = 1.f - smoothstep(-light.Range, light.Range, rcp(dRcp));
        color = CalculateLighting(N, L, V, light.Color * light.Intensity * attenuation * 0.2f);
    }
#endif
    return color;
}

float3 SpotLight(float3 N, float3 worldPosition, float3 V, LightParameters light)
{
    float3 L = light.Position - worldPosition;
    const float dSq = dot(L, L);
    float3 color = 0.f;
#if NO_LIGHT_ATTENUATION
    if (dSq < light.Range * light.Range)
    {
        const float dRcp = rsqrt(dSq);
        L *= dRcp;
        const float CosAngleToLight = saturate(dot(-L, light.Direction));
        const float angularAttenuation = float(light.CosPenumbra < CosAngleToLight);
        color = saturate(dot(N, L)) * light.Color * light.Intensity * angularAttenuation * 0.01f;
    }
#else
    if (dSq < light.Range * light.Range)
    {
        const float dRcp = rsqrt(dSq);
        L *= dRcp;
        const float attenuation = 1.f - smoothstep(-light.Range, light.Range, rcp(dRcp));
        const float CosAngleToLight = saturate(dot(-L, light.Direction));
        const float angularAttenuation = smoothstep(light.CosPenumbra, light.CosUmbra, CosAngleToLight);
        color = CalculateLighting(N, L, V, light.Color * light.Intensity * attenuation * angularAttenuation * 0.2f);
    }
#endif
    return color;
}

uint GetGridIndex(float2 uv, float width)
{
    const uint2 pos = uint2(uv);
    const uint tileX = ceil(width / TILE_SIZE);
    return (pos.x / TILE_SIZE) + (tileX * (pos.y / TILE_SIZE));
}

[earlydepthstencil]
PixelOut TestShaderPS(in VertexOut psIn, uint pId : SV_PrimitiveID)
{
#if 0
    PixelOut psOut;
    static const uint N = 16;

    float3 color = float3(((pId / uint3(1, N, N * N)) % N) / (float) N);
    psOut.Color = float4(color, 1.f);
    
    return psOut;
#else
    PixelOut psOut;
    
    float3 normal = normalize(psIn.WorldNormal);
    float3 viewDir = normalize(GlobalData.CameraPosition - psIn.WorldPosition);
    
    float3 color = 0;
    
    uint i = 0;
    for (i = 0; i < GlobalData.NumDirectionalLights; ++i)
    {
        DirectionalLightParameters light = DirectionalLights[i];

        float3 lightDirection = light.Direction;
        if (abs(lightDirection.z - 1.f) < 0.001f)
        {
            lightDirection = GlobalData.CameraDirection;
        }
        
        color += 0.02 * CalculateLighting(normal, -lightDirection, viewDir, light.Color * light.Intensity);
    }
    
    const uint gridIndex = GetGridIndex(psIn.HomogeneousPosition.xy, GlobalData.ViewWidth);
    uint lightStartIndex = LightGrid[gridIndex].x;
    const uint lightCount = LightGrid[gridIndex].y;
    
#if USE_BOUNDING_SPHERES
    const uint numPointLights = lightStartIndex + (lightCount >> 16);
    const uint numSpotLights = numPointLights + (lightCount & 0xffff);
    
    for (i = lightStartIndex; i < numPointLights; ++i)
    {
        const uint lightIndex = LightIndexList[i];
        LightParameters light = CullableLights[lightIndex];
        color += PointLight(normal, psIn.WorldPosition, viewDir, light);
    }
    
    for (i = numPointLights; i < numSpotLights; ++i)
    {
        const uint lightIndex = LightIndexList[i];
        LightParameters light = CullableLights[lightIndex];
        color += SpotLight(normal, psIn.WorldPosition, viewDir, light);
    }
#else
    for (i = 0; i < lightCount; ++i)
    {
        const uint lightIndex = LightIndexList[lightStartIndex + i];
        LightParameters light = CullableLights[lightIndex];
                
        if (light.Type == LIGHT_TYPE_POINT_LIGHT)
        {
            color += PointLight(normal, psIn.WorldPosition, viewDir, light);
        }
        else if (light.Type == LIGHT_TYPE_SPOTLIGHT)
        {
            color += SpotLight(normal, psIn.WorldPosition, viewDir, light);
        }
    }
#endif   

    float3 ambient = 0.f / 255.f;
    psOut.Color = saturate(float4(color + ambient, 1.f));
    
    return psOut;
#endif
}