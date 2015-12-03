/*********************************************
	DX10Particles.cpp

	Using geometry shaders and stream out to
	manage GPU-only particle systems
	
	This is not instancing, but a single list
	of points updated and rendered as quads
	using the geometry shader
**********************************************/

#include <string>
using namespace std;

// General definitions used across all the project source files
#include "Defines.h"

// Declarations for supporting source files
#include "Model.h"    // Model class
#include "Camera.h"   // Camera class
#include "Shader.h"   // Vertex / pixel shader support
#include "Input.h"    // Input support
#include "CTimer.h"
#include "BaseMath.h"

#include "Resource.h" // Resource file (used to add icon for application)

// Move / rotation speed constants in Defines.h


//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

// DirectX interfaces
ID3D10Device*           g_pd3dDevice = NULL;
IDXGISwapChain*         g_pSwapChain = NULL;
ID3D10Texture2D*        g_pDepthStencil = NULL;
ID3D10RenderTargetView* g_pRenderTargetView = NULL;
ID3D10DepthStencilView* g_pDepthStencilView = NULL;

// Back buffer
unsigned int ViewportWidth;
unsigned int ViewportHeight;

// Camera
CCamera* MainCamera;

// Models
CModel* Floor = NULL;

// Textures for models
// In DX10, textures used in shaders are replaced by "shader resource views"
ID3D10ShaderResourceView* ParticleTexture;
ID3D10ShaderResourceView* FloorTexture;

// Lights
const int   NumLights = 2;
CModel*     LightModels[NumLights];     // Each light has a model attached in this exercise
D3DXCOLOR   AmbientColour;              // Background light level
D3DXVECTOR4 LightPositions[NumLights];  // Point light positions
D3DXCOLOR   LightColours[NumLights];    // Point light colours
float       LightBrightness[NumLights]; // Point light brightness
float       SpecularPower = 256.0f;     // Specular power same for all models in this exercise
const float LightOrbit = 25.0f; // Controls orbiting light
const float LightSpeed = 1.0f; // -"-


// IMPORTANT: This timer object is created dynamically, through a pointer. Remove this pointer and the program will instantly
// crash. That's because as this variable is global then if it was an actual object it would be constructed *before Main runs*
// The timer class uses Windows functions in it's constructor, which can't be used before Main runs in this kind of project.
// This could be fixed in the timer class, but left like this as an example of the kind of problem that globals with
// constructors can bring. 
CTimer* timer = NULL;

// Shader variables
ID3D10VertexShader*   VS_XformOnly = NULL;
ID3D10VertexShader*   VS_LightingTex = NULL;
ID3D10VertexShader*   VS_PassThruGS = NULL;
ID3D10GeometryShader* GS_ParticlesUpdate = NULL;
ID3D10GeometryShader* GS_ParticlesDraw = NULL;
ID3D10PixelShader*    PS_PlainColour = NULL;
ID3D10PixelShader*    PS_TexOnly = NULL;
ID3D10PixelShader*    PS_LightingTex = NULL;

// Retained vertex shader code for DX10
ID3D10Blob* VSCode_XformOnly = NULL;
ID3D10Blob* VSCode_LightingTex = NULL;
ID3D10Blob* VSCode_PassThruGS = NULL;

// DX10 shader constant buffers
ID3D10Buffer* VS_ConstBuffer = NULL;
ID3D10Buffer* GS_ConstBuffer = NULL;
ID3D10Buffer* PS_ConstBuffer = NULL;

// C++ forms of shader constant buffers. Careful alignment required
#define AL16 __declspec(align(16)) // Align any variable prefixed with AL16 to a 16 byte (float4) boundary,
								   // which is (roughly) what the HLSL compiler does

// The vertex shaders use the same two matrices
struct VS_CONSTS
{
	AL16 D3DXMATRIX WorldMatrix;
	AL16 D3DXMATRIX ViewProjMatrix;
};

// Geometry shader constants
struct GS_CONSTS
{
	AL16 D3DXMATRIX  ViewProjMatrix;
	AL16 D3DXMATRIX  InvViewMatrix;
	float            UpdateTime; // GPU is performing some scene update, so it needs timing info
};

// Pixel shader constants
struct PS_CONSTS
{
	// Current lighting information - ambient + two point lights
	AL16 D3DXCOLOR   BaseColour;       // Shared colour value between shaders (ambient colour or plain colour)
	AL16 D3DXVECTOR4 Light1Position;   // Point light 1 - position
	AL16 D3DXCOLOR   Light1Colour;     // Point light 1 - colour
	AL16 float       Light1Brightness; // Point light 1 - brightness
	AL16 D3DXVECTOR4 Light2Position;   // Point light 2...
	AL16 D3DXCOLOR   Light2Colour;
	AL16 float       Light2Brightness;

