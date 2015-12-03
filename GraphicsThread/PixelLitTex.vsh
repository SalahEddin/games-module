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

// World matrix and combined view / projection matrix for transformation calculations
float4x4 WorldMatrix    : WORLD;
float4x4 ViewProjMatrix : VIEWPROJECTION;


//-----------------------------------------------------------------------------
// Input / output structures
//-----------------------------------------------------------------------------

// Input to vertex shader - usual position, normal and UVs
struct VS_Input
{
	float3 Position  : POSITION;  // The position of the vertex in model-space
	float3 Normal    : NORMAL;    // Vertex normal in model-space
	float2 TexCoord0 : TEXCOORD0; // Texture coordinate for the vertex
};

// Output from vertex shader. This shader sends the world position and world normal to the pixel
// shader to do the lighing calculations. There are no semantics for such a use so we use
// TEXCOORD semantics, this is usual for any non-standard data types passed to a pixel shader
struct VS_Output
{
	float4 Position      : POSITION;
	float2 TexCoord0     : TEXCOORD0;
	float3 WorldPosition : TEXCOORD1; // Use TEXCOORD semantic for non-standard data - see above
	float3 WorldNormal   : TEXCOORD2;
};

//-----------------------------------------------------------------------------
// Main function
//-----------------------------------------------------------------------------

// Main vertex shader function. Transforms model-space position and normal to world space and
// passes on to the pixel shader for it to do per-pixel lighting calculations
void main( in VS_Input i, out VS_Output o ) 
{
    // Convert model vertex position from (x,y,z) to (x,y,z,1) to prepare for matrix multiplication
    float4 ModelPosition = float4(i.Position, 1.0f);

    // Multiply model vertex position by the world matrix to get its 3D world position
    float4 WorldPosition = mul( ModelPosition, WorldMatrix );         

    // Multiply world position by combined view/projection matrix to get 2D viewport vertex
    o.Position = mul( WorldPosition, ViewProjMatrix );

	// Similar process to transform model normals to world space
    float4 ModelNormal = float4(i.Normal, 0.0f);
    float4 WorldNormal = mul( ModelNormal, WorldMatrix );

    // For pixel lighting, pass the world position & normal to the pixel shader
    o.WorldPosition = WorldPosition;
	o.WorldNormal = WorldNormal;

	// Pass input texture coordinate directly to the pixel shader
    o.TexCoord0 = i.TexCoord0;
}
