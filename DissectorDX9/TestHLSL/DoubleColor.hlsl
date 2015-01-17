float4 kColor;
float4 kBlend;

float4 DoubleColorPS( ) : COLOR0
{
	return kColor * kBlend;
}
