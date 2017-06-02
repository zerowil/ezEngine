﻿#include <PCH.h>
#include <RecastPlugin/NavMeshBuilder/NavMeshBuilder.h>
#include <ThirdParty/Recast/Recast.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Time/Stopwatch.h>
#include <Core/World/World.h>
#include <GameEngine/Messages/BuildNavMeshMessage.h>
#include <ThirdParty/Recast/DetourNavMeshBuilder.h>
#include <ThirdParty/Recast/DetourNavMesh.h>

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezRecastConfig, ezNoBase, 1, ezRTTIDefaultAllocator<ezRecastConfig>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("AgentHeight", m_fAgentHeight)->AddAttributes(new ezDefaultValueAttribute(1.5f)),
    EZ_MEMBER_PROPERTY("AgentRadius", m_fAgentRadius)->AddAttributes(new ezDefaultValueAttribute(0.3f)),
    EZ_MEMBER_PROPERTY("AgentClimbHeight", m_fAgentClimbHeight)->AddAttributes(new ezDefaultValueAttribute(0.4f)),
    EZ_MEMBER_PROPERTY("WalkableSlope", m_WalkableSlope)->AddAttributes(new ezDefaultValueAttribute(ezAngle::Degree(45))),
    EZ_MEMBER_PROPERTY("CellSize", m_fCellSize)->AddAttributes(new ezDefaultValueAttribute(0.2f)),
    EZ_MEMBER_PROPERTY("CellHeight", m_fCellHeight)->AddAttributes(new ezDefaultValueAttribute(0.2f)),
    EZ_MEMBER_PROPERTY("MinRegionSize", m_fMinRegionSize)->AddAttributes(new ezDefaultValueAttribute(3.0f)),
    EZ_MEMBER_PROPERTY("RegionMergeSize", m_fRegionMergeSize)->AddAttributes(new ezDefaultValueAttribute(20.0f)),
    EZ_MEMBER_PROPERTY("SampleDistanceFactor", m_fDetailMeshSampleDistanceFactor)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("SampleErrorFactor", m_fDetailMeshSampleErrorFactor)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("MaxSimplification", m_fMaxSimplificationError)->AddAttributes(new ezDefaultValueAttribute(1.3f)),
    EZ_MEMBER_PROPERTY("MaxEdgeLength", m_fMaxEdgeLength)->AddAttributes(new ezDefaultValueAttribute(4.0f)),
  }
  EZ_END_PROPERTIES
}
EZ_END_STATIC_REFLECTED_TYPE

class ezRcBuildContext : public rcContext
{
public:
  ezRcBuildContext() {}

protected:
  virtual void doLog(const rcLogCategory category, const char* msg, const int len)
  {
    switch (category)
    {
    case RC_LOG_ERROR:
      ezLog::Error("Recast: {0}", msg);
      return;
    case RC_LOG_WARNING:
      ezLog::Warning("Recast: {0}", msg);
      return;
    case RC_LOG_PROGRESS:
      ezLog::Debug("Recast: {0}", msg);
      return;

    default:
      ezLog::Error("Unknwon recast log: {0}", msg);
      return;
    }
  }
};

ezRecastNavMeshBuilder::ezRecastNavMeshBuilder() { }

ezRecastNavMeshBuilder::~ezRecastNavMeshBuilder()
{
  rcFreePolyMesh(m_polyMesh);
  rcFreePolyMeshDetail(m_detailMesh);
}

ezResult ezRecastNavMeshBuilder::Build(const ezRecastConfig& config, const ezWorld& world)
{
  EZ_LOG_BLOCK("ezRecastNavMeshBuilder::Build (world)");

  ezStopwatch sw;

  ezNavMeshDescription desc;

  ezBuildNavMeshMessage msg;
  msg.m_pNavMeshDescription = &desc;

  // gather all nav mesh related information from all objects in the world
  for (auto it = world.GetObjects(); it.IsValid(); ++it)
  {
    it->SendMessage(msg);
  }

  ezLog::Debug("Gathering NavMesh description: {0}ms", ezArgF(sw.Checkpoint().GetMilliseconds(), 2));
  ezLog::Debug("NavMesh Box Obstacles: {0}", desc.m_BoxObstacles.GetCount());

  return Build(config, desc);
}

