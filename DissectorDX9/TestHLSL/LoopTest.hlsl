//
// Mesh effect
//

// -----------------------------------------------------------------------------
// Transforms
// -----------------------------------------------------------------------------
float4x4 gWorld;
float4x4 gView;
float4x4 gProj;
float4x4 gViewProj;

// -----------------------------------------------------------------------------
// Textures
// -----------------------------------------------------------------------------
texture sTexture0;
sampler sTextureSampler = sampler_state
{ 
  texture = sTexture0; 
  mipfilter = POINT; 
  AddressU = Clamp;
  AddressV = Clamp;
  MinFilter = POINT;
  MagFilter = POINT;
};


// -----------------------------------------------------------------------------
// Structures
// -----------------------------------------------------------------------------
struct VS_OUTPUT
{
    float4 mPosition : POSITION;
    float2 mTexCoord : TEXCOORD1;
};

struct PS_INPUT
{
    float2 mTexCoord		: TEXCOORD1;
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

struct FunkyStruct
{
	float4 thingy;
	float4 bloopy;
	float test;
};

float4 TilePS( PS_INPUT input ) : COLOR
{
	float4 rvalue;
	float4 texColor = tex2D( sTextureSampler, input.mTexCoord );

	FunkyStruct fs = (FunkyStruct)0;

	for( int ii = 0; ii < 4; ++ii )
	{
		fs.test = (texColor.r * .5f) - .5f;
		texColor.rgb += tex2D( sTextureSampler, float2(.5f,.5f) ).rgb;
		if( ii == 3 )
		{
			rvalue = texColor.rgbb;
		}
		fs.thingy.rgb += texColor.rgb;
		fs.thingy.w += sqrt( dot(texColor.rgb, texColor.rgb) );
		fs.bloopy += fs.thingy * .5f;
		fs.test = sqrt( dot(fs.bloopy, fs.bloopy) );
	}

	return rvalue + fs.thingy;//fs.bloopy;
}
