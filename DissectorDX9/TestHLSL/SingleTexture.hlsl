sampler2D gTex;

float4 SingleTexturePS( in float2 iTex0 : TEXCOORD0 ) : COLOR0
{
	return tex2D( gTex, iTex0 );
}
