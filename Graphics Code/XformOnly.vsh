/**********************************************
	XformOnly.vsh

	Simple vertex shader that only transforms
	the vertex position into 2D viewport space
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

// Input to vertex shader
struct VS_Input
{
	float3 Position : POSITION;
};

// Output from vertex shader
struct VS_Output
{
	float4 Position : SV_Position; // New semantics for DX10 - pixel position from vertex shader - not used in DX9
};

//-----------------------------------------------------------------------------
// Main function
//-----------------------------------------------------------------------------

// Main vertex shader function
void main( in VS_Input i, out VS_Output o ) 
{
   // Transform model-space vertex position directly to viewport space, then output it
	o.Position = mul( mul( float4(i.Position, 1.0f), WorldMatrix ), ViewProjMatrix );         
}
