// -----------------------------------------------------------------------------
// Transforms
// -----------------------------------------------------------------------------
float4x4 gWorld;
float4x4 gView;
float4x4 gProj;
float4x4 gViewProj;

struct VS_OUTPUT
{
    float4 mPosition : POSITION;
    float2 mTexCoord : TEXCOORD1;
};

struct 
VS_OUTPUT TileVS
    (
    float2 iPosition : POSITION,
	float2 iUV : TEXCOORD0
    )
{
    VS_OUTPUT output = (VS_OUTPUT)0;

	output.mPosition = float4( iPosition.xy, 0, 1 );
	output.mPosition = mul( output.mPosition, gViewProj );
	output.mTexCoord.xy = iUV;

	return output;
}
