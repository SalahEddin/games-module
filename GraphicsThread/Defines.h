/*******************************************
	Defines.h

	General definitions for the project
********************************************/

#pragma once // Prevent file being included more than once (would cause errors)

#include <Windows.h>
#include <d3d9.h>
#include <d3dx9.h>

//-----------------------------------------------------------------------------
// Movement / rotation definitions
//-----------------------------------------------------------------------------

// Angular helper functions to convert from degrees to radians and back
inline float ToRadians( float deg ) { return deg * D3DX_PI / 180.0f; }
inline float ToDegrees( float rad ) { return rad * 180.0f / D3DX_PI; }

// Move speed constants
const float MoveSpeed = 0.6f;
const float RotSpeed = 0.02f;


//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

// Make DirectX render device available to other source files. We declare this
// variable global in one file, then make it "extern" to all others. In general
// this is not good practice - this is a kind of super-global variable. Would
// be better to have a Device class responsible for this data. However, this
// example aims for a minimum of code to help demonstrate the focus topic
extern LPDIRECT3DDEVICE9 g_pd3dDevice;


//-----------------------------------------------------------------------------
// Helper macros
//-----------------------------------------------------------------------------

// A macro to simplify releasing of DirectX resources - taken from the DirectX helper libraries
// If the resource pointer is not NULL, it is released then set to NULL
#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p)=NULL; } }
