#include "Common.hlsl"

sampler2D gTex;
sampler2D gShadow;
float     gDepth;
float4x4  gMatrix;
float4x4  gMatrix2;


float4 Complicated2PS( in float2 iTex0 : TEXCOORD0 ) : COLOR0
{
	float4 val1 = tex2D( gTex, iTex0 );
	float4 val2 = tex2D( gShadow, iTex0 );

	float4x4 tempMatrix = mul( gMatrix, gMatrix2 );
	float4 rvalue = .75f * (DoSomeBullshit( val1, val2 ) + val1 + val2);
	rvalue = mul( rvalue, tempMatrix );

	return rvalue;
}
