/*******************************************
	Shader.h

	Shader Support Declarations
********************************************/

#pragma once

#include <string>
using namespace std;

#include <d3dx9.h>

// Flags used when creating shaders - change value to enable shader debugging
const DWORD SHADER_FLAGS = 0;
//const DWORD SHADER_FLAGS = D3DXSHADER_DEBUG | D3DXSHADER_SKIPOPTIMIZATION;

//-----------------------------------------------------------------------------
// Vertex Shaders
//-----------------------------------------------------------------------------

// Load and compiler a HLSL vertex shader from a file. Provide the source code filename and pointers
// to the variables to hold the resultant shader and it associated constant table
bool LoadVertexShader( const string& fileName, LPDIRECT3DVERTEXSHADER9* vertexShader,
					   LPD3DXCONSTANTTABLE* constants );

// Use the given vertex shader for subsequent rendering
void SetVertexShader( LPDIRECT3DVERTEXSHADER9 vertexShader );


//-----------------------------------------------------------------------------
// Pixel Shaders
//-----------------------------------------------------------------------------

// Load and compiler a HLSL pixel shader from a file. Provide the source code filename and pointers
// to the variables to hold the resultant shader and it associated constant table
bool LoadPixelShader( const string& fileName, LPDIRECT3DPIXELSHADER9* pixelShader,
					  LPD3DXCONSTANTTABLE* constants );

// Use the given pixel shader for subsequent rendering
void SetPixelShader( LPDIRECT3DPIXELSHADER9 pixelShader );