ezResult ezRecastNavMeshBuilder::Build(const ezRecastConfig& config, const ezNavMeshDescription& desc)
{
  EZ_LOG_BLOCK("ezRecastNavMeshBuilder::Build (desc)");

  ezStopwatch watch;

  ezUniquePtr<ezRcBuildContext> recastContext = EZ_DEFAULT_NEW(ezRcBuildContext);
  m_pRecastContext = recastContext.Borrow();

  GenerateTriangleMeshFromDescription(desc);

  if (m_Vertices.IsEmpty())
  {
    ezLog::Debug("Navmesh is empty");
    return EZ_SUCCESS;
  }

  ComputeBoundingBox();

  ezLog::Debug("Generate Triangle Mesh: {0}ms", ezArgF(watch.Checkpoint().GetMilliseconds()));

  if (BuildRecastNavMesh(config).Failed())
    return EZ_FAILURE;

  ezLog::Debug("Build Recast Navmesh: {0}ms", ezArgF(watch.Checkpoint().GetMilliseconds()));

  return EZ_SUCCESS;
}

void ezRecastNavMeshBuilder::ReserveMemory(const ezNavMeshDescription& desc)
{
  const ezUInt32 uiBoxes = desc.m_BoxObstacles.GetCount();
  const ezUInt32 uiBoxTriangles = uiBoxes * 12;
  const ezUInt32 uiBoxVertices = uiBoxes * 8;

  const ezUInt32 uiTriangles = uiBoxTriangles + desc.m_Triangles.GetCount();
  const ezUInt32 uiVertices = uiBoxVertices + desc.m_Vertices.GetCount();

  m_Triangles.Reserve(uiTriangles);
  m_TriangleAreaIDs.Reserve(uiTriangles);
  m_Vertices.Reserve(uiVertices);
}

void ezRecastNavMeshBuilder::GenerateTriangleMeshFromDescription(const ezNavMeshDescription& desc)
{
  EZ_LOG_BLOCK("ezRecastNavMeshBuilder::GenerateTriangleMesh");

  m_Triangles.Clear();
  m_TriangleAreaIDs.Clear();
  m_Vertices.Clear();

  ReserveMemory(desc);

  {
    for (const auto& v : desc.m_Vertices)
    {
      m_Vertices.PushBack(v);
    }

    for (const auto& tri : desc.m_Triangles)
    {
      auto& nt = m_Triangles.ExpandAndGetRef();
      nt.m_VertexIdx[0] = tri.m_uiVertexIndices[0];
      nt.m_VertexIdx[1] = tri.m_uiVertexIndices[1];
      nt.m_VertexIdx[2] = tri.m_uiVertexIndices[2];
    }
  }

  for (const auto& box : desc.m_BoxObstacles)
  {
    const ezUInt32 uiFirstVtx = m_Vertices.GetCount();

    // add the 8 box vertices
    {
      ezVec3 ext = box.m_vHalfExtents;

      ezVec3 exts[8];
      exts[0] = ezVec3(ext.x, ext.y, ext.z);
      exts[1] = ezVec3(ext.x, ext.y, -ext.z);
      exts[2] = ezVec3(ext.x, -ext.y, ext.z);
      exts[3] = ezVec3(ext.x, -ext.y, -ext.z);
      exts[4] = ezVec3(-ext.x, ext.y, ext.z);
      exts[5] = ezVec3(-ext.x, ext.y, -ext.z);
      exts[6] = ezVec3(-ext.x, -ext.y, ext.z);
      exts[7] = ezVec3(-ext.x, -ext.y, -ext.z);

      for (ezUInt32 i = 0; i < 8; ++i)
      {
        ezVec3 pos = box.m_vPosition + box.m_qRotation * exts[i];
        ezMath::Swap(pos.y, pos.z);

        m_Vertices.ExpandAndGetRef() = pos;
      }
    }

    // Add all triangles
    {
      m_Triangles.ExpandAndGetRef() = Triangle(uiFirstVtx + 0, uiFirstVtx + 5, uiFirstVtx + 1);
      m_Triangles.ExpandAndGetRef() = Triangle(uiFirstVtx + 0, uiFirstVtx + 4, uiFirstVtx + 5);
      m_Triangles.ExpandAndGetRef() = Triangle(uiFirstVtx + 2, uiFirstVtx + 1, uiFirstVtx + 3);
      m_Triangles.ExpandAndGetRef() = Triangle(uiFirstVtx + 2, uiFirstVtx + 0, uiFirstVtx + 1);
      m_Triangles.ExpandAndGetRef() = Triangle(uiFirstVtx + 6, uiFirstVtx + 3, uiFirstVtx + 7);
      m_Triangles.ExpandAndGetRef() = Triangle(uiFirstVtx + 6, uiFirstVtx + 2, uiFirstVtx + 3);
      m_Triangles.ExpandAndGetRef() = Triangle(uiFirstVtx + 4, uiFirstVtx + 7, uiFirstVtx + 5);
      m_Triangles.ExpandAndGetRef() = Triangle(uiFirstVtx + 4, uiFirstVtx + 6, uiFirstVtx + 7);
      m_Triangles.ExpandAndGetRef() = Triangle(uiFirstVtx + 4, uiFirstVtx + 2, uiFirstVtx + 6);
      m_Triangles.ExpandAndGetRef() = Triangle(uiFirstVtx + 4, uiFirstVtx + 0, uiFirstVtx + 2);
      m_Triangles.ExpandAndGetRef() = Triangle(uiFirstVtx + 7, uiFirstVtx + 1, uiFirstVtx + 5);
      m_Triangles.ExpandAndGetRef() = Triangle(uiFirstVtx + 7, uiFirstVtx + 3, uiFirstVtx + 1);
    }
  }

  // initialize the IDs to zero
  m_TriangleAreaIDs.SetCount(m_Triangles.GetCount());

  ezLog::Debug("Vertices: {0}, Triangles: {1}", m_Vertices.GetCount(), m_Triangles.GetCount());
}


