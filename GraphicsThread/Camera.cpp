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
				  float nearClip, float farClip )
{
	SetPosition( posX, posY, posZ );
	SetRotation( rotX, rotY, rotZ );
	SetNearFarClip( nearClip, farClip );
}


///////////////////////////////
// Camera Usage

// Calculate view, projection & combined view-projection matrices for the camera
void CCamera::CalculateMatrices()
{
     // Set up the view matrix (reverse signs and multiplication to create inverse)
    D3DXMATRIXA16 MatScale, MatX, MatY, MatZ, MatTrans;
	D3DXMatrixRotationX( &MatX, -m_Rotation.x );
	D3DXMatrixRotationY( &MatY, -m_Rotation.y );
	D3DXMatrixRotationZ( &MatZ, -m_Rotation.z );
	D3DXMatrixTranslation( &MatTrans, -m_Position.x, -m_Position.y, -m_Position.z);
	m_MatView = MatTrans * MatY * MatX * MatZ;
    g_pd3dDevice->SetTransform( D3DTS_VIEW, &m_MatView );

    // For the projection matrix, we set up a perspective transform (which
    // transforms geometry from 3D view space to 2D viewport space, with
    // a perspective divide making objects smaller in the distance). To build
    // a perpsective transform, we need the field of view (1/4 pi is common),
    // the aspect ratio, and the near and far clipping planes (which define at
    // what distances geometry should be no longer be rendered).
    D3DXMatrixPerspectiveFovLH( &m_MatProj, D3DX_PI/4, 1.33f, m_NearClip, m_FarClip );

	// Normally no need to send matrices to DirectX (with SetTransform) when using a vertex shader
	// for matrix work. However, when using DirectX fog, we must send the projection matrix anyway
    g_pd3dDevice->SetTransform( D3DTS_PROJECTION, &m_MatProj );

	// Combine the view and projection matrix into a single matrix - this will
	// be passed to vertex shaders as a single matrix (more efficient this way)
	m_MatViewProj = m_MatView * m_MatProj;
}


// Control the camera using keys
void CCamera::Control( EKeyCode turnUp, EKeyCode turnDown,
                       EKeyCode turnLeft, EKeyCode turnRight,  
                       EKeyCode moveForward, EKeyCode moveBackward,
                       EKeyCode moveLeft, EKeyCode moveRight)
{
	if (KeyHeld( turnDown ))
	{
		m_Rotation.x += RotSpeed;
	}
	if (KeyHeld( turnUp ))
	{
		m_Rotation.x -= RotSpeed;
	}
	if (KeyHeld( turnRight ))
	{
		m_Rotation.y += RotSpeed;
	}
	if (KeyHeld( turnLeft ))
	{
		m_Rotation.y -= RotSpeed;
	}

	// Local X movement - move in the direction of the X axis, get axis from view matrix
	if (KeyHeld( moveRight ))
	{
		m_Position.x += m_MatView._11 * MoveSpeed;
		m_Position.y += m_MatView._21 * MoveSpeed;
		m_Position.z += m_MatView._31 * MoveSpeed;
	}
	if (KeyHeld( moveLeft ))
	{
		m_Position.x -= m_MatView._11 * MoveSpeed;
		m_Position.y -= m_MatView._21 * MoveSpeed;
		m_Position.z -= m_MatView._31 * MoveSpeed;
	}

	// Local Z movement - move in the direction of the Z axis, get axis from view matrix
	if (KeyHeld( moveForward ))
	{
		m_Position.x += m_MatView._13 * MoveSpeed;
		m_Position.y += m_MatView._23 * MoveSpeed;
		m_Position.z += m_MatView._33 * MoveSpeed;
	}
	if (KeyHeld( moveBackward ))
	{
		m_Position.x -= m_MatView._13 * MoveSpeed;
		m_Position.y -= m_MatView._23 * MoveSpeed;
		m_Position.z -= m_MatView._33 * MoveSpeed;
	}
}
