
#if !defined(PRIMAL_COMMON_HLSLI) && !defined(__cplusplus)
#error Do not include this header directly in shader files. Only include this file via Common.hlsli
#endif

static const uint LIGHT_TYPE_DIRECTIONAL_LIGHT = 0;
static const uint LIGHT_TYPE_POINT_LIGHT = 1;
static const uint LIGHT_TYPE_SPOTLIGHT = 2;