	// Shininess of material and camera position needed for specular calculation
	AL16 D3DXVECTOR4 CameraPosition;
	AL16 float       SpecularPower;
};

// Also create a single pointer to each of the above structures for use in rendering code
VS_CONSTS* VS_Consts;
GS_CONSTS* GS_Consts;
PS_CONSTS* PS_Consts;



//*****************************************************************************
// Particle Data
//*****************************************************************************

int NumParticles = 100000;

// The particles are going to be rendered in one draw call as a point list. The points will be expanded to quads in
// the geometry shader. This point list needs to be initialised in a vertex buffer, and will be updated using stream out.
// So the layout of the vertex (particle) data needs to be specified in three ways: for our use in C++, for the creation
// of the vertex buffer, and to indicate what data that will be updated using the stream output stage

// C++ data structure for a particle. Contains both rendering and update information. Contrast this with the instancing
// lab, where there were two seperate structures. The GPU will do all the work in this example
struct SParticle
{
	D3DXVECTOR3 Position;
	D3DXVECTOR3 Velocity;
	float       Life;
};

// An array of element descriptions to create the vertex buffer, *** This must match the SParticle structure ***
// Contents explained with an example: the third entry in the list below indicates that 24 bytes into each
// vertex (into the particle structure) is a TEXCOORD1 which is a single float - this is the Life value (recall
// that non-standard shader input (such as Life) uses TEXCOORD semantics
D3D10_INPUT_ELEMENT_DESC ParticleElts[] = 
{
	// Semantic  &  Index,   Type (eg. 1st one is float3),  Slot,   Byte Offset,  Instancing or not (not here), --"--
	{ "POSITION",   0,       DXGI_FORMAT_R32G32B32_FLOAT,   0,      0,            D3D10_INPUT_PER_VERTEX_DATA,    0 },
	{ "TEXCOORD",   0,       DXGI_FORMAT_R32G32B32_FLOAT,   0,      12,           D3D10_INPUT_PER_VERTEX_DATA,    0 },
	{ "TEXCOORD",   1,       DXGI_FORMAT_R32_FLOAT,         0,      24,           D3D10_INPUT_PER_VERTEX_DATA,    0 },
};
const unsigned int NumParticleElts = sizeof(ParticleElts) / sizeof(D3D10_INPUT_ELEMENT_DESC);

// Vertex layout and buffer described by the above - buffer held on the GPU
ID3D10InputLayout* ParticleLayout; 
ID3D10Buffer* ParticleBufferFrom;
ID3D10Buffer* ParticleBufferTo;


// Third specification is for the data that will be updated using the stream out stage. This array indicates which
// outputs of the vertex or geometry shader will be streamed back into GPU memory. Again, in this case the structure
// below must match the SParticle structure above (although more complex stream out arrangements are possible)
D3D10_SO_DECLARATION_ENTRY ParticleStreamOutDecl[] =
{
	// Semantic name & index, start component, component count, output slot (always 0 here)
	{ "POSITION", 0,   0, 3,   0 }, // Output first 3 components of "POSITION",     - i.e. Position, 3 floats (xyz)
	{ "TEXCOORD", 0,   0, 3,   0 }, // Output the first 3 components of "TEXCOORD0" - i.e. Velocity, 3 floats (xyz)
	{ "TEXCOORD", 1,   0, 1,   0 }, // Output the first component of "TEXCOORD1"    - i.e. Life, 1 float
};

//*****************************************************************************


//-----------------------------------------------------------------------------
// Light functions
//-----------------------------------------------------------------------------

// Initialise a light model for each light in the scene
bool InitialiseLightModels()
{
	for (int light = 0; light < NumLights; ++light)
	{
		LightModels[light] = new CModel();
		// Need to pass vertex shader code that will be used for models under DX10
		if (!LightModels[light]->Load( "Sphere.x", VSCode_XformOnly ))
		{
			return false;
		}
		LightModels[light]->SetScale( 0.3f );
	}
	return true;
}

// Release light models
void UninitialiseLightModels()
{
	for (int light = 0; light < NumLights; ++light)
	{
		delete LightModels[light];
	}
}

// Set the ambient colour for the whole scene
void SetAmbientColour( float r, float g, float b )
{
	AmbientColour = D3DXCOLOR( r, g, b, 1.0f );
}


// Create a point light given position, colour and brightness
void SetPointLight( int lightNum,  float posX, float posY, float posZ,  float r, float g, float b, float bright )
{
	LightPositions[lightNum] = D3DXVECTOR4( posX, posY, posZ, 1.0f );
	LightColours[lightNum] = D3DXCOLOR( r, g, b, 1.0f );
	LightModels[lightNum]->SetPosition( posX, posY, posZ );
	LightBrightness[lightNum] = bright;
}


