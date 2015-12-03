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
	m_VertexLayout = NULL;

	m_IndexBuffer = NULL;
	m_NumIndices = 0;
	m_bHasAdjacency = false;

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
	// Release resources
	// Using a DirectX helper macro to simplify code here - look it up in Defines.h
	SAFE_RELEASE( m_IndexBuffer );
	SAFE_RELEASE( m_VertexBuffer );
	SAFE_RELEASE( m_VertexLayout );
	m_bHasAdjacency = false;
	m_HasGeometry = false;
}


/////////////////////////////
// Model Loading

// Load the model geometry from a file. This function only reads the geometry using the first  
// material in the file,so multi-material models will load but will have parts missing
// May optionally request for tangents to be created for the model (for normal or parallax mapping)
// Under DX10, we need to pass the vertex shader code that the model will use so it can
// precalculate the mapping from vertex data to shader input
// Can optionally add adjacency data to the index buffer (DirectX 10) and/or tangents to the vertex data
bool CModel::Load( const string& fileName, ID3D10Blob* shaderCode, bool adjacency /*= false*/, bool tangents /*= false*/ )
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

	// Get first sub-mesh from loaded file
	gen::SSubMesh subMesh;
	if (mesh.GetSubMesh( 0, &subMesh, adjacency, tangents ) != gen::kSuccess)
	{
		return false;
	}
	m_bHasAdjacency = adjacency;

	// Create vertex element list & layout
	unsigned int numElts = 0;
	unsigned int offset = 0;
	m_VertexElts[numElts].SemanticName = "POSITION";   // Semantic in HLSL (equivalent of Usage in DX9 struct)
	m_VertexElts[numElts].SemanticIndex = 0;           // Index to add to semantic (equivalent of UsageIndex)
	m_VertexElts[numElts].Format = DXGI_FORMAT_R32G32B32_FLOAT; // Type of data - this one is float3 (equivalent of Type)
	m_VertexElts[numElts].AlignedByteOffset = offset;  // Offset of element from start of vertex data
	m_VertexElts[numElts].InputSlot = 0;               // For when using multiple vertex buffers (e.g. instancing) (equivalent of Stream)
	m_VertexElts[numElts].InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA; // Use this value if not using instancing
	m_VertexElts[numElts].InstanceDataStepRate = 0;                     // --"--
	offset += 12;
	++numElts;
	if (subMesh.hasNormals)
	{
		m_VertexElts[numElts].SemanticName = "NORMAL";
		m_VertexElts[numElts].SemanticIndex = 0;
		m_VertexElts[numElts].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		m_VertexElts[numElts].AlignedByteOffset = offset;
		m_VertexElts[numElts].InputSlot = 0;
		m_VertexElts[numElts].InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA;
		m_VertexElts[numElts].InstanceDataStepRate = 0;
		offset += 12;
		++numElts;
	}
	if (subMesh.hasTangents)
	{
		m_VertexElts[numElts].SemanticName = "TANGENT";
		m_VertexElts[numElts].SemanticIndex = 0;
		m_VertexElts[numElts].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		m_VertexElts[numElts].AlignedByteOffset = offset;
		m_VertexElts[numElts].InputSlot = 0;
		m_VertexElts[numElts].InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA;
		m_VertexElts[numElts].InstanceDataStepRate = 0;
		offset += 12;
		++numElts;
	}
	if (subMesh.hasTextureCoords)
	{
		m_VertexElts[numElts].SemanticName = "TEXCOORD";
		m_VertexElts[numElts].SemanticIndex = 0;
		m_VertexElts[numElts].Format = DXGI_FORMAT_R32G32_FLOAT;
		m_VertexElts[numElts].AlignedByteOffset = offset;
		m_VertexElts[numElts].InputSlot = 0;
		m_VertexElts[numElts].InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA;
		m_VertexElts[numElts].InstanceDataStepRate = 0;
		offset += 8;
		++numElts;
	}
	if (subMesh.hasVertexColours)
	{
		m_VertexElts[numElts].SemanticName = "COLOR";
		m_VertexElts[numElts].SemanticIndex = 0;
		m_VertexElts[numElts].Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		m_VertexElts[numElts].AlignedByteOffset = offset;
		m_VertexElts[numElts].InputSlot = 0;
		m_VertexElts[numElts].InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA;
		m_VertexElts[numElts].InstanceDataStepRate = 0;
		offset += 4;
		++numElts;
	}
	m_VertexSize = offset;

	g_pd3dDevice->CreateInputLayout( m_VertexElts, numElts, shaderCode->GetBufferPointer(),
	                                 shaderCode->GetBufferSize(), &m_VertexLayout );

	// Create the vertex buffer
	// Can also fill the buffer with the vertex data at the same time as creating it under DX10
	m_NumVertices = subMesh.numVertices;
	D3D10_BUFFER_DESC bufferDesc;
	bufferDesc.BindFlags = D3D10_BIND_VERTEX_BUFFER;
	bufferDesc.Usage = D3D10_USAGE_DEFAULT; // Not a dynamic buffer
	bufferDesc.ByteWidth = m_NumVertices * m_VertexSize; // Buffer size
	bufferDesc.CPUAccessFlags = 0;   // Indicates that CPU won't access this buffer at all after creation
	bufferDesc.MiscFlags = 0;
	D3D10_SUBRESOURCE_DATA initData; // Initial data
	initData.pSysMem = subMesh.vertices;   
	if (FAILED( g_pd3dDevice->CreateBuffer( &bufferDesc, &initData, &m_VertexBuffer )))
	{
		return false;
	}

	// Index data depends on use of adjacency data. DirectX 10 interleaves triangle indexes and the indexes of adjacent vertices.
	// At this point the adjacency data is in a seperate data block, so the interleaving is done now to a temporary data area
	gen::TUInt16* indexData;
	if (!m_bHasAdjacency)
	{
		// No adjacency data needed, face list is already in correct form (list of triplets of indices)
		m_NumIndices = static_cast<unsigned int>(subMesh.numFaces) * 3;
		indexData = reinterpret_cast<gen::TUInt16*>(subMesh.faces);
	}
	else
	{
		m_NumIndices = static_cast<unsigned int>(subMesh.numFaces) * 6;
		indexData = new gen::TUInt16[m_NumIndices];
		gen::TUInt16* data = indexData;
		for (unsigned int face = 0; face < subMesh.numFaces; ++face)
		{
			*data++ = subMesh.faces[face].aiVertex[0];
			*data++ = subMesh.faceAdjacency[face].aiVertex[0];
			*data++ = subMesh.faces[face].aiVertex[1];
			*data++ = subMesh.faceAdjacency[face].aiVertex[1];
			*data++ = subMesh.faces[face].aiVertex[2];
			*data++ = subMesh.faceAdjacency[face].aiVertex[2];
		}
	}

	// Create the index buffer - assuming 2-byte (WORD) index data
	bufferDesc.BindFlags = D3D10_BIND_INDEX_BUFFER;
	bufferDesc.Usage = D3D10_USAGE_DEFAULT;
	bufferDesc.ByteWidth = m_NumIndices * sizeof(WORD);
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;
	initData.pSysMem = indexData;   
	if (FAILED( g_pd3dDevice->CreateBuffer( &bufferDesc, &initData, &m_IndexBuffer )))
	{
		if (m_bHasAdjacency) delete[] indexData; // Clean up temporary adjacency data
		return false;
	}
	if (m_bHasAdjacency) delete[] indexData;

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
	D3DXMATRIX MatScale, MatRotX, MatRotY, MatRotZ, MatTrans;
	D3DXMatrixScaling( &MatScale, m_Scale, m_Scale, m_Scale );
	D3DXMatrixRotationX( &MatRotX, m_Rotation.x );
	D3DXMatrixRotationY( &MatRotY, m_Rotation.y );
	D3DXMatrixRotationZ( &MatRotZ, m_Rotation.z );
	D3DXMatrixTranslation( &MatTrans, m_Position.x, m_Position.y, m_Position.z );
	m_Matrix = MatScale * MatRotZ * MatRotX * MatRotY * MatTrans;
}

// Render the model (using current material)
void CModel::Render()
{
	// Don't render if no geometry
	if (!m_HasGeometry)
	{
		return;
	}

	ID3D10Buffer* buffers[1] = { m_VertexBuffer };
	UINT strides[1] = { m_VertexSize };
	UINT offsets[1] = {0};
	g_pd3dDevice->IASetVertexBuffers( 0, 1, buffers, strides, offsets );
	
	g_pd3dDevice->IASetInputLayout( m_VertexLayout );
	g_pd3dDevice->IASetIndexBuffer( m_IndexBuffer, DXGI_FORMAT_R16_UINT, 0 );

	// Specify that model geometry is a triangle list - and whether primitives have adjacency data
	g_pd3dDevice->IASetPrimitiveTopology( m_bHasAdjacency ? D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ :
		                                                    D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	g_pd3dDevice->DrawIndexed( m_NumIndices, 0, 0 );
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