void ezRecastNavMeshBuilder::ComputeBoundingBox()
{
  if (!m_Vertices.IsEmpty())
  {
    m_BoundingBox.SetFromPoints(m_Vertices.GetData(), m_Vertices.GetCount());
  }
}

void ezRecastNavMeshBuilder::FillOutConfig(rcConfig& cfg, const ezRecastConfig& config)
{
  ezMemoryUtils::ZeroFill(&cfg);
  cfg.bmin[0] = m_BoundingBox.m_vMin.x;
  cfg.bmin[1] = m_BoundingBox.m_vMin.y;
  cfg.bmin[2] = m_BoundingBox.m_vMin.z;
  cfg.bmax[0] = m_BoundingBox.m_vMax.x;
  cfg.bmax[1] = m_BoundingBox.m_vMax.y;
  cfg.bmax[2] = m_BoundingBox.m_vMax.z;
  cfg.ch = config.m_fCellHeight;
  cfg.cs = config.m_fCellSize;
  cfg.walkableSlopeAngle = config.m_WalkableSlope.GetDegree();
  cfg.walkableHeight = (int)ceilf(config.m_fAgentHeight / cfg.ch);
  cfg.walkableClimb = (int)floorf(config.m_fAgentClimbHeight / cfg.ch);
  cfg.walkableRadius = (int)ceilf(config.m_fAgentRadius / cfg.cs);
  cfg.maxEdgeLen = (int)(config.m_fMaxEdgeLength / cfg.cs);
  cfg.maxSimplificationError = config.m_fMaxSimplificationError;
  cfg.minRegionArea = (int)ezMath::Square(config.m_fMinRegionSize / cfg.cs);
  cfg.mergeRegionArea = (int)ezMath::Square(config.m_fRegionMergeSize / cfg.cs);
  cfg.maxVertsPerPoly = 6;
  cfg.detailSampleDist = config.m_fDetailMeshSampleDistanceFactor < 0.9f ? 0 : cfg.cs * config.m_fDetailMeshSampleDistanceFactor;
  cfg.detailSampleMaxError = cfg.ch * config.m_fDetailMeshSampleDistanceFactor;

  rcCalcGridSize(cfg.bmin, cfg.bmax, cfg.cs, &cfg.width, &cfg.height);
}

