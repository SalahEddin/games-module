/*******************************************
	Shader.cpp

	Shader Support Implementation
********************************************/

#include "Defines.h"
#include "BaseMath.h"
#include "Shader.h"

//-----------------------------------------------------------------------------
// Vertex Shaders
//-----------------------------------------------------------------------------

// Load and compiler a HLSL vertex shader from a file. Provide the source code filename and pointers
// to the variable to hold the resultant shader
// DX10 does not use per-shader constant tables like DX9 so no equivalent parameter needed
// Need to also retain and return the intermediate compiled vertex shader code under DX10 (see main code)
bool LoadVertexShader( const string& fileName, ID3D10VertexShader** vertexShader, ID3D10Blob** shaderCode )
{
	//**********************************
	// Step 1: Compile shader program

	// Temporary variable to hold error messages
	ID3D10Blob* errors;

	// Compile external HLSL shader into shader code to submit to the hardware
	HRESULT hr = 
		D3DX10CompileFromFile( fileName.c_str(), // File containing pixel shader (HLSL)
		                       NULL, NULL,       // Advanced compilation options - not needed here
		                       "main",           // Name of entry point in the shader
		                       "vs_4_0",         // Target vertex shader hardware - vs_1_1 is lowest level
		                                         // vs_2_0 works on most modern video cards, vs_4_0 required for DX10
		                       0,                // Additional compilation flags (such as debug flags)
		                       0,                // More compilation flags (added in DX10)
		                       NULL,             // Parameter to run compilation concurrently 
		                       shaderCode,       // Ptr to variable to hold compiled shader code
		                       &errors,             // Ptr to variable to hold error messages (not needed)
		                       NULL );           // Ptr to variable to hold result if compiling concurrently
	if (FAILED(hr))
	{
		if (errors)
		{
			void* errorMsg = errors->GetBufferPointer(); // This pointer points at the shader error message in case errors don't appear in the output window
			MessageBox( NULL, (char*)errorMsg, "Error", MB_OK );
			errors->Release();
		}

		// Return if compilation failed
		return false;
	}

	//***************************************************
	// Step 2: Convert compiled program to usable shader
	// Create the pixel shader using the compiled shader code
	hr = g_pd3dDevice->CreateVertexShader( (DWORD*)(*shaderCode)->GetBufferPointer(), (*shaderCode)->GetBufferSize(), 
	                                       vertexShader );
	
	// Don't discard vertex shader code under DX10

	// If the creation failed then return (wait until after shader code has been discarded)
	if (FAILED(hr))
	{
		return false;
	}

	return true;
}

// Use the given vertex shader for subsequent rendering
void SetVertexShader( ID3D10VertexShader* vertexShader )
{
	g_pd3dDevice->VSSetShader( vertexShader );
}


//-----------------------------------------------------------------------------
// Geometry Shaders
//-----------------------------------------------------------------------------

// Load and compiler a HLSL geometry shader from a file. Provide the source code filename and pointers
// to the variable to hold the resultant shader
bool LoadGeometryShader( const string& fileName, ID3D10GeometryShader** geometryShader )
{
	//**********************************
	// Step 1: Compile shader program

	// Temporary variable to hold compiled shader code and error messages
	ID3D10Blob* shaderCode;
	ID3D10Blob* errors;

	// Compile external HLSL shader into shader code to submit to the hardware
	HRESULT hr = 
		D3DX10CompileFromFile( fileName.c_str(), // File containing geometry shader (HLSL)
		                       NULL, NULL,       // Advanced compilation options - not needed here
		                       "main",           // Name of entry point in the shader
		                       "gs_4_0",         // Target geometry shader hardware - ps_1_1 is lowest level
		                                         // ps_2_0 works on most modern video cards, ps_4_0 required for DX10
		                       0,                // Additional compilation flags (such as debug flags)
		                       0,                // More compilation flags (added in DX10)
		                       NULL,             // Parameter to run compilation concurrently 
		                       &shaderCode,      // Ptr to variable to hold compiled shader code
		                       &errors,          // Ptr to variable to hold error messages
		                       NULL );           // Ptr to variable to hold result if compiling concurrently
	if (FAILED(hr))
	{
		if (errors)
		{
			void* errorMsg = errors->GetBufferPointer(); // This pointer points at the shader error message in case errors don't appear in the output window
			MessageBox( NULL, (char*)errorMsg, "Error", MB_OK );
			errors->Release();
		}

		// Return if compilation failed
		return false;
	}

	//***************************************************
	// Step 2: Convert compiled program to usable shader

	// Create the geometry shader using the compiled shader code
	hr = g_pd3dDevice->CreateGeometryShader( (DWORD*)shaderCode->GetBufferPointer(), shaderCode->GetBufferSize(), 
	                                          geometryShader );
	
	// Discard the shader code now the shader has been created 
	shaderCode->Release();

	// If the creation failed then return (wait until after shader code has been discarded)
	if (FAILED(hr))
	{
		return false;
	}

	return true;
}

