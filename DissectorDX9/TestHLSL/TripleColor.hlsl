float4 kColor;
float4 kBlend;
float4 kBlah;

float4 TripleColorPS( ) : COLOR0
{
	return (kColor * kBlend * kBlah) + kBlah;
}
