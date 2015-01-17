sampler2D sTextureSampler;

struct PS_INPUT
{
    float2 mTexCoord		: TEXCOORD1;
    float4 mUV[4]		: TEXCOORD2;
};

struct FunkyStruct
{
	float4 thingy;
	float4 bloopy;
	float test;
};

int intRegister;
float4 TilePS( PS_INPUT input ) : COLOR
{
	float4 texColor = tex2D( sTextureSampler, input.mTexCoord );
	FunkyStruct fs;
	//if( texColor.a > .75f )
	//{

	texColor.rgb += tex2D( sTextureSampler, float2(.5f,.5f) );
	[loop]
	for( int ii = 0; ii < intRegister; ii++ )
	{
		fs.test = (texColor.r * .5f) - .5f;
		[loop]
		for( int ii = 0; ii < intRegister; ii++ )
		{
			fs.thingy.rgb += input.mUV[ii].rgb;
		}
		fs.thingy.w += sqrt( dot(texColor.rgb, texColor.rgb) );
		fs.bloopy += fs.thingy * .5f;
		fs.test += sqrt( dot(fs.bloopy, fs.bloopy) );
		clip( fs.test );
	}

	return fs.bloopy;
}
