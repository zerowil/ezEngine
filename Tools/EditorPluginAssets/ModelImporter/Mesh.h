﻿#pragma once

#include <EditorPluginAssets/ModelImporter/Node.h>
#include <EditorPluginAssets/ModelImporter/VertexIndex.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <Foundation/Containers/HashTable.h>

namespace ezModelImporter
{
  class VertexDataStream;

  /// Range in a mesh using a specific material.
  struct SubMesh
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt32 m_uiFirstTriangle;
    ezUInt32 m_uiTriangleCount;

    MaterialHandle m_Material;
  };

  /// Triangle mesh imported from a raw asset file.
  ///
  /// Note that as of now we do not support other mesh representations, which means that any other topology needs to be triangulated on import.
  class Mesh : public HierarchyObject
  {
    EZ_DISALLOW_COPY_AND_ASSIGN(Mesh);

  public:

    Mesh();
    Mesh(Mesh&& mesh);

    ~Mesh();

    // Triangles
  public:

    struct Triangle
    {
      EZ_DECLARE_POD_TYPE();

      void operator = (const Triangle& rhs);
      VertexIndex m_Vertices[3];
    };

    /// Adds a given number of triangles to the mesh.
    /// All vertex data points to zero elements by default.
    void AddTriangles(ezUInt32 num);
    ezUInt32 GetNumTriangles() const { return m_Triangles.GetCount(); }
    ezArrayPtr<const Triangle> GetTriangles() const { return ezMakeArrayPtr(m_Triangles); }
    ezArrayPtr<Triangle> GetTriangles()             { return ezMakeArrayPtr(m_Triangles); }

    // Data Streams
  public:

    /// Adds a new data stream with the given semantic.
    ///
    /// If a data stream already exists it will be returned, unless the existing data stream has a different element type or element count per vertex attribute.
    /// In this case nullptr will be returned.
    VertexDataStream* AddDataStream(ezGALVertexAttributeSemantic::Enum semantic, ezUInt32 uiNumElementsPerVertex, VertexElementType elementType = VertexElementType::FLOAT);

    /// Removes a data stream from the mesh.
    void RemoveDataStream(ezGALVertexAttributeSemantic::Enum semantic);

    /// Retrieves a data stream for a given semantic. Null if there is none.
    VertexDataStream* GetDataStream(ezGALVertexAttributeSemantic::Enum semantic);
    const VertexDataStream* GetDataStream(ezGALVertexAttributeSemantic::Enum semantic) const;

    // Submeshes
  public:

    ezUInt32 GetNumSubMeshes() const;
    const SubMesh& GetSubMesh(ezUInt32 idx) const;
    SubMesh& GetSubMesh(ezUInt32 idx);
    void AddSubMesh(SubMesh& mesh);

    // Processing
  public:

    /// Applies a transform to all directional and positional vertex data.
    void ApplyTransform(const ezTransform& transform);

    /// Adds all triangles, vertices and submeshes from an existing mesh.
    void AddData(const Mesh& mesh, const ezTransform& transform = ezTransform::Identity());

    /// Merges all sub-meshes that use the same material.
    /// Especially useful after merging meshes.
    void MergeSubMeshesWithSameMaterials();

    /// Computes vertex normals from position data.
    /// If the mesh already has a vertex stream for normals, they will be recomputed.
    /// Fails if there is no position stream.
    ezResult ComputeNormals();

    /// Computes vertex tangents and bitangent signs.
    ///
    /// Bitangent vertex stream will only have a single element which determines the sign of the bitangent. BiTangent = (normal x tangent) * sign
    /// If the mesh already has a vertex stream for (bi)tangent, they will be recomputed.
    /// Fails if there is no normal stream or there is no Texcoord0 stream with 2 components.
    ezResult ComputeTangents();

  private:

    ezDynamicArray<Triangle> m_Triangles;
    ezUInt32 m_uiNextUnusedVertexIndex;

    ezHashTable<ezInt32, VertexDataStream*> m_VertexDataStreams;

    /// List of submeshes. There should be always at least one submesh.
    ezDynamicArray<SubMesh> m_SubMeshes;
  };
}

#include <EditorPluginAssets/ModelImporter/Mesh.inl>
