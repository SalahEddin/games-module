/*******************************************
	Shader.h

	Shader Support Declarations
********************************************/

#pragma once

#include <string>
using namespace std;

#include "Defines.h"


//-----------------------------------------------------------------------------
// Vertex Shaders
//-----------------------------------------------------------------------------

// Load and compiler a HLSL vertex shader from a file. Provide the source code filename and pointers
// to the variable to hold the resultant shader
bool LoadVertexShader( const string& fileName, ID3D10VertexShader** vertexShader, ID3D10Blob** shaderCode );

// Use the given vertex shader for subsequent rendering
void SetVertexShader( ID3D10VertexShader* vertexShader );


//-----------------------------------------------------------------------------
// Geometry Shaders
//-----------------------------------------------------------------------------

// Load and compiler a HLSL geometry shader from a file. Provide the source code filename and pointers
// to the variable to hold the resultant shader
bool LoadGeometryShader( const string& fileName, ID3D10GeometryShader** geometryShader );

// Load and compiler a HLSL geometry shader from a file, where the geometry shader will be using the stream out stage.
// Provide the source code filename and pointers to the variable to hold the resultant shader
// Also pass the stream out declaration, number of entries in the declaration and the size of each output element
bool LoadStreamOutGeometryShader( const string& fileName, D3D10_SO_DECLARATION_ENTRY* soDecl, unsigned int soNumEntries,
								  unsigned int soStride, ID3D10GeometryShader** geometryShader );

// Use the given geometry shader for subsequent rendering
void SetGeometryShader( ID3D10GeometryShader* geometryShader );


//-----------------------------------------------------------------------------
// Pixel Shaders
//-----------------------------------------------------------------------------

// Load and compiler a HLSL pixel shader from a file. Provide the source code filename and pointers
// to the variable to hold the resultant shader
bool LoadPixelShader( const string& fileName, ID3D10PixelShader** vertexShader );

// Use the given pixel shader for subsequent rendering
void SetPixelShader( ID3D10PixelShader* pixelShader );


//-----------------------------------------------------------------------------
// Constant Buffers
//-----------------------------------------------------------------------------

// New to DirectX 10 - see cpp file

/// Create and return a constant buffer of the given size
ID3D10Buffer* CreateConstantBuffer( int size );

// Set a single constant buffer for use by vertex shaders
void SetVertexConstantBuffer( ID3D10Buffer* constantBuffer );

// Set a single constant buffer for use by geometry shaders
void SetGeometryConstantBuffer( ID3D10Buffer* constantBuffer );

// Set a single constant buffer for use by pixel shaders
void SetPixelConstantBuffer( ID3D10Buffer* constantBuffer );