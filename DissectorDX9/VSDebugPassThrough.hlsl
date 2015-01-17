struct RColors
{
	float4 mCol0 : COLOR0;
	float4 mCol1 : COLOR1;
};

RColors VSDebugPassThrough(
	float4 iInstructionDebug : TEXCOORD0,
	float4 iExecutionData : TEXCOORD1 )
{
	RColors rvalue;
	rvalue.mCol0 = iInstructionDebug;
	rvalue.mCol1 = iExecutionData;

	return rvalue;
}