// Set the given point light's position only
void SetPointLightPos( int lightNum,  float x, float y, float z )
{
	LightPositions[lightNum] = D3DXVECTOR4( x, y, z, 1.0f );
	LightModels[lightNum]->SetPosition( x, y, z ); // Match light model position to light
}


//-----------------------------------------------------------------------------
// Texture functions
//-----------------------------------------------------------------------------

// Load a texture
ID3D10ShaderResourceView* LoadTexture( const string& fileName )
{
	ID3D10ShaderResourceView* texture = NULL;
	
	// Create the texture using D3DX helper function
	// The D3DX10 helper function has extra parameters (all null here - look them up)
	HRESULT hr = D3DX10CreateShaderResourceViewFromFile( g_pd3dDevice, fileName.c_str(), NULL, NULL, &texture, NULL );
	if (FAILED(hr))
	{
		MessageBox(NULL, "Could not find texture map", "CO3303 Lab 20: Geometry Shaders - DX10Particles", MB_OK);
	}

	return texture;
}

// Select the given texture into the given pixel shader slot
void SetTexture( int texNum, ID3D10ShaderResourceView* texture )
{
	g_pd3dDevice->PSSetShaderResources( texNum, 1, &texture );
}


//-----------------------------------------------------------------------------
// State functions
//-----------------------------------------------------------------------------

// Helper function to enable / disable depth and stencil buffering
// Can disable entire depth buffer use, or writes only (e.g. for sorting blended polygons)
void DepthStencilEnable( bool depth = true, bool depthWrite = true, bool stencil = false )
{
	D3D10_DEPTH_STENCIL_DESC dsDesc;
	ZeroMemory( &dsDesc, sizeof(dsDesc) );
	dsDesc.DepthEnable = depth;
	dsDesc.DepthFunc = D3D10_COMPARISON_LESS;
	dsDesc.DepthWriteMask = depthWrite ? D3D10_DEPTH_WRITE_MASK_ALL : D3D10_DEPTH_WRITE_MASK_ZERO;
	dsDesc.StencilEnable = false;
	ID3D10DepthStencilState * dsState;
	g_pd3dDevice->CreateDepthStencilState( &dsDesc, &dsState );
	g_pd3dDevice->OMSetDepthStencilState( dsState, 1 );
}

// Helper function to enable / disable blending modes
void BlendEnable( bool blend, D3D10_BLEND source = D3D10_BLEND_ONE, D3D10_BLEND dest = D3D10_BLEND_ZERO,
				  D3D10_BLEND_OP operation = D3D10_BLEND_OP_ADD )
{
	D3D10_BLEND_DESC blendDesc;
	ZeroMemory( &blendDesc, sizeof(blendDesc) );
	blendDesc.BlendEnable[0] = blend;
	blendDesc.SrcBlend = source;
	blendDesc.DestBlend = dest;
	blendDesc.BlendOp = operation;
	blendDesc.SrcBlendAlpha = D3D10_BLEND_ONE;
	blendDesc.DestBlendAlpha = D3D10_BLEND_ZERO;
	blendDesc.BlendOpAlpha = D3D10_BLEND_OP_ADD;
	blendDesc.RenderTargetWriteMask[0] = D3D10_COLOR_WRITE_ENABLE_ALL;
	ID3D10BlendState* blendState = NULL;
	g_pd3dDevice->CreateBlendState( &blendDesc, &blendState );
	g_pd3dDevice->OMSetBlendState( blendState, 0, 0xffffffff );
}


//-----------------------------------------------------------------------------
// Scene management
//-----------------------------------------------------------------------------

