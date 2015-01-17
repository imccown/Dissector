#include "Common.hlsl"

float4 kColor;
float4 kBlend;

float4 IncludeTestPS( ) : COLOR0
{
	return kColor * DoSomeBullshit( kColor, kBlend );
}