// Load and compiler a HLSL geometry shader from a file, where the geometry shader will be using the stream out stage.
// Provide the source code filename and pointers to the variable to hold the resultant shader
// Also pass the stream out declaration, number of entries in the declaration and the size of each output element
bool LoadStreamOutGeometryShader( const string& fileName, D3D10_SO_DECLARATION_ENTRY* soDecl, unsigned int soNumEntries,
								  unsigned int soStride, ID3D10GeometryShader** geometryShader )
{
	//**********************************
	// Step 1: Compile shader program

	// Temporary variable to hold compiled shader code and error messages
	ID3D10Blob* shaderCode;
	ID3D10Blob* errors;

	// Compile external HLSL shader into shader code to submit to the hardware
	HRESULT hr = 
		D3DX10CompileFromFile( fileName.c_str(), // File containing geometry shader (HLSL)
		                       NULL, NULL,       // Advanced compilation options - not needed here
		                       "main",           // Name of entry point in the shader
		                       "gs_4_0",         // Target geometry shader hardware - ps_1_1 is lowest level
		                                         // ps_2_0 works on most modern video cards, ps_4_0 required for DX10
		                       0,                // Additional compilation flags (such as debug flags)
		                       0,                // More compilation flags (added in DX10)
		                       NULL,             // Parameter to run compilation concurrently 
		                       &shaderCode,      // Ptr to variable to hold compiled shader code
		                       &errors,          // Ptr to variable to hold error messages
		                       NULL );           // Ptr to variable to hold result if compiling concurrently
	if (FAILED(hr))
	{
		if (errors)
		{
			void* errorMsg = errors->GetBufferPointer(); // This pointer points at the shader error message in case errors don't appear in the output window
			MessageBox( NULL, (char*)errorMsg, "Error", MB_OK );
			errors->Release();
		}

		// Return if compilation failed
		return false;
	}

	//***************************************************
	// Step 2: Convert compiled program to usable shader

	// Create the geometry shader using the compiled shader code, also pass on the stream out information
	hr = g_pd3dDevice->CreateGeometryShaderWithStreamOutput( (DWORD*)shaderCode->GetBufferPointer(), shaderCode->GetBufferSize(), 
	                                                         soDecl, soNumEntries, soStride, geometryShader );
	
	// Discard the shader code now the shader has been created 
	shaderCode->Release();

	// If the creation failed then return (wait until after shader code has been discarded)
	if (FAILED(hr))
	{
		return false;
	}

	return true;
}

// Use the given geometry shader for subsequent rendering
void SetGeometryShader( ID3D10GeometryShader* geometryShader )
{
	g_pd3dDevice->GSSetShader( geometryShader );
}


//-----------------------------------------------------------------------------
// Pixel Shaders
//-----------------------------------------------------------------------------

