
float4 DoSomeBullshit( float4 iVal1, float4 iVal2 )
{
	float4 rvalue = iVal1 + iVal2;
	rvalue = rvalue * iVal1 * iVal2;

	return rvalue;
}