// Creates the scene geometry
bool SceneSetup()
{
	// Load and compile shaders. Two shader pairs here, a pixel lighting effect for the main 
	// models and a simple plain colour effect for the light models
	//*** Note the use of a new function to initialise a geometry shader that uses the stream out stage
	if (!LoadVertexShader( "Graphics Code\\XformOnly.vsh", &VS_XformOnly, &VSCode_XformOnly ) ||
	    !LoadVertexShader( "Graphics Code\\PixelLitTex.vsh", &VS_LightingTex, &VSCode_LightingTex ) ||
	    !LoadVertexShader( "Graphics Code\\PassThruGS.vsh", &VS_PassThruGS, &VSCode_PassThruGS ) ||
	    !LoadStreamOutGeometryShader( "Graphics Code\\DX10ParticlesUpdate.gsh", ParticleStreamOutDecl,
		                              sizeof(ParticleStreamOutDecl) / sizeof(D3D10_SO_DECLARATION_ENTRY), 
		                              sizeof(SParticle), &GS_ParticlesUpdate ) ||
	    !LoadGeometryShader( "Graphics Code\\DX10ParticlesDraw.gsh", &GS_ParticlesDraw ) ||
	    !LoadPixelShader( "Graphics Code\\PlainColour.psh", &PS_PlainColour ) ||
	    !LoadPixelShader( "Graphics Code\\TexOnly.psh", &PS_TexOnly ) ||
	    !LoadPixelShader( "Graphics Code\\PixelLitTex.psh", &PS_LightingTex ))
	{
		return false;
	}

	// Create constant buffers to access global variables in shaders. The buffers sized to hold the structures
	// declared above (see shader variables)
	VS_ConstBuffer = CreateConstantBuffer( sizeof(VS_CONSTS) );
	GS_ConstBuffer = CreateConstantBuffer( sizeof(GS_CONSTS) );
	PS_ConstBuffer = CreateConstantBuffer( sizeof(PS_CONSTS) );

	// As these buffers are shared across all shaders, we can set them up for use now at set-up time
	SetVertexConstantBuffer( VS_ConstBuffer );
	SetGeometryConstantBuffer( GS_ConstBuffer );
	SetPixelConstantBuffer( PS_ConstBuffer );


	// Create camera
	MainCamera = new CCamera();
	MainCamera->SetPosition( 0.0f, 50.0f, -200.0f );
	MainCamera->SetRotation( 0.0f, 0.0f, 0.0f );
	MainCamera->SetAspect(ViewportWidth / static_cast<float>(ViewportHeight));



	// Load models from files
	// Need to pass vertex shader code that will be used for models under DX10
	Floor = new CModel;
	if (!Floor->Load( "Floor.x", VSCode_LightingTex ))
	{
		return false;  // Return on failure
	}


	// Load textures to apply to models / particles
	ParticleTexture = LoadTexture( "Flare.jpg" );
	FloorTexture = LoadTexture( "WoodDiffuseSpecular.dds" );
	if (!ParticleTexture || !FloorTexture)
	{
		return false;
	}


	//*************************************************************************
	// Initialise particles

	// Create the vertex layout using the vertex elements declared near the top of the file. We must also send information
	// about the vertex shader that will accept vertices of this form
	g_pd3dDevice->CreateInputLayout( ParticleElts, NumParticleElts, VSCode_PassThruGS->GetBufferPointer(),
                                     VSCode_PassThruGS->GetBufferSize(), &ParticleLayout );


	// Set up some initial particle data. This will be transferred to the vertex buffer and the CPU will not use it again
	SParticle* particles = new SParticle[NumParticles];
	for (int p = 0; p < NumParticles; ++p)
	{
		particles[p].Position = D3DXVECTOR3( gen::Random(-10.0f, 10.0f), gen::Random(-50.0f, 50.0f), gen::Random(-10.0f, 10.0f) );
		particles[p].Velocity = D3DXVECTOR3( gen::Random(-40.0f, 40.0f), gen::Random(0.0f, 60.0f), gen::Random(-40.0f, 40.0f) );
		particles[p].Life = (5.0f * p) / NumParticles;
	}

	// Create / initialise the vertex buffers to hold the particles
	// Need to create two vertex buffers, because we cannot stream output to the same buffer than we are reading
	// the input from. Instead we input from one buffer, and stream out to another, and swap the buffers each frame
	D3D10_BUFFER_DESC bufferDesc;
	bufferDesc.BindFlags = D3D10_BIND_VERTEX_BUFFER | D3D10_BIND_STREAM_OUTPUT; // Vertex buffer using stream output
	bufferDesc.Usage = D3D10_USAGE_DEFAULT;                  // Default buffer use is read / write from GPU only
	bufferDesc.ByteWidth = NumParticles * sizeof(SParticle); // Buffer size in bytes
	bufferDesc.CPUAccessFlags = 0;   // Indicates that CPU won't access this buffer at all after creation
	bufferDesc.MiscFlags = 0;
	D3D10_SUBRESOURCE_DATA initData; // Initial data (only needed in one of the buffers to start)
	initData.pSysMem = particles;   
	if (FAILED( g_pd3dDevice->CreateBuffer( &bufferDesc, &initData, &ParticleBufferFrom )) ||
		FAILED( g_pd3dDevice->CreateBuffer( &bufferDesc, 0, &ParticleBufferTo )))
	{
		delete[] particles;
		return false;
	}
	delete[] particles;

	//*************************************************************************


	// Create lights
	if (!InitialiseLightModels())
	{
		return false;
	}
	SetAmbientColour( 0.2f, 0.2f, 0.2f );
	SetPointLight( 0,  LightOrbit, 15.0f, 0.0f,  1.0f, 1.0f, 1.0f,  10.0f );
	SetPointLight( 1,  -60.0f, 30.0f, 60.0f,     1.0f, 0.9f, 0.2f,  100.0f );

	return true;
}