// Load and compiler a HLSL pixel shader from a file. Provide the source code filename and pointers
// to the variable to hold the resultant shader
// DX10 does not use per-shader constant tables like DX9 so no equivalent parameter needed
bool LoadPixelShader( const string& fileName, ID3D10PixelShader** pixelShader )
{
	//**********************************
	// Step 1: Compile shader program

	// Temporary variable to hold compiled shader code and error messages
	ID3D10Blob* shaderCode;
	ID3D10Blob* errors;

	// Compile external HLSL shader into shader code to submit to the hardware
	HRESULT hr = 
		D3DX10CompileFromFile( fileName.c_str(), // File containing pixel shader (HLSL)
		                       NULL, NULL,       // Advanced compilation options - not needed here
		                       "main",           // Name of entry point in the shader
		                       "ps_4_0",         // Target pixel shader hardware - ps_1_1 is lowest level
		                                         // ps_2_0 works on most modern video cards, ps_4_0 required for DX10
		                       0,                // Additional compilation flags (such as debug flags)
		                       0,                // More compilation flags (added in DX10)
		                       NULL,             // Parameter to run compilation concurrently 
		                       &shaderCode,      // Ptr to variable to hold compiled shader code
		                       &errors,          // Ptr to variable to hold error messages
		                       NULL );           // Ptr to variable to hold result if compiling concurrently
	if (FAILED(hr))
	{
		if (errors)
		{
			void* errorMsg = errors->GetBufferPointer(); // This pointer points at the shader error message in case errors don't appear in the output window
			MessageBox( NULL, (char*)errorMsg, "Error", MB_OK );
			errors->Release();
		}

		// Return if compilation failed
		return false;
	}

	//***************************************************
	// Step 2: Convert compiled program to usable shader

	// Create the pixel shader using the compiled shader code
	hr = g_pd3dDevice->CreatePixelShader( (DWORD*)shaderCode->GetBufferPointer(), shaderCode->GetBufferSize(), 
	                                      pixelShader );
	
	// Discard the shader code now the shader has been created 
	shaderCode->Release();

	// If the creation failed then return (wait until after shader code has been discarded)
	if (FAILED(hr))
	{
		return false;
	}

	return true;
}

// Use the given pixel shader for subsequent rendering
void SetPixelShader( ID3D10PixelShader* pixelShader )
{
	g_pd3dDevice->PSSetShader( pixelShader );
}


//-----------------------------------------------------------------------------
// Constant Buffers
//-----------------------------------------------------------------------------

// Constant buffers are new to DirectX 10. We now consider the global variables in a shader as a block of memory.
// We can then access the entire block from C++ to update values (rather than using constant tables, SetFloatArray etc)
// We typically set up a C++ structure to match the variables in the shader to make this easier (see main code)
// Also typical to share a single constant buffer across several shaders of the same type (vertex or pixel)
// It is possible to have multiple constant buffers in use - some that are updated more frequently. E.g. a buffer 
// containing the world matrix that will change for every model; and a buffer containing the camera matrices
// that will only change once per frame. This improves performance
// Create and return a constant buffer of the given size
ID3D10Buffer* CreateConstantBuffer( int size )
{
	D3D10_BUFFER_DESC cbDesc;
	cbDesc.ByteWidth = gen::Round( size, gen::kRoundUp, 16 ); // Constant buffer data is packed into float4 data - must round up to size of float4 (16)
	cbDesc.Usage = D3D10_USAGE_DYNAMIC;
	cbDesc.BindFlags = D3D10_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE; // CPU is only going to write to the constants (not read them)
	cbDesc.MiscFlags = 0;
	ID3D10Buffer* constantBuffer;
	if (FAILED(g_pd3dDevice->CreateBuffer( &cbDesc, NULL, &constantBuffer )))
	{
		return NULL;
	}
	return constantBuffer;
}


// Set a single constant buffer for use by vertex shaders
void SetVertexConstantBuffer( ID3D10Buffer* constantBuffer )
{
	g_pd3dDevice->VSSetConstantBuffers( 0, 1, &constantBuffer );
}

// Set a single constant buffer for use by geometry shaders
void SetGeometryConstantBuffer( ID3D10Buffer* constantBuffer )
{
	g_pd3dDevice->GSSetConstantBuffers( 0, 1, &constantBuffer );
}

// Set a single constant buffer for use by pixel shaders
void SetPixelConstantBuffer( ID3D10Buffer* constantBuffer )
{
	g_pd3dDevice->PSSetConstantBuffers( 0, 1, &constantBuffer );
}