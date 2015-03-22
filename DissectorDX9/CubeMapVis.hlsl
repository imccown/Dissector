samplerCUBE gTex : register(s0);
float3 xRotate( float3 iVal, float iRad )
{
    float sVal = sin( iRad );
    float cVal = cos( iRad );
    
    float3 rvalue;
    rvalue.x = iVal.x;
	rvalue.y = (cVal *  iVal.y - sVal *  iVal.z);
	rvalue.z = (sVal *  iVal.y + cVal *  iVal.z);
    
    return rvalue;
}

float3 yRotate( float3 iVal, float iRad )
{
    float sVal = sin( iRad );
    float cVal = cos( iRad );
    
    float3 rvalue;
    rvalue.x = (cVal * iVal.x + sVal * iVal.z);
    rvalue.y = iVal.y;
    rvalue.z = (-sVal * iVal.x + cVal * iVal.z);
    
    return rvalue;
}

float3 zRotate( float3 iVal, float iRad )
{
    float sVal = sin( iRad );
    float cVal = cos( iRad );
    
    float3 rvalue;
	rvalue.x = (cVal * iVal.x - sVal * iVal.y);
	rvalue.y = (sVal * iVal.x + cVal * iVal.y);
    rvalue.z = iVal.z;
    
    return rvalue;
}

float4 CubeMapVisPS( in float2 uv : TEXCOORD0 ) : COLOR0
{
    float4 fragColor;
    
    float3 dir;
    dir = float3( -1.0, 0.0, 0.0 );
    dir = zRotate( dir, (uv.y-(.375/.75)) * .75 * 2.0 * 3.14 );
    dir = yRotate( dir, uv.x * 2.0 * 3.14 );
    
        fragColor = texCUBE( gTex, dir.xyz );

    return fragColor;
}