// Release everything in the scene
void SceneShutdown()
{
	// Release DirectX allocated objects
	SAFE_RELEASE( ParticleBufferTo );
	SAFE_RELEASE( ParticleBufferFrom );
	SAFE_RELEASE( ParticleLayout );
	SAFE_RELEASE( PS_ConstBuffer );
	SAFE_RELEASE( GS_ConstBuffer );
	SAFE_RELEASE( VS_ConstBuffer );
	SAFE_RELEASE( PS_LightingTex );
	SAFE_RELEASE( PS_TexOnly );
	SAFE_RELEASE( PS_PlainColour );
	SAFE_RELEASE( GS_ParticlesDraw );
	SAFE_RELEASE( GS_ParticlesUpdate );
	SAFE_RELEASE( VSCode_PassThruGS );
	SAFE_RELEASE( VSCode_LightingTex );
	SAFE_RELEASE( VSCode_XformOnly );
	SAFE_RELEASE( VS_PassThruGS );
	SAFE_RELEASE( VS_LightingTex );
	SAFE_RELEASE( VS_XformOnly );
	SAFE_RELEASE( FloorTexture );
	SAFE_RELEASE( ParticleTexture );

	// Release light models
	UninitialiseLightModels();

	// Delete dynamically allocated objects. Our own types - no need to use DirectX release code
	delete Floor;
	delete MainCamera; 
}


// Reset all the particles to their original positions
void ResetParticles()
{
	// Release existing particles
	SAFE_RELEASE( ParticleBufferFrom );

	// Just create a new particle buffer as before
	SParticle* particles = new SParticle[NumParticles];
	for (int p = 0; p < NumParticles; ++p)
	{
		particles[p].Position = D3DXVECTOR3( gen::Random(-10.0f, 10.0f), gen::Random(-50.0f, 50.0f), gen::Random(-10.0f, 10.0f) );
		particles[p].Velocity = D3DXVECTOR3( gen::Random(-40.0f, 40.0f), gen::Random(0.0f, 60.0f), gen::Random(-40.0f, 40.0f) );
		particles[p].Life = (5.0f * p) / NumParticles;
	}

	// Create / initialise the vertex buffer to hold the particles
	D3D10_BUFFER_DESC bufferDesc;
	bufferDesc.BindFlags = D3D10_BIND_VERTEX_BUFFER | D3D10_BIND_STREAM_OUTPUT; // Vertex buffer using stream output
	bufferDesc.Usage = D3D10_USAGE_DEFAULT;                  // Default buffer use is read / write from GPU only
	bufferDesc.ByteWidth = NumParticles * sizeof(SParticle); // Buffer size in bytes
	bufferDesc.CPUAccessFlags = 0;   // Indicates that CPU won't access this buffer at all after creation
	bufferDesc.MiscFlags = 0;
	D3D10_SUBRESOURCE_DATA initData;
	initData.pSysMem = particles;   
	g_pd3dDevice->CreateBuffer( &bufferDesc, &initData, &ParticleBufferFrom );

	delete[] particles;
}


//-----------------------------------------------------------------------------
// Game loop functions
//-----------------------------------------------------------------------------

