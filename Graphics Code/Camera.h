/**********************************************
	Camera.h

	Declaration of camera class for DirectX
***********************************************/

#pragma once // Prevent file being included more than once (would cause errors)

#include "Defines.h"
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
	         float nearClip = 0.1f, float farClip = 10000.0f, 
			 float fov = (float)D3DX_PI/4, float aspect = 1.3333f );


	/////////////////////////////
	// Data access

	// Getters
	D3DXVECTOR3 GetPosition()
	{
		return m_Position;
	}
	D3DXVECTOR3 GetFacing()
	{
		return D3DXVECTOR3( m_MatView._13, m_MatView._23, m_MatView._33 );
	}

	D3DXMATRIX GetViewMatrix()
	{
		return m_MatView;
	}
	D3DXMATRIX GetProjectionMatrix()
	{
		return m_MatProj;
	}
	D3DXMATRIX GetViewProjectionMatrix()
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
	void SetFOV( float fov )
	{
		m_FOV = fov;
	}
	void SetAspect( float aspect )
	{
		m_Aspect = aspect;
	}


	/////////////////////////////
	// Camera Usage

	// Calculate view, projection & combined view-projection matrices for the camera
	void CalculateMatrices();

	// Controls the camera - uses the current view matrix for local movement
	void Control( EKeyCode turnUp, EKeyCode turnDown,
	              EKeyCode turnLeft, EKeyCode turnRight,  
	              EKeyCode moveForward, EKeyCode moveBackward,
	              EKeyCode moveLeft, EKeyCode moveRight, float updateTime );


/////////////////////////////
// Private member variables
private:

	// Postition and rotations for the camera (using DirectX vector types)
	D3DXVECTOR3   m_Position;
	D3DXVECTOR3   m_Rotation;

	// Near and far clip plane distances
	float         m_NearClip;
	float         m_FarClip;

	// Viewport FOV & aspect ratio
	float         m_FOV;
	float         m_Aspect;

	// Current view, projection and combined view-projection matrices (DirectX matrix type)
	D3DXMATRIX m_MatView;
	D3DXMATRIX m_MatProj;
	D3DXMATRIX m_MatViewProj;
};

