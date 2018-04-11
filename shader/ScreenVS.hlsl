//--------------------------------------------------------------------------------------
// Vertex Shader
// http://altdevblog.com/2011/08/08/an-interesting-vertex-shader-trick/
//--------------------------------------------------------------------------------------
struct VSQuadOut
{
	float4 position : SV_Position;
	float2 texcoord: TexCoord;
};

VSQuadOut Entry(uint id: SV_VertexID)
{
	VSQuadOut Out;
	// full screen quad with texture coords
	Out.texcoord = float2((id << 1) & 2, id & 2);
	Out.position = float4(Out.texcoord * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
	return Out;
}
