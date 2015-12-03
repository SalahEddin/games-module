/*********************************************
	GraphicsThread.cpp

	Threading in a graphics environment
**********************************************/

#include <string>
using namespace std;

#define _WIN32_WINNT 0x0400 // Must define minimum Windows version to use TryEnterCriticalSection
#include <process.h> // Use standard library to create / destroy threads
#include <windows.h> // Use windows functions for other thread control

// General definitions used across all the project source files
#include "Defines.h"

// Declarations for supporting source files
#include "Model.h"    // Model class
#include "Camera.h"   // Camera class
#include "Shader.h"   // Vertex / pixel shader support
#include "Input.h"    // Input support

#include "Resource.h" // Resource file (used to add icon for application)

// Move / rotation speed constants in Defines.h


//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

// DirectX interface
LPDIRECT3D9             g_pD3D       = NULL; // Used to create the D3DDevice
LPDIRECT3DDEVICE9       g_pd3dDevice = NULL; // Our rendering device

// Camera
CCamera* MainCamera;

// Models
CModel* Cube  = NULL;
CModel* Floor = NULL;

// Textures for models
LPDIRECT3DTEXTURE9 CubeTexture;
LPDIRECT3DTEXTURE9 FloorTexture;

// Lights
const int   NumLights = 2;
CModel*     LightModels[NumLights];       // Each light has a model attached in this exercise
D3DXCOLOR   AmbientColour;                // Background light level
D3DXVECTOR3 LightPositions[NumLights];    // Point light positions
D3DXCOLOR   LightColours[NumLights];      // Point light colours
float       LightAttenuations[NumLights]; // Point light attenuations
float       SpecularPower = 256.0f;       // Specular power (shininess) of materials,
                                          // same for all models in this exercise
const float LightOrbit = 15.0f; // Controls orbiting light
const float LightSpeed = 0.01f; // -"-

// Shader variables, more shaders
LPDIRECT3DVERTEXSHADER9 VS_XformOnly,       VS_LightingTex;
LPD3DXCONSTANTTABLE     VS_XformOnlyConsts, VS_LightingTexConsts;
LPDIRECT3DPIXELSHADER9  PS_PlainColour,       PS_LightingTex;
LPD3DXCONSTANTTABLE     PS_PlainColourConsts, PS_LightingTexConsts;
                                                   
// Thread
HANDLE hThread;
CRITICAL_SECTION critical_section;  // shared structure
volatile bool RedrawFractal;
volatile bool ThreadShutDown;
//-----------------------------------------------------------------------------
// Light functions
//-----------------------------------------------------------------------------