ezResult ezRecastNavMeshBuilder::BuildRecastNavMesh(const ezRecastConfig& config)
{
  rcConfig cfg;
  FillOutConfig(cfg, config);

  ezRcBuildContext* pContext = m_pRecastContext;
  const float* pVertices = &m_Vertices[0].x;
  const ezInt32* pTriangles = &m_Triangles[0].m_VertexIdx[0];

  rcHeightfield* heightfield = rcAllocHeightfield();
  EZ_SCOPE_EXIT(rcFreeHeightField(heightfield));

  if (!rcCreateHeightfield(pContext, *heightfield, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs, cfg.ch))
  {
    pContext->log(RC_LOG_ERROR, "Could not create solid heightfield");
    return EZ_FAILURE;
  }

  // TODO Instead of this, it should use area IDs and then clear the non-walkable triangles
  rcMarkWalkableTriangles(pContext, cfg.walkableSlopeAngle, pVertices, m_Vertices.GetCount(), pTriangles, m_Triangles.GetCount(), m_TriangleAreaIDs.GetData());

  if (!rcRasterizeTriangles(pContext, pVertices, m_Vertices.GetCount(), pTriangles, m_TriangleAreaIDs.GetData(), m_Triangles.GetCount(), *heightfield, cfg.walkableClimb))
  {
    pContext->log(RC_LOG_ERROR, "Could not rasterize triangles");
    return EZ_FAILURE;
  }

  // Optional stuff
  {
    //if (m_filterLowHangingObstacles)
    rcFilterLowHangingWalkableObstacles(pContext, cfg.walkableClimb, *heightfield);

    //if (m_filterLedgeSpans)
    rcFilterLedgeSpans(pContext, cfg.walkableHeight, cfg.walkableClimb, *heightfield);

    //if (m_filterWalkableLowHeightSpans)
    rcFilterWalkableLowHeightSpans(pContext, cfg.walkableHeight, *heightfield);
  }

  rcCompactHeightfield* compactHeightfield = rcAllocCompactHeightfield();
  EZ_SCOPE_EXIT(rcFreeCompactHeightfield(compactHeightfield));

  if (!rcBuildCompactHeightfield(pContext, cfg.walkableHeight, cfg.walkableClimb, *heightfield, *compactHeightfield))
  {
    pContext->log(RC_LOG_ERROR, "Could not build compact data");
    return EZ_FAILURE;
  }

  if (!rcErodeWalkableArea(pContext, cfg.walkableRadius, *compactHeightfield))
  {
    pContext->log(RC_LOG_ERROR, "Could not erode with character radius");
    return EZ_FAILURE;
  }

  // (Optional) Mark areas.
  //{
  //  const ConvexVolume* vols = m_geom->getConvexVolumes();
  //  for (int i = 0; i < m_geom->getConvexVolumeCount(); ++i)
  //    rcMarkConvexPolyArea(pContext, vols[i].verts, vols[i].nverts, vols[i].hmin, vols[i].hmax, (unsigned char)vols[i].area, *compactHeightfield);
  //}


  // Partition the heightfield so that we can use simple algorithm later to triangulate the walkable areas.
  // Default algorithm is 'Watershed'
  {
    // PARTITION_WATERSHED
    {
      // Prepare for region partitioning, by calculating distance field along the walkable surface.
      if (!rcBuildDistanceField(pContext, *compactHeightfield))
      {
        pContext->log(RC_LOG_ERROR, "Could not build distance field.");
        return EZ_FAILURE;
      }

      // Partition the walkable surface into simple regions without holes.
      if (!rcBuildRegions(pContext, *compactHeightfield, 0, cfg.minRegionArea, cfg.mergeRegionArea))
      {
        pContext->log(RC_LOG_ERROR, "Could not build watershed regions.");
        return EZ_FAILURE;
      }
    }

    //// PARTITION_MONOTONE
    //{
    //  // Partition the walkable surface into simple regions without holes.
    //  // Monotone partitioning does not need distance field.
    //  if (!rcBuildRegionsMonotone(pContext, *compactHeightfield, 0, cfg.minRegionArea, cfg.mergeRegionArea))
    //  {
    //    pContext->log(RC_LOG_ERROR, "Could not build monotone regions.");
    //    return EZ_FAILURE;
    //  }
    //}

    //// PARTITION_LAYERS
    //{
    //  // Partition the walkable surface into simple regions without holes.
    //  if (!rcBuildLayerRegions(pContext, *compactHeightfield, 0, cfg.minRegionArea))
    //  {
    //    pContext->log(RC_LOG_ERROR, "Could not build layer regions.");
    //    return EZ_FAILURE;
    //  }
    //}
  }

  rcContourSet* contourSet = rcAllocContourSet();
  EZ_SCOPE_EXIT(rcFreeContourSet(contourSet));

  if (!rcBuildContours(pContext, *compactHeightfield, cfg.maxSimplificationError, cfg.maxEdgeLen, *contourSet))
  {
    pContext->log(RC_LOG_ERROR, "Could not create contours");
    return EZ_FAILURE;
  }

  m_polyMesh = rcAllocPolyMesh();
  // this is not deallocated, it is the final result !

  if (!rcBuildPolyMesh(pContext, *contourSet, cfg.maxVertsPerPoly, *m_polyMesh))
  {
    pContext->log(RC_LOG_ERROR, "Could not triangulate contours");
    return EZ_FAILURE;
  }

  m_detailMesh = rcAllocPolyMeshDetail();
  // this is not deallocated, it is the final result !

  if (!rcBuildPolyMeshDetail(pContext, *m_polyMesh, *compactHeightfield, cfg.detailSampleDist, cfg.detailSampleMaxError, *m_detailMesh))
  {
    pContext->log(RC_LOG_ERROR, "buildNavigation: Could not build detail mesh.");
    return EZ_FAILURE;
  }

  //////////////////////////////////////////////////////////////////////////
  // Detour Navmesh

  // TODO modify area IDs and flags 

  for (int i = 0; i < m_polyMesh->npolys; ++i)
  {
    if (m_polyMesh->areas[i] == RC_WALKABLE_AREA)
    {
      m_polyMesh->flags[i] = 0xFFFF;
    }
  }

  return CreateDetourNavMesh(config);
}


