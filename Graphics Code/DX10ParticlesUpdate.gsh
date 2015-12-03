/**********************************************
	DX10ParticlesUpdate.gsh

	Geometry shader that updates a list of
	particles (passed as points) and uses the
	stream out stage to rewrite them into their
	vertex buffer
	Does not perform any rendering
***********************************************/

//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

// Constants collected into constant buffers
cbuffer cb0
{
// Unused in this shader, but retained to keep constant buffer structure the same across all geometry shaders
row_major float4x4 ViewProjMatrix;
row_major float4x4 InvViewMatrix;

// GPU is performing some scene update, so it needs timing info
float UpdateTime;
};

//-----------------------------------------------------------------------------
// Input / output structures
//-----------------------------------------------------------------------------

// Vertex structure used for input and output
struct GS_VertInOut
{
	float3 Position : POSITION;  // World position of particle
	float3 Velocity : TEXCOORD0; // World velocity of particle
	float  Life     : TEXCOORD1; // Current life of particle (seconds)
};


//-----------------------------------------------------------------------------
// Main function
//-----------------------------------------------------------------------------

[maxvertexcount(1)]  // This shader deals with particles one at a time. When a particle dies, it is immediately respawned,
                     // so this shader will always output one particle as well
void main
(
	point GS_VertInOut               inParticle[1],  // One particle in, as a point
	inout PointStream<GS_VertInOut>  outParticles    // Must output a point stream, will only contain the one updated point
)
{
	const float3 Gravity = { 0, -9.8f, 0 };
	
	// Fairly general purpose particle update code
    inParticle[0].Life -= UpdateTime;
    inParticle[0].Position += inParticle[0].Velocity * UpdateTime;
    inParticle[0].Velocity += Gravity * UpdateTime;    
    outParticles.Append( inParticle[0] );
}