// Draw one frame of the scene
void RenderScene()
{
	// Clear the back-buffer and the depth/stencil buffer
	float ClearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };// RGBA
	g_pd3dDevice->ClearRenderTargetView( g_pRenderTargetView, ClearColor );
	g_pd3dDevice->ClearDepthStencilView( g_pDepthStencilView, D3D10_CLEAR_DEPTH, 1.0f, 0 );


	//////////////////////////
	// Common model settings

	// Calculate the view and projection matrices for the camera
	MainCamera->CalculateMatrices();
	
	// Map our constant structure pointers to the actual constant buffers. This gives access
	// to the shader constants as C++ variables
	PS_ConstBuffer->Map( D3D10_MAP_WRITE_DISCARD, NULL, ( void** )&PS_Consts );

	// Pass lighting information to pixel shader
	PS_Consts->BaseColour = AmbientColour;
	PS_Consts->Light1Position = LightPositions[0];
	PS_Consts->Light1Colour = LightColours[0];
	PS_Consts->Light1Brightness = LightBrightness[0];
	PS_Consts->Light2Position = LightPositions[1];
	PS_Consts->Light2Colour = LightColours[1];
	PS_Consts->Light2Brightness = LightBrightness[1];
	PS_Consts->CameraPosition = D3DXVECTOR4( MainCamera->GetPosition(), 1.0f );
	PS_Consts->SpecularPower = SpecularPower;

	// We must unmap constant buffers before rendering
	PS_ConstBuffer->Unmap();

	////////////////////
	// Floor rendering

	// Select shaders
	SetVertexShader( VS_LightingTex );
	SetGeometryShader( 0 );            // Not using geometry shader for floor
	SetPixelShader( PS_LightingTex );

	// Calculate world matrix for floor and pass it to the vertex shader
	// When writing to a constant buffer, all previous contents must be discarded, so wea lso need to pass
	// the view-projection matrix each time. We can improve on this by using multiple constant buffers
	Floor->CalculateMatrix();
	VS_ConstBuffer->Map( D3D10_MAP_WRITE_DISCARD, NULL, ( void** )&VS_Consts );
	VS_Consts->ViewProjMatrix = MainCamera->GetViewProjectionMatrix();
	VS_Consts->WorldMatrix = Floor->GetWorldMatrix();
	VS_ConstBuffer->Unmap();

	// Select a texture for the floor and render it
	SetTexture( 0, FloorTexture );
	Floor->Render();


	//////////////////////////
	// Light model rendering

	// Select different shaders - light models are rendering in plain colour
	SetVertexShader( VS_XformOnly );
	SetGeometryShader( 0 );            // Not using geometry shader for lights
	SetPixelShader( PS_PlainColour );
	
	// Render each light in turn
	for (int light = 0; light < NumLights; ++light)
	{
		// Pass world matrix to light's vertex shader
		// Also pass view/projection matrix (see note above)
		LightModels[light]->CalculateMatrix();
		VS_ConstBuffer->Map( D3D10_MAP_WRITE_DISCARD, NULL, ( void** )&VS_Consts );
		VS_Consts->ViewProjMatrix = MainCamera->GetViewProjectionMatrix();
		VS_Consts->WorldMatrix = LightModels[light]->GetWorldMatrix();
		VS_ConstBuffer->Unmap();

		// Use light colour for model plain colour - set in pixel shader
		PS_ConstBuffer->Map( D3D10_MAP_WRITE_DISCARD, NULL, ( void** )&PS_Consts );
		PS_Consts->BaseColour = LightColours[light];
		PS_ConstBuffer->Unmap();

		LightModels[light]->Render();
	}


	//////////////////////////
	// Particle Rendering

	// Set shaders for particle rendering - the vertex shader just passes the data to the 
	// geometry shader, which generates a camera-facing 2D quad from the particle world position 
	// The pixel shader is a very simple texture-only shader
	SetVertexShader( VS_PassThruGS );
	SetGeometryShader( GS_ParticlesDraw );
	SetPixelShader( PS_TexOnly );

	// Set constants for geometry shader, it needs the view/projection matrix to transform the
	// particles to 2D. It also needs the inverse view matrix (the camera's world matrix effectively)
	// to make all the particles face the camera
	GS_ConstBuffer->Map( D3D10_MAP_WRITE_DISCARD, NULL, ( void** )&GS_Consts );
	GS_Consts->ViewProjMatrix = MainCamera->GetViewProjectionMatrix();
	D3DXMATRIX viewMatrix = MainCamera->GetViewMatrix();
	D3DXMATRIX invViewMatrix;
	D3DXMatrixInverse( &invViewMatrix, 0, &viewMatrix );
	GS_Consts->InvViewMatrix = invViewMatrix;
	GS_ConstBuffer->Unmap();

	// Select a texture and set up additive blending (using helper function above)
	SetTexture( 0, ParticleTexture );
	BlendEnable( true, D3D10_BLEND_ONE, D3D10_BLEND_ONE );
	DepthStencilEnable( true, false, false ); // Fix sorting for blending

	// Set up particle vertex buffer / layout
	unsigned int particleVertexSize = sizeof(SParticle);
	unsigned int offset = 0;
	g_pd3dDevice->IASetVertexBuffers( 0, 1, &ParticleBufferFrom, &particleVertexSize, &offset );
	g_pd3dDevice->IASetInputLayout( ParticleLayout );

	// Indicate that this is a point list and render it
	g_pd3dDevice->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_POINTLIST );
	g_pd3dDevice->Draw( NumParticles, 0 );

	// Disable additive blending
	DepthStencilEnable();
	BlendEnable( false );


	// Present the backbuffer contents to the display. 1st parameter = 0: present immediately,  = 1: wait for next vsync
	g_pSwapChain->Present( 0, 0 );
}


