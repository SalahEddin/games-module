/**********************************************
	Model.cpp

	Implementation of model class for DirectX
***********************************************/

#include "Defines.h"
#include "Model.h"

#include "CImportXFile.h"    // Class to load meshes (taken from a full graphics engine)

///////////////////////////////
// Constructors / Destructors

// Model constructor
CModel::CModel()
{
	// Initialise member variables
	m_VertexBuffer = NULL;
	m_NumVertices = 0;
	m_VertexSize = 0;

	m_IndexBuffer = NULL;
	m_NumIndices = 0;

	m_HasGeometry = false;

	m_Position = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
	m_Rotation = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
	m_Scale = 1.0f;
}

// Model destructor
CModel::~CModel()
{
	ReleaseResources();
}

// Release resources used by model
void CModel::ReleaseResources()
{
	// Release resources (where necessary)
	if (m_IndexBuffer != NULL)
	{
		m_IndexBuffer->Release();
	}
	if (m_VertexBuffer != NULL)
	{
		m_VertexBuffer->Release();
	}
	m_HasGeometry = false;
}


/////////////////////////////
// Model Loading / Creation

// Create the model geometry from arrays of vertices and indices
bool CModel::CreateGeometry
(
	void*        vertices,   // Pointer to vertex array (void* because we allow custom types)
	unsigned int numVertices,// Number of vertices in the model mesh
	DWORD        vertexFVF,  // DirectX FVF code describing the vertex format (look up D3DFVF)
	WORD*        indices,    // Pointer to index array (assuming 2-byte values, WORD in DirectX)
	unsigned int numIndices  // Number of indices in the model mesh
)
{
	// Release any existing geometry
	ReleaseResources();

	// Store FVF (vertex format descriptor) and use it to get size of a single vertex
	m_VertexFVF = vertexFVF;
	m_VertexSize = D3DXGetFVFVertexSize( m_VertexFVF );

	// Create the vertex buffer
	unsigned int bufferSize = numVertices * m_VertexSize;
    if (FAILED(g_pd3dDevice->CreateVertexBuffer( bufferSize, D3DUSAGE_WRITEONLY, 0,
                                                 D3DPOOL_DEFAULT, &m_VertexBuffer, NULL )))
    {
        return false;
    }
	m_NumVertices = numVertices;

    // "Lock" the vertex buffer so we can write to it
    void* bufferData;
    if (FAILED(m_VertexBuffer->Lock( 0, bufferSize, (void**)&bufferData, 0 )))
	{
        return false;
	}

	// Copy the vertex data
    memcpy( bufferData, vertices, bufferSize );

	// Unlock the vertex buffer again so it can be used for rendering
    m_VertexBuffer->Unlock();


    // Create the index buffer - assuming 2-byte (WORD) index data
	bufferSize = numIndices * sizeof(WORD);
    if (FAILED(g_pd3dDevice->CreateIndexBuffer( bufferSize, D3DUSAGE_WRITEONLY, D3DFMT_INDEX16,
                                                D3DPOOL_DEFAULT, &m_IndexBuffer, NULL )))
    {
        return false;
    }
	m_NumIndices = numIndices;

    // "Lock" the index buffer so we can write to it
    if (FAILED(m_IndexBuffer->Lock( 0, bufferSize, (void**)&bufferData, 0 )))
	{
        return false;
	}

	// Copy the index data
    memcpy( bufferData, indices, bufferSize );

	// Unlock the index buffer again so it can be used for rendering
    m_IndexBuffer->Unlock();

	m_HasGeometry = true;
	return true;
}


