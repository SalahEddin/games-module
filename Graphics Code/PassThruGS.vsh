/**********************************************
	PassThruGS.vsh

	Vertex shader that passes on particle data
	to the geometry shader without change
	The geometry shaders do all the update
	& rendering work for this particle system
***********************************************/

//-----------------------------------------------------------------------------
// Input / output structures
//-----------------------------------------------------------------------------

// Input and output from vertex shader - particle data
struct VS_InputOutput
{
	float3 Position : POSITION; 
	float3 Velocity : TEXCOORD0;
	float  Life     : TEXCOORD1;
};


//-----------------------------------------------------------------------------
// Main function
//-----------------------------------------------------------------------------

// Main vertex shader function
void main( in VS_InputOutput i, out VS_InputOutput o ) 
{
	o = i;
}