// Update the scene between rendering
void UpdateScene()
{
	/****| INFO |******************************************************/
	// Update time is handled entirely inside the DirectX code using
	// a singleton like-approach. This is simpler than getting timing
    // from the Windows form. Actually not doing this properly here as
    // this variable is never released. However, read the long comment
    // near the top - I want to highlight an advanced, subtle issue.
    // There are several tidy ways to handle this properly - timing on
    // DirectX side of the code only but without leaks or construction
    // problems. Try it for yourself if you wish.
	/******************************************************************/
	if (!timer)  timer = new CTimer(); // Timer hasn't been created yet so make one, only happens once
	float updateTime = timer->GetLapTime();


	//////////////////////////
	// Particle Update

	// Shaders for particle update. Again the vertex shader does nothing, and this time we explicitly
	// switch off rendering by setting no pixel shader (also need to switch off depth buffer)
	// All the particle update work is done by the geometry shader
	SetVertexShader( VS_PassThruGS );
	SetGeometryShader( GS_ParticlesUpdate );
	SetPixelShader( 0 );
	DepthStencilEnable( false, false, false );

	// Set the update time in the geometry shader (don't care about other constants in this pass)
	GS_ConstBuffer->Map( D3D10_MAP_WRITE_DISCARD, NULL, ( void** )&GS_Consts );
	GS_Consts->UpdateTime = updateTime;
	GS_ConstBuffer->Unmap();

	// The updated particle data is streamed back to GPU memory, but we can't write to the same buffer
	// we are reading from, so it goes to a second (identical) particle buffer
	unsigned int offset = 0;
	g_pd3dDevice->SOSetTargets( 1, &ParticleBufferTo, &offset );
	
	// Calling draw doesn't actually draw anything, just performs the particle update
	g_pd3dDevice->Draw( NumParticles, 0 );

	// Switch off stream output
	ID3D10Buffer* nullBuffer = 0;
    g_pd3dDevice->SOSetTargets( 1, &nullBuffer, &offset );

	// Switch depth buffer back to default for subseqent rendering
	DepthStencilEnable();

	// Swap the two particle buffers
    ID3D10Buffer* temp = ParticleBufferFrom;
    ParticleBufferFrom = ParticleBufferTo;
    ParticleBufferTo = temp;


	//////////////////////////
	// Other Update

	// Move the camera with keys
	// Keys in order - rotation: up,down,left,right then movement: forward,backward,left,right
	MainCamera->Control( Key_Up, Key_Down, Key_Left, Key_Right, Key_W, Key_S, Key_A, Key_D, updateTime );

	// One light follows an orbit
	static float Rotate = 0.0f;
	SetPointLightPos( 0, cos(Rotate) * LightOrbit, 15.0f, sin(Rotate) * LightOrbit  );
	Rotate -= LightSpeed * updateTime;
}


//-----------------------------------------------------------------------------
// D3D management
//-----------------------------------------------------------------------------

// Initialise Direct3D
bool D3DSetup(HWND hWnd)
{
	// Get dimensions of window that we will render to
	RECT clientRect;
	GetClientRect( hWnd, &clientRect );
	ViewportWidth  = clientRect.right - clientRect.left;
	ViewportHeight = clientRect.bottom - clientRect.top;

	// Create the DirectX device (g_pd3dDevice)and the swap chain
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory( &sd, sizeof(sd) );
	sd.Windowed = TRUE;
	sd.OutputWindow = hWnd; // Handle of window to present back-buffer
	sd.BufferCount = 1;
	sd.BufferDesc.Width  = ViewportWidth;
	sd.BufferDesc.Height = ViewportHeight;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // from DXGI_FORMAT
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;  // Buffer usage is required
	sd.SampleDesc.Count = 1;  // Multisampling settings
	sd.SampleDesc.Quality = 0;

	// This D3DX version of create device tries to create a DX10.1 interface if possible (needs Vista service pack 1)
	// Otherwise gets a DX10 interface
	if (FAILED(D3DX10CreateDeviceAndSwapChain( NULL,                       // Use default adapter (monitor)
	                                           D3D10_DRIVER_TYPE_HARDWARE, // Use hardware driver
	                                           NULL,                       // Only used for software drivers
	                                           0,                          // Flags - only for special cases
	                                           &sd,                        // Structure set up above
	                                           &g_pSwapChain, &g_pd3dDevice ))) // The device and swap chain to be created
	{
		return false;
	}

	// Create depth stencil texture
	D3D10_TEXTURE2D_DESC descDepth;
	descDepth.Width  = ViewportWidth;
	descDepth.Height = ViewportHeight;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D32_FLOAT; // 32-bit float depth value per pixel - no stencil info used here
											  // Use DXGI_FORMAT_D24_UNORM_S8_UINT if using stencil buffer
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D10_USAGE_DEFAULT;
	descDepth.BindFlags = D3D10_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	if (FAILED(g_pd3dDevice->CreateTexture2D( &descDepth, NULL, &g_pDepthStencil )))
	{
		return false;
	}

	
	// Get the back buffer from the swap chain just created - treated as a 2D texture
	ID3D10Texture2D* pBackBuffer;
	if(FAILED(g_pSwapChain->GetBuffer( 0, __uuidof( ID3D10Texture2D ), ( LPVOID* )&pBackBuffer )))
	{
		return false;
	}

	// Create a "render target view" object from the back buffer
	HRESULT hr = g_pd3dDevice->CreateRenderTargetView( pBackBuffer, NULL, &g_pRenderTargetView );
	pBackBuffer->Release();
	if (FAILED(hr))
	{
		return false;
	}

	// Create a "depth stencil view" from the texture we created above
	D3D10_DEPTH_STENCIL_VIEW_DESC descDSV;
	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	if (FAILED(g_pd3dDevice->CreateDepthStencilView( g_pDepthStencil, &descDSV, &g_pDepthStencilView )))
	{
		return false;
	}

	// Bind the back buffer and depth/stencil buffer views to the end of the rendering pipeline
	g_pd3dDevice->OMSetRenderTargets( 1, &g_pRenderTargetView, g_pDepthStencilView );


	// Indentify the viewport (window) dimensions used in the "rasterisation stage"
	D3D10_VIEWPORT vp;
	vp.Width  = ViewportWidth;
	vp.Height = ViewportHeight;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pd3dDevice->RSSetViewports( 1, &vp );


	// Set up tri-linear filtering (and texture wrapping) by defining "sampler state"
	ID3D10SamplerState* samplerState;
	D3D10_SAMPLER_DESC samplerDesc;
	ZeroMemory( &samplerDesc, sizeof(samplerDesc) );
	samplerDesc.Filter = D3D10_FILTER_MIN_MAG_MIP_LINEAR; // Tri-linear filtering equivalent
	samplerDesc.AddressU = D3D10_TEXTURE_ADDRESS_WRAP;    // Wrap texture addressing mode
	samplerDesc.AddressV = D3D10_TEXTURE_ADDRESS_WRAP;    // --"--
	samplerDesc.AddressW = D3D10_TEXTURE_ADDRESS_WRAP;    // --"--
	samplerDesc.ComparisonFunc = D3D10_COMPARISON_NEVER;  // Allows a test on the value sampled from texture prior to use (new to DX10 - not using here)
	samplerDesc.MinLOD = 0;                               // Set range of mip-maps to use, these values indicate...
	samplerDesc.MaxLOD = D3D10_FLOAT32_MAX;               // ...to use all available mip-maps (i.e. enable mip-mapping)
	g_pd3dDevice->CreateSamplerState( &samplerDesc, &samplerState );
	
	// Set the sampler state into the pixel shader pipeline stage
	g_pd3dDevice->PSSetSamplers( 0, 1, &samplerState );


	return true;
}


