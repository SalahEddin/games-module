/**********************************************
	DX10ParticlesDraw.gsh

	Geometry shader that renders a list of
	particles. The particles are passed as
	points and each is converted to a camera-
	facing quad
***********************************************/

//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

// Constants collected into constant buffers
cbuffer cb0
{
// Geometry shader receives vertices in world space - it needs camera matrices to transform into viewport space
row_major float4x4 ViewProjMatrix;

// Use the inverse camera matrix to help create a camera facing quad for the particle
row_major float4x4 InvViewMatrix;
};


//-----------------------------------------------------------------------------
// Input / output structures
//-----------------------------------------------------------------------------

// Vertex structure used for input 
struct GS_VertIn
{
	float3 Position : POSITION;  // World position of particle
	float3 Velocity : TEXCOORD0; // World velocity of particle (unused when drawing)
	float  Life     : TEXCOORD1; // Current life of particle (unused when drawing)
};

// Vertex structure used as output from this geometry shader
// This shader converts the input particle point into a quad (pair of triangles)
struct GS_VertOut
{
	float4 ViewportPosition : SV_Position; // 2D viewport pixel position for vertex
	float2 TexCoord         : TEXCOORD0;   // UV for new/updated vertex
};


//-----------------------------------------------------------------------------
// Main function
//-----------------------------------------------------------------------------

// Each particle point is converted to a quad (two triangles)
[maxvertexcount(4)]  
void main
(
	point GS_VertIn                  inParticle[1], // One particle in, as a point
	inout TriangleStream<GS_VertOut> outStrip       // Triangle stream output, a quad containing two triangles
)
{
	// Camera-space offsets for the four corners of the quad from the particle centre
	// Will be scaled depending on distance of the particle
    const float3 Corners[4] =
    {
        float3(-1,  1, 0),
        float3( 1,  1, 0),
        float3(-1, -1, 0),
        float3( 1, -1, 0),
    };
    
    // Texture coordinates for the four corners of the generated quad
    const float2 UVs[4] = 
    { 
        float2(0,1), 
        float2(1,1),
        float2(0,0),
        float2(1,0),
    };
    
    GS_VertOut outVert; // Used to build output vertices

	// Output the four corner vertices of a quad centred on the particle position
	for (int i = 0; i < 4; ++i)
	{
		// Use the corners defined above and the inverse camera matrix to calculate each world
		// space corner of the quad
		const float scale = 1.0f;
        float3 corner = Corners[i] * scale;
        float3 worldPosition = inParticle[0].Position + mul( corner, (float3x3)InvViewMatrix );
        
        // Transform to 2D position and output along with an appropriate UV
		outVert.ViewportPosition = mul( float4(worldPosition, 1.0f), ViewProjMatrix );
		outVert.TexCoord = UVs[i];
		outStrip.Append( outVert );
	}
	outStrip.RestartStrip();
}