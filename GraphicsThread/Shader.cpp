/*******************************************
	Shader.cpp

	Shader Support Implementation
********************************************/

#include "Defines.h"
#include "Shader.h"

//-----------------------------------------------------------------------------
// Vertex Shaders
//-----------------------------------------------------------------------------

// Load and compiler a HLSL vertex shader from a file. Provide the source code filename and pointers
// to the variables to hold the resultant shader and it associated constant table
bool LoadVertexShader( const string& fileName, LPDIRECT3DVERTEXSHADER9* vertexShader,
					   LPD3DXCONSTANTTABLE* constants )
{
	//**********************************
	// Step 1: Compile shader program

	// Temporary variable to hold compiled pixel shader code
    LPD3DXBUFFER pShaderCode;

	// Compile external HLSL pixel shader into shader code to submit to the hardware
	HRESULT hr = 
		D3DXCompileShaderFromFile( fileName.c_str(), // File containing pixel shader (HLSL)
			                       NULL, NULL,       // Advanced compilation options - not needed here
								   "main",           // Name of main function in the shader
								   "vs_2_0",         // Target vertex shader hardware - vs_1_1 is lowest level
												     // and will work on all video cards with a pixel shader
								   SHADER_FLAGS,     // Additional compilation flags (such as debug flags)
								   &pShaderCode,     // Ptr to variable to hold compiled shader code
								   NULL,             // Ptr to variable to hold error messages (not needed)
								   constants );      // Ptr to variable to hold constants for the shader
    if (FAILED(hr))
	{
		// Return if compilation failed
		return false;
	}

	//***************************************************
	// Step 2: Convert compiled program to usable shader

	// Create the pixel shader using the compiled shader code
    hr = g_pd3dDevice->CreateVertexShader( (DWORD*)pShaderCode->GetBufferPointer(), vertexShader );
    
	// Discard the shader code now the shader has been created 
	pShaderCode->Release();

	// If the creation failed then return (wait until after shader code has been discarded)
    if (FAILED(hr))
	{
		return false;
	}

	return true;
}

// Use the given vertex shader for subsequent rendering
void SetVertexShader( LPDIRECT3DVERTEXSHADER9 vertexShader )
{
	g_pd3dDevice->SetVertexShader( vertexShader );
}


//-----------------------------------------------------------------------------
// Pixel Shaders
//-----------------------------------------------------------------------------

// Load and compiler a HLSL pixel shader from a file. Provide the source code filename and pointers
// to the variables to hold the resultant shader and it associated constant table
bool LoadPixelShader( const string& fileName, LPDIRECT3DPIXELSHADER9* pixelShader,
					  LPD3DXCONSTANTTABLE* constants )
{
	//**********************************
	// Step 1: Compile shader program

	// Temporary variable to hold compiled pixel shader code
    LPD3DXBUFFER pShaderCode;

	// Compile external HLSL pixel shader into shader code to submit to the hardware
	HRESULT hr = 
		D3DXCompileShaderFromFile( fileName.c_str(), // File containing pixel shader (HLSL)
			                       NULL, NULL,       // Advanced compilation options - not needed here
								   "main",           // Name of main function in the shader
								   "ps_2_0",         // Target pixel shader hardware - ps_1_1 is lowest level
												     // ps_2_0 works on most modern video cards, ps_4_0 is best (currently)
								   0,                // Additional compilation flags (such as debug flags)
								   &pShaderCode,     // Ptr to variable to hold compiled shader code
								   NULL,             // Ptr to variable to hold error messages (not needed)
								   constants );      // Ptr to variable to hold constants for the shader
    if (FAILED(hr))
	{
		// Return if compilation failed
		return false;
	}

	//***************************************************
	// Step 2: Convert compiled program to usable shader

	// Create the pixel shader using the compiled shader code
    hr = g_pd3dDevice->CreatePixelShader( (DWORD*)pShaderCode->GetBufferPointer(), pixelShader );
    
	// Discard the shader code now the shader has been created 
	pShaderCode->Release();

	// If the creation failed then return (wait until after shader code has been discarded)
    if (FAILED(hr))
	{
		return false;
	}

	return true;
}

// Use the given pixel shader for subsequent rendering
void SetPixelShader( LPDIRECT3DPIXELSHADER9 pixelShader )
{
	g_pd3dDevice->SetPixelShader( pixelShader );
}
