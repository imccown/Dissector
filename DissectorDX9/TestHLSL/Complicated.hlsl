#include "Common.hlsl"

sampler2D gTex;
sampler2D gShadow;
float     gDepth;

float4 ComplicatedPS( in float2 iTex0 : TEXCOORD0 ) : COLOR0
{
	float4 val1 = tex2D( gTex, iTex0 );
	float4 val2 = tex2D( gShadow, iTex0 );

	float4 rvalue = DoSomeBullshit( val1, val2 ) + val1 + val2;

	return rvalue;
}
