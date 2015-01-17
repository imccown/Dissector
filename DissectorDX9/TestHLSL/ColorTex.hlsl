sampler2D gTex;
float4 kColor;

float4 ColorTexPS( in float2 iTex0 : TEXCOORD0 ) : COLOR0
{
	return kColor * tex2D( gTex, iTex0 );
}