// Initialise a light model for each light in the scene
bool InitialiseLightModels()
{
	for (int light = 0; light < NumLights; ++light)
	{
		LightModels[light] = new CModel();
		if (!LightModels[light]->Load( "Sphere.x" ))
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


// Create a point light given position and colour
void SetPointLight( int lightNum,  float posX, float posY, float posZ,  float r, float g, float b,
                    float attenuation )
{
	LightPositions[lightNum] = D3DXVECTOR3( posX, posY, posZ );
	LightColours[lightNum] = D3DXCOLOR( r, g, b, 1.0f );
	LightAttenuations[lightNum] = attenuation;
	LightModels[lightNum]->SetPosition( posX, posY, posZ );
}


// Set the given point light's position only
void SetPointLightPos( int lightNum,  float x, float y, float z )
{
	LightPositions[lightNum] = D3DXVECTOR3( x, y, z );
	LightModels[lightNum]->SetPosition( x, y, z ); // Match light model position to light
}


//-----------------------------------------------------------------------------
// Texture functions
//-----------------------------------------------------------------------------

// Load a texture
LPDIRECT3DTEXTURE9 LoadTexture( const string& fileName )
{
    LPDIRECT3DTEXTURE9 texture = NULL;
    
	// Create the texture
	if (FAILED(D3DXCreateTextureFromFile( g_pd3dDevice, fileName.c_str(), &texture )))
    {
		MessageBox(NULL, "Could not find texture map", "CO3303 Lab 15: Graphics Threading", MB_OK);
    }

	return texture;
}

// Create a dynamic texture of the given size - will have no mip-maps
LPDIRECT3DTEXTURE9 CreateDynamicTexture( int width, int height )
{
    LPDIRECT3DTEXTURE9 texture = NULL;
    
	// Create the texture
	if (FAILED(D3DXCreateTexture( g_pd3dDevice, width, height, 1, D3DUSAGE_DYNAMIC,
	                              D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &texture )))
    {
		MessageBox(NULL, "Cannot create dynamic texture map", "CO3303 Lab 19: Threading", MB_OK);
    }

	// Clear the texture
	D3DLOCKED_RECT lockedRect;
	texture->LockRect( 0, &lockedRect, NULL, 0 );
	memset( lockedRect.pBits, 0, lockedRect.Pitch * height );
	texture->UnlockRect( 0 );

	return texture;
}


// Copy the given data into a dynamic texture. Assumes texture and data is in D3DFMT_A8R8G8B8
// format and that data will cover entire texture area
void CopyToDynamicTexture( char* data, LPDIRECT3DTEXTURE9 texture )
{
	// Get texture information
	D3DSURFACE_DESC desc;
	texture->GetLevelDesc( 0, &desc );
	int rowLength = desc.Width * 4; // D3DFMT_A8R8G8B8 = 4 bytes

	// Lock entire texture
	D3DLOCKED_RECT lockedRect;
	texture->LockRect( 0, &lockedRect, NULL, 0 );

	// Copy each row
	char* textureRow = reinterpret_cast<char*>(lockedRect.pBits);
	for (unsigned int h = 0; h < desc.Height; ++h)
	{
		memcpy( textureRow, data, rowLength );
		data += rowLength;
		textureRow += lockedRect.Pitch;
	}

	// Unlock texture
	texture->UnlockRect( 0 );
}


// Set one of the textures to use for subsequent rendering
// Can select which texture to set - for shaders that use multiple textures
void SetTexture( int texNum, LPDIRECT3DTEXTURE9 texture )
{
    g_pd3dDevice->SetTexture( texNum, texture );
}


//*****************************************************************************
// Fractal Generation
//*****************************************************************************

// Size of fractal texture
const int FractalTexWidth = 512;
const int FractalTexHeight = 512;

// Areas to store fractal data
unsigned int FractalDepths[FractalTexHeight * FractalTexWidth];
unsigned int FractalPixels[FractalTexHeight * FractalTexWidth];

// Dimensions of the fractal area being generated
double FractalAreaLeft = -2.0;
double FractalAreaTop = -1.1;
double FractalAreaWidth = 2.5;
double FractalAreaHeight = 2.2;

// Flag if fractal needs to be recalculated
bool FractalDirty = true;

// For cycling colours
float FractalCycle = 0.0f; 


/////////////////////////////
// Fractal Area Movement

void FractalMoveX( double xOffset )
{
	FractalAreaLeft += xOffset * FractalAreaWidth;
	FractalDirty = true;
}

void FractalMoveY( double yOffset )
{
	FractalAreaTop += yOffset * FractalAreaHeight;
	FractalDirty = true;
}

void FractalZoomIn( double percent )
{
	double scale = 100.0 / percent;
	double newWidth = FractalAreaWidth * scale;
	double newHeight = FractalAreaHeight * scale;
	FractalAreaLeft += (FractalAreaWidth - newWidth) / 2.0;
	FractalAreaTop += (FractalAreaHeight - newHeight) / 2.0;
	FractalAreaWidth = newWidth;
	FractalAreaHeight = newHeight;
	FractalDirty = true;
}

void FractalZoomOut( double percent )
{
	FractalZoomIn( 10000.0 / percent );
	FractalDirty = true;
}


/////////////////////////////
// Fractal generation

// Draw Mandelbrot set. Formula using complex numbers:
//     c = ai + b, z(0) = 0, z(n+1) = z(n)^2 + c
// We plot a black colour for c if z(n) doesn't diverge, i.e. z(n) doesn't head off to infinity
// If z(n) diverges, we plot a colour for c based on how many steps it took us to realise the divergence
// Same formula using real numbers:
//     given a & b,  x(0) = y(0) = 0, x(n+1) = x(n)^2 - y(n)^2 + a, y(n+1) = 2x(n)y(n) + b
// We stop if n becomes too large or x(n)^2 + y(n)^2 >= 4 (for which we can guarantee divergence)
void DrawMandelbrot()
{
	// Calculate fractal into data areas
	unsigned int* pDepth;
	unsigned int* pPixel;

	// Step per-pixel
	double stepX = FractalAreaWidth / FractalTexWidth;
	double stepY = FractalAreaHeight / FractalTexHeight;

	// Calculate how large we will allow n depending on zoom level
	double stepMin = log( min( stepX, stepY ) ) / log( 2.0 );
	unsigned int depth = static_cast<unsigned int>(max( 15.0, -12.0 * stepMin - 45 ));

	// First calculate steps to diverge, only recalculate this (slow) stage if necessary
	if (FractalDirty)
	{
		unsigned int d;
		double p, q, r, s, t;

		// Per-pixel calculations
		pDepth = FractalDepths;
		double y = FractalAreaTop;
		for (unsigned int rows = FractalTexHeight; rows; --rows)
		{
			double x = FractalAreaLeft;
			for (unsigned int cols = FractalTexWidth; cols; --cols)
			{
				p = x;
				q = y;
				d = depth;
				do
				{
					// Pipeline optimised - split into tiny steps with minimal adjacent dependency
					s = q;
					r = p;
					t = 4.0;
					s *= q;
					r *= p;
					t -= s;
					q += q;
					if (t <= r)
					{
						break;
					}
					q *= p;
					r -= s;
					p = x;
					q += y;
					p += r;

				} while (--d > 0);
				*pDepth++ = d;

				x += stepX;
			}
			y += stepY;
		}
		FractalDirty = false;
	}
	
	// Convert steps to diverge into colours
	pDepth = FractalDepths;
	pPixel = FractalPixels;
	for (unsigned int rows = FractalTexHeight; rows; --rows)
	{
		for (unsigned int cols = FractalTexWidth; cols; --cols)
		{
			unsigned int d = *pDepth++;
			if (d == 0)
			{
				*pPixel = 0;
			}
			else
			{
				unsigned int level = static_cast<unsigned int>(FractalCycle + depth - d);
				unsigned int R, G, B;
				R = level & 0x1ff;
				G = (level * 3) & 0x1ff;
				B = (level * 7) & 0x1ff;
				if (R & 0x100)
				{
					R = 0x1ff - R;
				}
				if (G & 0x100)
				{
					G = 0x1ff - G;
				}
				if (B & 0x100)
				{
					B = 0x1ff - B;
				}
				*pPixel = (R << 16) | (G << 8) | B;
			}
			++pPixel;
		}
	}
}


/////////////////////////////
// Fractal texture update

// Draw fractal and copy it to the given texture, also cycle the colours
//****** Convert this function to a thread
unsigned int __stdcall FractalUpdate(void* x)
{
	while (true)
	{
		if (RedrawFractal)
		{
			EnterCriticalSection(&critical_section);
			DrawMandelbrot();
			LeaveCriticalSection(&critical_section);
			FractalCycle += 0.3f;
			RedrawFractal = false;
		}
	}

	return 0;
}

//void FractalUpdate()
//{
//	DrawMandelbrot();
//	FractalCycle += 0.3f;
//}


//-----------------------------------------------------------------------------
// Scene management
//-----------------------------------------------------------------------------

// Creates the scene geometry
bool SceneSetup()
{
	// Create camera
	MainCamera = new CCamera();
	MainCamera->SetPosition( -16.0f, 25.0f, -50.0f );
	MainCamera->SetRotation( ToRadians(13.0f), 0.0f, 0.0f );

	// Load models from files
	Cube = new CModel;
	Floor = new CModel;
	if (!Floor->Load( "Floor.x" ) || !Cube->Load( "Cube.x" ))
	{
		return false;  // Return on failure
	}
	Cube->SetPosition( 0.0f, 15.0f, 0.0f );

	// Load textures to apply to models
	CubeTexture = CreateDynamicTexture( FractalTexWidth, FractalTexHeight );
	FloorTexture = LoadTexture( "wood.jpg" );
	if (!CubeTexture || !FloorTexture)
	{
		return false;
	}

    // Create lights - the pixel shader in this example will handle two point lights
	// Will also load and display models for the lights in this exercise
	if (!InitialiseLightModels())
	{
		return false;
	}
	SetAmbientColour( 0.5f, 0.5f, 0.5f );
	SetPointLight( 0,  LightOrbit, 15.0f, 0.0f,  1.0f, 1.0f, 1.0f,  10.0f );
	SetPointLight( 1,  -60.0f, 30.0f, 60.0f,     1.0f, 0.9f, 0.2f,  100.0f );

	// Load and compile shaders. Two shader pairs here, a pixel lighting effect for the main 
	// models and a simple plain colour effect for the light models
	if (!LoadVertexShader( "XformOnly.vsh", &VS_XformOnly, &VS_XformOnlyConsts ) ||
	    !LoadVertexShader( "PixelLitTex.vsh", &VS_LightingTex, &VS_LightingTexConsts ) ||
	    !LoadPixelShader( "PlainColour.psh", &PS_PlainColour, &PS_PlainColourConsts ) ||
	    !LoadPixelShader( "PixelLit2Tex.psh", &PS_LightingTex, &PS_LightingTexConsts ))
	{
		return false;
	}

	ThreadShutDown = false;
	// create handle
	HANDLE hThread = // Handle of the thread, used to interact with it
		reinterpret_cast<HANDLE>(_beginthreadex(nullptr,        // Default security attributes
			0,           // Default stack size - OK unless using deep recursion
						 // or large local variables, both use stack memory
			FractalUpdate /********MISSING - not 0*/,  // Thread entry function 
			nullptr		/********MISSING - not 0*/, // Data to initialise thread
			0,           // Initial thread state (0 = running)
			nullptr));      // Can optionally return a thread UID, not needed here
	if (hThread == nullptr)
	{
		return -1; // Failure creating thread
	}
	InitializeCriticalSection(&critical_section);
	

	return true;
}

// Release everything in the scene
void SceneShutdown()
{
	DeleteCriticalSection(&critical_section);
	CloseHandle(hThread);
	// Release DirectX allocated objects
	// Using a DirectX helper macro to simplify code here - look it up in Defines.h
	SAFE_RELEASE( PS_LightingTexConsts );
	SAFE_RELEASE( PS_LightingTex );
	SAFE_RELEASE( PS_PlainColourConsts );
	SAFE_RELEASE( PS_PlainColour );
	SAFE_RELEASE( VS_LightingTexConsts );
	SAFE_RELEASE( VS_LightingTex );
	SAFE_RELEASE( VS_XformOnlyConsts );
	SAFE_RELEASE( VS_XformOnly );
	SAFE_RELEASE( FloorTexture );
	SAFE_RELEASE( CubeTexture );

	// Release light models
	UninitialiseLightModels();

	// Delete dynamically allocated objects. Our own types - no need to use DirectX release code
	delete Floor;
	delete Cube;
	delete MainCamera; 
}


//-----------------------------------------------------------------------------
// Game loop functions
//-----------------------------------------------------------------------------

// Draw one frame of the scene
void RenderScene()
{
    // Clear the back-buffer and the z-buffer
    g_pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                         D3DCOLOR_XRGB(128,128,128), 1.0f, 0 );

    // Begin the scene
    if (SUCCEEDED(g_pd3dDevice->BeginScene()))
    {
		//////////////////////////
		// Common model settings

		// Select shaders
		SetVertexShader( VS_LightingTex );
		SetPixelShader( PS_LightingTex );

		// Calculate the view and projection matrices for the camera, then pass combined
		// view-projection matrix to the vertex shader
		MainCamera->CalculateMatrices();
		D3DXMATRIXA16 viewProjMatrix = MainCamera->GetViewProjectionMatrix();
		VS_LightingTexConsts->SetMatrix( g_pd3dDevice, "ViewProjMatrix", &viewProjMatrix );
		
		// Pass lighting information to *pixel* shader
		PS_LightingTexConsts->SetFloatArray( g_pd3dDevice, "AmbientLight", (float*)&AmbientColour, 3 );
		PS_LightingTexConsts->SetFloatArray( g_pd3dDevice, "Light1Position", (float*)&LightPositions[0], 3 );
		PS_LightingTexConsts->SetFloatArray( g_pd3dDevice, "Light1Colour", (float*)&LightColours[0], 3 );
		PS_LightingTexConsts->SetFloat( g_pd3dDevice, "Light1Brightness", LightAttenuations[0] );
		PS_LightingTexConsts->SetFloatArray( g_pd3dDevice, "Light2Position", (float*)&LightPositions[1], 3 );
		PS_LightingTexConsts->SetFloatArray( g_pd3dDevice, "Light2Colour", (float*)&LightColours[1], 3 );
		PS_LightingTexConsts->SetFloat( g_pd3dDevice, "Light2Brightness", LightAttenuations[1] );
		PS_LightingTexConsts->SetFloatArray( g_pd3dDevice, "CameraPosition", (float*)&MainCamera->GetPosition(), 3 );
		PS_LightingTexConsts->SetFloat( g_pd3dDevice, "SpecularPower", SpecularPower );


		////////////////////
		// Floor rendering

		// Calculate world matrix for floor and pass it to the vertex shader
		Floor->CalculateMatrix();
		D3DXMATRIXA16 worldMatrix = Floor->GetWorldMatrix();
		VS_LightingTexConsts->SetMatrix( g_pd3dDevice, "WorldMatrix", &worldMatrix );

		// Select a texture for the floor and render it
		SetTexture( 0, FloorTexture );
		Floor->Render();


		////////////////////
		// Cube rendering

		// Copy the fractal pixels to the cube texture
		CopyToDynamicTexture( (char*)FractalPixels, CubeTexture );
		
		// threads
		RedrawFractal = true;

		// Calculate world matrix for cube and pass it to the vertex shader
		Cube->CalculateMatrix();
		worldMatrix = Cube->GetWorldMatrix();
		VS_LightingTexConsts->SetMatrix( g_pd3dDevice, "WorldMatrix", &worldMatrix );

		// Select a texture and render cube
		SetTexture( 0, CubeTexture );
		Cube->Render();


		//////////////////////////
		// Light model rendering

		// Select different shaders - light models are rendering in plain colour
		SetVertexShader( VS_XformOnly );
		SetPixelShader( PS_PlainColour );
		
		// Render each light in turn
		for (int light = 0; light < NumLights; ++light)
		{
			// The light's vertex shader requires a single combined world/view/projection
			// matrix. The view/projection matrix has already been calculated from the camera.
			// Calculate world matrix for current light and combine into a single matrix before
			// passing to the light's vertex shader
			LightModels[light]->CalculateMatrix();
			D3DXMATRIXA16 worldViewProjMatrix = LightModels[light]->GetWorldMatrix() * viewProjMatrix;
			VS_XformOnlyConsts->SetMatrix( g_pd3dDevice, "WorldViewProjMatrix", &worldViewProjMatrix );

			// Use light colour for model plain colour - set in pixel shader
			PS_PlainColourConsts->SetFloatArray( g_pd3dDevice, "MaterialColour", (float*)&LightColours[light], 3 );

			LightModels[light]->Render();
		}


        // End the scene
        g_pd3dDevice->EndScene();
	}

    // Present the backbuffer contents to the display
    g_pd3dDevice->Present( NULL, NULL, NULL, NULL );
}


// Update the scene between rendering
void UpdateScene()
{
	// Move the camera with keys
	// Keys in order - rotation: up,down,left,right then movement: forward,backward,left,right
	MainCamera->Control( Key_Up, Key_Down, Key_Left, Key_Right, Key_W, Key_S, Key_A, Key_D );

	// Move the models with keys
	// Keys in order - rotation: up,down,left,right,CCW,CW then movement: forward,backward
	Cube->Control( Key_I, Key_K, Key_J, Key_L, Key_U, Key_O, Key_Period, Key_Comma );

	// One light follows an orbit
	static float Rotate = 0.0f;
	SetPointLightPos( 0, cos(Rotate) * LightOrbit, 15.0f, sin(Rotate) * LightOrbit  );
	Rotate -= LightSpeed;

	if (TryEnterCriticalSection(&critical_section)) {
		// Fractal movement
		if (KeyHeld(Key_Numpad6))
		{
			FractalMoveX(0.1);
		}
		if (KeyHeld(Key_Numpad4))
		{
			FractalMoveX(-0.1);
		}
		if (KeyHeld(Key_Numpad2))
		{
			FractalMoveY(0.1);
		}
		if (KeyHeld(Key_Numpad8))
		{
			FractalMoveY(-0.1);
		}
		if (KeyHeld(Key_Numpad3))
		{
			FractalZoomIn(110.0);
		}
		if (KeyHeld(Key_Numpad1))
		{
			FractalZoomOut(110.0);
		}
		LeaveCriticalSection(&critical_section);
	}
	//******** Remove this call when switching to a threaded solution
	// FractalUpdate();
	// WaitForSingleObject(hThread, 0);
}


//-----------------------------------------------------------------------------
// D3D management
//-----------------------------------------------------------------------------

// Initialise Direct3D
bool D3DSetup( HWND hWnd )
{
    // Create the D3D object.
    g_pD3D = Direct3DCreate9( D3D_SDK_VERSION );
	if (!g_pD3D)
	{
        return false;
	}

    // Set up the structure used to create the D3DDevice
    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;  // Wait for vertical sync
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;               // Use desktop pixel format (windowed mode)
    d3dpp.BackBufferCount = 1;
    d3dpp.EnableAutoDepthStencil = TRUE;         // Create depth buffer (Z)
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8; // Allow 24 bits of depth buffer, 8 bits of stencil

    // Create the D3DDevice, would also need to specify D3DCREATE_MULTITHREADED if using DirectX
	// functions on a different thread than this call - not recommended (and not done here)
    if (FAILED(g_pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
                                     D3DCREATE_HARDWARE_VERTEXPROCESSING,// | D3DCREATE_MULTITHREADED,
                                     &d3dpp, &g_pd3dDevice )))
    {
        return false;
    }

    // Turn on tri-linear filtering for everything
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );

	return true;
}


// Uninitialise D3D
void D3DShutdown()
{
	// Release D3D interfaces
    if( g_pd3dDevice != NULL )
        g_pd3dDevice->Release();

    if( g_pD3D != NULL )
        g_pD3D->Release();
}


//-----------------------------------------------------------------------------
// Windows functions
//-----------------------------------------------------------------------------

// Window message handler
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


// Windows main function
INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR, INT )
{
    // Register the window class (adding our own icon to this window)
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L,
                      GetModuleHandle(NULL), LoadIcon( hInst, MAKEINTRESOURCE(IDI_TL) ),
					  LoadCursor( NULL, IDC_ARROW ), NULL, NULL,
                      "GraphicsThread", NULL };
    RegisterClassEx( &wc );

    // Create the application's window
	HWND hWnd = CreateWindow( "GraphicsThread", "CO3301: Graphics Threading",
                              WS_OVERLAPPEDWINDOW, 100, 100, 1280, 960,
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
					if (KeyHeld( Key_Escape ))
					{
						DestroyWindow( hWnd );
					}
				}
            }
        }
	    SceneShutdown();
    }
	D3DShutdown();

	UnregisterClass( "GraphicsThread", wc.hInstance );
    return 0;
}
