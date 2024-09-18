
float Pow5(float x)
{
    float xx = x * x;
    return xx * xx * x;
}

float D_GGX(float NoH, float a)
{
    float d = (NoH * a - NoH) * NoH + 1;
    return a / (PI * d * d);
}

float V_SmithGGXCorrelated(float NoV, float NoL, float a)
{
    float GGXL = NoV * sqrt((-NoL * a + NoL) * NoL + 1);
    float GGXV = NoL * sqrt((-NoV * a + NoV) * NoV + 1);
    return 0.5f / (GGXV + GGXL);
}

float V_SmithGGXCorrelatedApprox(float NoV, float NoL, float a)
{
    float GGXV = NoL * ((-NoV * a + NoV) * a);
    float GGXL = NoV * ((-NoL * a + NoL) * a);
    return 0.5f / (GGXV + GGXL);
}

float3 F_Schlick(float3 F0, float VoN)
{
    float u = Pow5(1.f - VoN);
    float3 F90Approx = saturate(50.f * F0.g);
    return F90Approx * u + (1.f - u) * F0;
}

float3 F_Schlick(float u, float3 f0, float3 f90)
{
    return f0 + (f90 - f0) * Pow5(1.f - u);
}

float3 Diffuse_Lambert()
{
    return 1.f / PI;
}

float3 Diffuse_Burley(float NoV, float NoL, float VoH, float roughness)
{
    float u = Pow5(1.f - NoV);
    float v = Pow5(1.f - NoL);
    
    float FD90 = 0.5f + 2.f * VoH * VoH * roughness;
    float FdV = 1.f + (u * FD90 - u);
    float FdL = 1.f + (v * FD90 - v);
    return (1.f / PI) * FdV * FdL;
}