ezResult ezRecastNavMeshBuilder::CreateDetourNavMesh(const ezRecastConfig& config)
{
  dtNavMeshCreateParams params;
  ezMemoryUtils::ZeroFill(&params);

  params.verts = m_polyMesh->verts;
  params.vertCount = m_polyMesh->nverts;
  params.polys = m_polyMesh->polys;
  params.polyAreas = m_polyMesh->areas;
  params.polyFlags = m_polyMesh->flags;
  params.polyCount = m_polyMesh->npolys;
  params.nvp = m_polyMesh->nvp;
  params.detailMeshes = m_detailMesh->meshes;
  params.detailVerts = m_detailMesh->verts;
  params.detailVertsCount = m_detailMesh->nverts;
  params.detailTris = m_detailMesh->tris;
  params.detailTriCount = m_detailMesh->ntris;
  params.walkableHeight = config.m_fAgentHeight;
  params.walkableRadius = config.m_fAgentRadius;
  params.walkableClimb = config.m_fAgentClimbHeight;
  rcVcopy(params.bmin, m_polyMesh->bmin);
  rcVcopy(params.bmax, m_polyMesh->bmax);
  params.cs = config.m_fCellSize;
  params.ch = config.m_fCellHeight;
  params.buildBvTree = true;

  ezUInt8* navData = nullptr;
  ezInt32 navDataSize = 0;

  if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
  {
    ezLog::Error("Could not build Detour navmesh.");
    return EZ_FAILURE;
  }

  m_pNavMesh = dtAllocNavMesh();

  if (dtStatusFailed(m_pNavMesh->init(navData, navDataSize, DT_TILE_FREE_DATA)))
  {
    dtFree(navData);
    ezLog::Error("Could not init Detour navmesh");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}


