/**********************************************
	XFormOnly.vsh

	Simple vertex shader that only transforms
	the vertex position into 2D viewport space
***********************************************/

//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

// Use a single combined world-view-projection matrix to perform the transform
float4x4 WorldViewProjMatrix : WORLDVIEWPROJECTION;


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
	float4 Position : POSITION;
};

//-----------------------------------------------------------------------------
// Main function
//-----------------------------------------------------------------------------

// Main vertex shader function
void main( in VS_Input i, out VS_Output o ) 
{
    // Transform model-space vertex position directly to viewport space, then output it
    o.Position = mul( float4(i.Position, 1.0f), WorldViewProjMatrix );         
}
