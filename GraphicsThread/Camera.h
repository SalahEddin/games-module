/**********************************************
	Camera.h

	Declaration of camera class for DirectX
***********************************************/

#pragma once // Prevent file being included more than once (would cause errors)

#include <d3d9.h>
#include <d3dx9.h>
#include "Input.h"

//-----------------------------------------------------------------------------
// DirectX Camera Class Defintition
//-----------------------------------------------------------------------------

class CCamera
{
/////////////////////////////
// Public member functions
public:

	///////////////////////////////
	// Constructors / Destructors

	// Constructor
	CCamera( float posX = 0.0f, float posY = 0.0f , float posZ = 0.0f,
	         float rotX = 0.0f, float rotY = 0.0f, float rotZ = 0.0f, 
	         float nearClip = 0.1f, float farClip = 10000.0f );


	/////////////////////////////
	// Data access

	// Getters
	D3DXVECTOR3 GetPosition()
	{
		return m_Position;
	}

	D3DXMATRIXA16 GetViewMatrix()
	{
		return m_MatView;
	}
	D3DXMATRIXA16 GetProjectionMatrix()
	{
		return m_MatProj;
	}
	D3DXMATRIXA16 GetViewProjectionMatrix()
	{
		return m_MatViewProj;
	}

	// Setters
	void SetPosition( float x, float y, float z )
	{
		m_Position = D3DXVECTOR3( x, y, z );
	}
	void SetRotation( float x, float y, float z )
	{
		m_Rotation = D3DXVECTOR3( x, y, z );
	}
	void SetNearFarClip( float nearClip, float farClip )
	{
		m_NearClip = nearClip;
		m_FarClip = farClip;
	}


	/////////////////////////////
	// Camera Usage

	// Calculate view, projection & combined view-projection matrices for the camera
	void CalculateMatrices();

	// Controls the camera - uses the current view matrix for local movement
	void Control( EKeyCode turnUp, EKeyCode turnDown,
	              EKeyCode turnLeft, EKeyCode turnRight,  
	              EKeyCode moveForward, EKeyCode moveBackward,
	              EKeyCode moveLeft, EKeyCode moveRight);


/////////////////////////////
// Private member variables
private:

	// Postition and rotations for the camera (using DirectX vector types)
	D3DXVECTOR3   m_Position;
	D3DXVECTOR3   m_Rotation;

	// Near and far clip plane distances
	float         m_NearClip;
	float         m_FarClip;

	// Current view, projection and combined view-projection matrices (DirectX matrix type)
	D3DXMATRIXA16 m_MatView;
	D3DXMATRIXA16 m_MatProj;
	D3DXMATRIXA16 m_MatViewProj;
};

