/**********************************************
	Model.h

	Declaration of model class for DirectX
***********************************************/

#pragma once // Prevent file being included more than once (would cause errors)

#include <string>
using namespace std;

#include <d3d9.h>
#include <d3dx9.h>
#include "Input.h"

//-----------------------------------------------------------------------------
// DirectX Model Class
//-----------------------------------------------------------------------------

class CModel
{
/////////////////////////////
// Public member functions
public:

	///////////////////////////////
	// Constructors / Destructors

	// Constructor
	CModel();

	// Destructor
	~CModel();

	// Release resources used by model
	void ReleaseResources();


	/////////////////////////////
	// Data access

	// Getters
	D3DXMATRIXA16 GetWorldMatrix()
	{
		return m_Matrix;
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

    void SetScale( float scale )
	{
		m_Scale = scale;
	}


	/////////////////////////////
	// Model Loading / Creation

	// Load the model geometry from a file
	bool Load( const string& fileName );

	// Create the model geometry from arrays of vertices and indices
	bool CreateGeometry
	(
		void*        vertices,   // Pointer to vertex array (void* because we allow custom types)
		unsigned int numVertices,// Number of vertices in the model mesh
		DWORD        vertexFVF,  // DirectX FVF code describing the vertex format (look up D3DFVF)
		WORD*        indices,    // Pointer to index array (assuming 2-byte values, WORD in DirectX)
		unsigned int numIndices  // Number of indices in the model mesh
	);


	/////////////////////////////
	// Model Usage

	// Calculate the model's world matrix from its current position, orientation and scale
	void CalculateMatrix();
	
	// Render the model
	void Render();
	
	// Control the model using keys
	void Control( EKeyCode turnUp, EKeyCode turnDown,
				  EKeyCode turnLeft, EKeyCode turnRight,  
				  EKeyCode turnCW, EKeyCode turnCCW,
				  EKeyCode moveForward, EKeyCode moveBackward );


/////////////////////////////
// Private member variables
private:

	// Vertex data for the model stored in a vertex buffer and the number / size of
	// the vertices in the buffer
	LPDIRECT3DVERTEXBUFFER9 m_VertexBuffer;
	unsigned int            m_NumVertices;
	DWORD                   m_VertexFVF;  // DirectX FVF code for vertex format (look up D3DFVF)
	unsigned int            m_VertexSize;

	// Index data for the model stored in a index buffer and the number of
	// indices in the buffer
	LPDIRECT3DINDEXBUFFER9  m_IndexBuffer;
	unsigned int            m_NumIndices;

	// Does this model have any geometry to render
	bool          m_HasGeometry;

	// Positions, rotations and scaling for the model
	D3DXVECTOR3   m_Position;
	D3DXVECTOR3   m_Rotation;
	float         m_Scale;

	// World matrix for the model - built from the above
	D3DXMATRIXA16 m_Matrix;
};
