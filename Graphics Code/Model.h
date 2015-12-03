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
	D3DXMATRIX GetWorldMatrix()
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
	// Model Loading

	// Load the model geometry from a file. This function only reads the geometry using the first  
	// material in the file,so multi-material models will load but will have parts missing
	// May optionally request for tangents to be created for the model (for normal or parallax mapping)
	// Under DX10, we need to pass the vertex shader code that the model will use so it can
	// precalculate the mapping from vertex data to shader input
	// Can optionally add adjacency data to the index buffer (DirectX 10) and/or tangents to the vertex data
	bool Load( const string& fileName, ID3D10Blob* shaderCode, bool adjacency = false, bool tangents = false );


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
	ID3D10Buffer*            m_VertexBuffer;
	unsigned int             m_NumVertices;

	// Description of the elements in a single vertex (position, normal etc.)
	static const int         MAX_VERTEX_ELTS = 64;
	D3D10_INPUT_ELEMENT_DESC m_VertexElts[MAX_VERTEX_ELTS];
	ID3D10InputLayout*       m_VertexLayout; // Layout of a vertex (derived from above)
	unsigned int             m_VertexSize;   // Size of vertex calculated from contained elements

	// Index data for the model stored in a index buffer and the number of
	// indices in the buffer
	ID3D10Buffer*            m_IndexBuffer;
	unsigned int             m_NumIndices;
	bool                     m_bHasAdjacency; // Whether index data has DirectX 10 adjacency data

	// Does this model have any geometry to render
	bool          m_HasGeometry;

	// Positions, rotations and scaling for the model
	D3DXVECTOR3   m_Position;
	D3DXVECTOR3   m_Rotation;
	float         m_Scale;

	// World matrix for the model - built from the above
	D3DXMATRIX m_Matrix;
};
