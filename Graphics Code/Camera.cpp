/**********************************************
	Camera.cpp

	Implementation of camera class for DirectX
***********************************************/

#include "Defines.h" // General definitions for all source files
#include "Camera.h"  // Declaration of this

///////////////////////////////
// Constructors / Destructors

// Constructor
CCamera::CCamera( float posX, float posY, float posZ,
                  float rotX, float rotY, float rotZ,
                  float nearClip, float farClip, float fov, float aspect )
{
	SetPosition( posX, posY, posZ );
	SetRotation( rotX, rotY, rotZ );
	SetNearFarClip( nearClip, farClip );
	SetFOV( fov );
	SetAspect( aspect );
}


///////////////////////////////
// Camera Usage

// Calculate view, projection & combined view-projection matrices for the camera
void CCamera::CalculateMatrices()
{
	// Set up the view matrix (reverse signs and multiplication to create inverse)
	D3DXMATRIX MatScale, MatRotX, MatRotY, MatRotZ, MatTrans;
	D3DXMatrixRotationX( &MatRotX, -m_Rotation.x );
	D3DXMatrixRotationY( &MatRotY, -m_Rotation.y );
	D3DXMatrixRotationZ( &MatRotZ, -m_Rotation.z );
	D3DXMatrixTranslation( &MatTrans, -m_Position.x, -m_Position.y, -m_Position.z);
	m_MatView = MatTrans * MatRotY * MatRotX * MatRotZ;

	// For the projection matrix, we set up a perspective transform (which
	// transforms geometry from 3D view space to 2D viewport space, with
	// a perspective divide making objects smaller in the distance). To build
	// a perpsective transform, we need the field of view (1/4 pi is common),
	// the aspect ratio, and the near and far clipping planes (which define at
	// what distances geometry should be no longer be rendered).
	D3DXMatrixPerspectiveFovLH( &m_MatProj, m_FOV, m_Aspect, m_NearClip, m_FarClip ); // D3DX_PI is a double in DX10

	// In DX10 there is no need to send the projection matrix to the fixed pipeline for fogging
	// In fact DX10 has no fixed pipeline at all

	// Combine the view and projection matrix into a single matrix - this will
	// be passed to vertex shaders as a single matrix (more efficient this way)
	m_MatViewProj = m_MatView * m_MatProj;
}


// Control the camera using keys
void CCamera::Control( EKeyCode turnUp, EKeyCode turnDown,
                       EKeyCode turnLeft, EKeyCode turnRight,  
                       EKeyCode moveForward, EKeyCode moveBackward,
                       EKeyCode moveLeft, EKeyCode moveRight, float updateTime )
{
	if (KeyHeld( turnDown ))
	{
		m_Rotation.x += RotSpeed * updateTime;
	}
	if (KeyHeld( turnUp ))
	{
		m_Rotation.x -= RotSpeed * updateTime;
	}
	if (KeyHeld( turnRight ))
	{
		m_Rotation.y += RotSpeed * updateTime;
	}
	if (KeyHeld( turnLeft ))
	{
		m_Rotation.y -= RotSpeed * updateTime;
	}

	// Local X movement - move in the direction of the X axis, get axis from view matrix
	if (KeyHeld( moveRight ))
	{
		m_Position.x += m_MatView._11 * MoveSpeed * updateTime;
		m_Position.y += m_MatView._21 * MoveSpeed * updateTime;
		m_Position.z += m_MatView._31 * MoveSpeed * updateTime;
	}
	if (KeyHeld( moveLeft ))
	{
		m_Position.x -= m_MatView._11 * MoveSpeed * updateTime;
		m_Position.y -= m_MatView._21 * MoveSpeed * updateTime;
		m_Position.z -= m_MatView._31 * MoveSpeed * updateTime;
	}

	// Local Z movement - move in the direction of the Z axis, get axis from view matrix
	if (KeyHeld( moveForward ))
	{
		m_Position.x += m_MatView._13 * MoveSpeed * updateTime;
		m_Position.y += m_MatView._23 * MoveSpeed * updateTime;
		m_Position.z += m_MatView._33 * MoveSpeed * updateTime;
	}
	if (KeyHeld( moveBackward ))
	{
		m_Position.x -= m_MatView._13 * MoveSpeed * updateTime;
		m_Position.y -= m_MatView._23 * MoveSpeed * updateTime;
		m_Position.z -= m_MatView._33 * MoveSpeed * updateTime;
	}
}
