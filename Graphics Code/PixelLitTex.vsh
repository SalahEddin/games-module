/**********************************************
	PixelLitTex.vsh

	Vertex shader that transforms 3D vertices
	to viewport, and passes data for pixel-
	lighting to the pixel shader. Also passes
	a set of texture coordinates (UVs)
***********************************************/

//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

// When using constant buffers in DX10 (see RenderScene in main code), all global shader variables
// must be collected together into cbuffer structures (the name, cb0, is not important)
cbuffer cb0
{
// World matrix and combined view / projection matrix for transformation calculations
row_major float4x4 WorldMatrix;    // Need to indicate that our matrices are row-based (recall DirectX vs OpenGL/maths text differences)
row_major float4x4 ViewProjMatrix; // --"--
};


//-----------------------------------------------------------------------------
// Input / output structures
//-----------------------------------------------------------------------------

// Input to vertex shader - usual position, normal and UVs
struct VS_Input
{
	float3 Position : POSITION;  // Position of the vertex in model-space
	float3 Normal   : NORMAL;    // Vertex normal in model-space
	float2 TexCoord : TEXCOORD0; // Texture coordinate for the vertex
};

// Output from vertex shader. This shader sends the world position and world normal to the pixel
// shader to do the lighing calculations
struct VS_Output
{
	float4 ViewportPosition : SV_Position; // 2D viewport position of vertex
	float3 WorldPosition    : TEXCOORD0;   // World position of the vertex
	float3 WorldNormal      : TEXCOORD1;   // World normal of the vertex
	float2 TexCoord         : TEXCOORD2;
};

//-----------------------------------------------------------------------------
// Main function
//-----------------------------------------------------------------------------

// Main vertex shader function
void main( in VS_Input i, out VS_Output o ) 
{
	///////////////////////////
	// Matrix transformations

	// Convert model vertex position from (x,y,z) to (x,y,z,1) to prepare for matrix multiplication
	float4 ModelPosition = float4(i.Position, 1.0f);

	// Multiply model vertex position by the world matrix to get its 3D world position
	float4 WorldPosition = mul( ModelPosition, WorldMatrix );         

	// Multiply world position by combined view/projection matrix to get 2D viewport position
	o.ViewportPosition = mul( WorldPosition, ViewProjMatrix );


	// Similar process for normals
	float4 ModelNormal = float4(i.Normal, 0.0f);
	float4 WorldNormal = normalize( mul( ModelNormal, WorldMatrix ) );


	///////////////////////////
	// Output for Pixel Shader

	// For pixel lighting, pass the world position & world normal to the pixel shader
	o.WorldPosition = WorldPosition;
	o.WorldNormal = WorldNormal;

	// Pass input texture coordinate directly to the pixel shader
	o.TexCoord = i.TexCoord;
}
