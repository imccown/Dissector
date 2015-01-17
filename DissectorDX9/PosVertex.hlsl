struct Vertex
{
	float2 iPosition : POSITION0;
	float2 iTex0	 : TEXCOORD0;
};

struct OutVertex
{
	float4 iPosition : POSITION0;
	float2 iTex0 : TEXCOORD0;
};

OutVertex PosVertexVS( in Vertex iVert )
{
	OutVertex rvalue;
	rvalue.iPosition = float4( iVert.iPosition.xy, 0.f, 1.f );
	rvalue.iTex0	 = iVert.iTex0;

	return rvalue;
}