// Uninitialise D3D
void D3DShutdown()
{
	// Release D3D interfaces
	SAFE_RELEASE( g_pDepthStencilView );
	SAFE_RELEASE( g_pDepthStencil );
	SAFE_RELEASE( g_pRenderTargetView );
	SAFE_RELEASE( g_pSwapChain );
	SAFE_RELEASE( g_pd3dDevice );
}


//-----------------------------------------------------------------------------
// Windows functions
//-----------------------------------------------------------------------------

/****| INFO |*******************************************************************/
// The DirectX code is barely changed - have even left the original WinMain and
// windows code in - it's not used in this app though
/*******************************************************************************/
	
// Window message handler - UNUSED, see above
LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg )
	{
		case WM_DESTROY:
			PostQuitMessage( 0 );
			return 0;

		case WM_KEYDOWN:
			KeyDownEvent( static_cast<EKeyState>(wParam) );
			break;

		case WM_KEYUP:
			KeyUpEvent( static_cast<EKeyState>(wParam) );
			break;
	}

	return DefWindowProc( hWnd, msg, wParam, lParam );
}


// Windows main function - UNUSED, see above (main is in ParticleTool.cpp)
INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR, INT )
{
	// Register the window class (adding our own icon to this window)
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L,
	                  GetModuleHandle(NULL), LoadIcon( hInst, MAKEINTRESOURCE(IDI_TL) ),
	                  LoadCursor( NULL, IDC_ARROW ), NULL, NULL,
	                  "DX10Particles", NULL };
	RegisterClassEx( &wc );

	// Create the application's window
	HWND hWnd = CreateWindow( "DX10Particles", "CO3303 Lab 20: Geometry Shaders - DX10Particles",
	                          WS_OVERLAPPEDWINDOW, 100, 100, 1024, 1024,
	                          NULL, NULL, wc.hInstance, NULL );

	// Initialize Direct3D
	if (D3DSetup( hWnd ))
	{
		// Prepare the scene
		if (SceneSetup())
		{
			// Show the window
			ShowWindow( hWnd, SW_SHOWDEFAULT );
			UpdateWindow( hWnd );

			// Enter the message loop
			MSG msg;
			ZeroMemory( &msg, sizeof(msg) );
			while( msg.message != WM_QUIT )
			{
				if( PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE ) )
				{
					TranslateMessage( &msg );
					DispatchMessage( &msg );
				}
				else
				{
					// Render and update the scene
					RenderScene();
					UpdateScene();
					if (KeyHeld( Key_Escape )) // Allow user to quit with escape key
					{
						DestroyWindow( hWnd );
					}
				}
			}
		}
		SceneShutdown();
	}
	D3DShutdown();

	UnregisterClass( "DX10Particles", wc.hInstance );
	return 0;
}