// Load the model geometry from a file. This model class only supports a single material 
// per model. Real world models often use several materials for different parts of the
// geometry. This function only reads the geometry using the first material in the file,
// so multi-material models will load but will have parts missing
bool CModel::Load( const string& fileName )
{
	// Release any existing geometry
	ReleaseResources();

	// Use CImportXFile class (from another application) to load the given file
	// The import code is wrapped in the namespace 'gen'
	gen::CImportXFile mesh;
	if (mesh.ImportFile( fileName.c_str() ) != gen::kSuccess)
	{
		return false;
	}

	// Just use first sub-mesh from loaded file
	gen::SSubMesh subMesh;
	if (mesh.GetSubMesh( 0, &subMesh ) != gen::kSuccess)
	{
		return false;
	}

	// Calculate FVF (vertex format descriptor) and use it to get size of a single vertex
	m_VertexFVF = D3DFVF_XYZ + (subMesh.hasNormals ? D3DFVF_NORMAL : 0) + 
	                           (subMesh.hasTextureCoords ? D3DFVF_TEX1 : 0) + 
	                           (subMesh.hasVertexColours ? D3DFVF_DIFFUSE : 0);
	m_VertexSize = subMesh.vertexSize;

	// Create the vertex buffer
	m_NumVertices = subMesh.numVertices;
	unsigned int bufferSize = m_NumVertices * m_VertexSize;
    if (FAILED(g_pd3dDevice->CreateVertexBuffer( bufferSize, D3DUSAGE_WRITEONLY, 0,
                                                 D3DPOOL_DEFAULT, &m_VertexBuffer, NULL )))
    {
        return false;
    }

    // "Lock" the vertex buffer so we can write to it
    void* bufferData;
    if (FAILED(m_VertexBuffer->Lock( 0, bufferSize, (void**)&bufferData, 0 )))
	{
        return false;
	}

	// Copy the vertex data from the sub-mesh
	memcpy( bufferData, subMesh.vertices, bufferSize );

	// Unlock the vertex buffer again so it can be used for rendering
    m_VertexBuffer->Unlock();


    // Create the index buffer - assuming 2-byte (WORD) index data
	m_NumIndices = static_cast<unsigned int>(subMesh.numFaces) * 3;
	bufferSize = m_NumIndices * sizeof(WORD);
    if (FAILED(g_pd3dDevice->CreateIndexBuffer( bufferSize, D3DUSAGE_WRITEONLY, D3DFMT_INDEX16,
                                                D3DPOOL_DEFAULT, &m_IndexBuffer, NULL )))
    {
        return false;
    }

    // "Lock" the index buffer so we can write to it
    if (FAILED(m_IndexBuffer->Lock( 0, bufferSize, (void**)&bufferData, 0 )))
	{
        return false;
	}

	// Copy the vertex data from the sub-mesh
	memcpy( bufferData, subMesh.faces, bufferSize );

	// Unlock the index buffer again so it can be used for rendering
    m_IndexBuffer->Unlock();

	m_HasGeometry = true;
	return true;
}


/////////////////////////////
// Model Usage

// Calculate the model's world matrix from its current position, orientation and scale
// Don't send matrix to DirectX (with SetTransform) as using a vertex shader for matrix work
void CModel::CalculateMatrix()
{
	// Build the matrix for the model from its position, rotation and scaling
	D3DXMATRIXA16 MatScale, MatX, MatY, MatZ, MatTrans;
	D3DXMatrixScaling( &MatScale, m_Scale, m_Scale, m_Scale );
	D3DXMatrixRotationX( &MatX, m_Rotation.x );
	D3DXMatrixRotationY( &MatY, m_Rotation.y );
	D3DXMatrixRotationZ( &MatZ, m_Rotation.z );
	D3DXMatrixTranslation( &MatTrans, m_Position.x, m_Position.y, m_Position.z );
	m_Matrix = MatScale * MatZ * MatX * MatY * MatTrans;
}

// Render the model (using current material)
void CModel::Render()
{
	// Don't render if no geometry
	if (!m_HasGeometry)
	{
		return;
	}

	// Tell DirectX the vertex buffer to use and indicate its type (using the FVF code)
	g_pd3dDevice->SetStreamSource( 0, m_VertexBuffer, 0, m_VertexSize );
	g_pd3dDevice->SetFVF( m_VertexFVF );

	// Now tell DirectX the index buffer to use
	g_pd3dDevice->SetIndices( m_IndexBuffer );


	// Draw the primitives from the vertex buffer - a triangle list
	g_pd3dDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST,  // Primitive type - usually tri-list or strip
										0,                   // Offset to add to all indices (0 in simple cases)
										0,                   // Minimum index used (allows for optimisation) 
										m_NumVertices,       // Range of vertices refered to, effectively =
															 //     maximum index - minimum index + 1
										0,                   // Position to start at in index buffer
										m_NumIndices / 3 );  // Number of primitives to render (triangles)
}


// Control the model using keys
void CModel::Control( EKeyCode turnUp, EKeyCode turnDown,
					  EKeyCode turnLeft, EKeyCode turnRight,  
					  EKeyCode turnCW, EKeyCode turnCCW,
					  EKeyCode moveForward, EKeyCode moveBackward )
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
	if (KeyHeld( turnCW ))
	{
		m_Rotation.z += RotSpeed;
	}
	if (KeyHeld( turnCCW ))
	{
		m_Rotation.z -= RotSpeed;
	}

	// Local Z movement - move in the direction of the Z axis, get axis from world matrix
	if (KeyHeld( moveForward ))
	{
		m_Position.x += m_Matrix._31 * MoveSpeed;
		m_Position.y += m_Matrix._32 * MoveSpeed;
		m_Position.z += m_Matrix._33 * MoveSpeed;
	}
	if (KeyHeld( moveBackward ))
	{
		m_Position.x -= m_Matrix._31 * MoveSpeed;
		m_Position.y -= m_Matrix._32 * MoveSpeed;
		m_Position.z -= m_Matrix._33 * MoveSpeed;
	}
}


