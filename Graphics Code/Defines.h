/*******************************************
	Defines.h

	General definitions for the project
********************************************/

#pragma once // Prevent file being included more than once (would cause errors)

#include <Windows.h>
#include <d3d10.h>  // Updated header files of course
#include <d3dx10.h> // --"--

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

// Move speed constants
const float MoveSpeed = 60.0f;
const float RotSpeed = 2.0f;


//-----------------------------------------------------------------------------
// Helper functions and macros
//-----------------------------------------------------------------------------

// Helper macro to release DirectX pointers only if they are not NULL
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p) = NULL; } }

// Angular helper functions to convert from degrees to radians and back
// D3DX_PI is a double in DX10
inline float ToRadians( float deg ) { return deg * (float)D3DX_PI / 180.0f; }
inline float ToDegrees( float rad ) { return rad * 180.0f / (float)D3DX_PI; }


//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

// Make DirectX render device available to other source files. We declare this
// variable global in one file, then make it "extern" to all others. In general
// this is not good practice - this is a kind of super-global variable. Would
// be better to have a Device class responsible for this data. However, this
// example aims for a minimum of code to help demonstrate the focus topic
extern ID3D10Device* g_pd3dDevice; // New type for DX10
