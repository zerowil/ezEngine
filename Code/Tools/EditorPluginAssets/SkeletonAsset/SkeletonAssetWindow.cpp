#include <PCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <EditorFramework/InputContexts/OrbitCameraContext.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAssetWindow.moc.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonPanel.moc.h>
#include <Texture/Image/ImageConversion.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/Widgets/ImageWidget.moc.h>
#include <QLabel>
#include <QLayout>

ezQtSkeletonAssetDocumentWindow::ezQtSkeletonAssetDocumentWindow(ezSkeletonAssetDocument* pDocument)
    : ezQtEngineDocumentWindow(pDocument)
{
  // Menu Bar
  {
    ezQtMenuBarActionMapView* pMenuBar = static_cast<ezQtMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "SkeletonAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezQtToolBarActionMapView* pToolBar = new ezQtToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "SkeletonAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("SkeletonAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // 3D View
  ezQtViewWidgetContainer* pContainer = nullptr;
  {
    SetTargetFramerate(25);

    m_ViewConfig.m_Camera.LookAt(ezVec3(-1.6, 0, 0), ezVec3(0, 0, 0), ezVec3(0, 0, 1));
    m_ViewConfig.ApplyPerspectiveSetting(90);

    m_pViewWidget = new ezQtOrbitCamViewWidget(this, &m_ViewConfig);
    m_pViewWidget->ConfigureOrbitCameraVolume(ezVec3(0, 0, 1), ezVec3(10.0f), ezVec3(-5, 1, 2));
    AddViewWidget(m_pViewWidget);
    pContainer = new ezQtViewWidgetContainer(this, m_pViewWidget, "SkeletonAssetViewToolBar");
    setCentralWidget(pContainer);
  }

  // Property Grid
  {
    ezQtDocumentPanel* pPropertyPanel = new ezQtDocumentPanel(this);
    pPropertyPanel->setObjectName("SkeletonAssetDockWidget");
    pPropertyPanel->setWindowTitle("Skeleton Properties");
    pPropertyPanel->show();

    ezQtPropertyGridWidget* pPropertyGrid = new ezQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  // Tree View
  {
    ezQtDocumentPanel* pPanelTree = new ezQtSkeletonPanel(this, static_cast<ezSkeletonAssetDocument*>(pDocument));
    pPanelTree->show();

    addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, pPanelTree);
  }

  FinishWindowCreation();

  QueryObjectBBox(0);
}

ezSkeletonAssetDocument* ezQtSkeletonAssetDocumentWindow::GetSkeletonDocument()
{
  return static_cast<ezSkeletonAssetDocument*>(GetDocument());
}

void ezQtSkeletonAssetDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (ezEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  for (auto pView : m_ViewWidgets)
  {
    pView->SetEnablePicking(false);
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }

  QueryObjectBBox(1);
}

void ezQtSkeletonAssetDocumentWindow::QueryObjectBBox(ezInt32 iPurpose)
{
  ezQuerySelectionBBoxMsgToEngine msg;
  msg.m_uiViewID = 0xFFFFFFFF;
  msg.m_iPurpose = iPurpose;
  GetDocument()->SendMessageToEngine(&msg);
}

void ezQtSkeletonAssetDocumentWindow::InternalRedraw()
{
  ezEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  ezQtEngineDocumentWindow::InternalRedraw();
}

void ezQtSkeletonAssetDocumentWindow::ProcessMessageEventHandler(const ezEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezQuerySelectionBBoxResultMsgToEditor>())
  {
    const ezQuerySelectionBBoxResultMsgToEditor* pMessage = static_cast<const ezQuerySelectionBBoxResultMsgToEditor*>(pMsg);

    if (pMessage->m_vCenter.IsValid() && pMessage->m_vHalfExtents.IsValid() && pMessage->m_vHalfExtents.x >= 0 &&
        pMessage->m_vHalfExtents.y >= 0 && pMessage->m_vHalfExtents.z >= 0)
    {
      m_pViewWidget->GetOrbitCamera()->SetOrbitVolume(pMessage->m_vCenter, pMessage->m_vHalfExtents * 2.0f,
                                                      pMessage->m_vCenter + ezVec3(5, -2, 3) * pMessage->m_vHalfExtents.GetLength() * 0.3f,
                                                      pMessage->m_iPurpose == 0);
    }
    else if (pMessage->m_iPurpose == 0)
    {
      // try again
      QueryObjectBBox(pMessage->m_iPurpose);
    }

    return;
  }

  ezQtEngineDocumentWindow::ProcessMessageEventHandler(pMsg);
}
