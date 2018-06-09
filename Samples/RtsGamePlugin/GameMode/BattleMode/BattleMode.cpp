#include <PCH.h>
#include <RtsGamePlugin/GameMode/BattleMode/BattleMode.h>
#include <RtsGamePlugin/GameState/RtsGameState.h>
#include <RtsGamePlugin/Components/ComponentMessages.h>

RtsBattleMode::RtsBattleMode() = default;
RtsBattleMode::~RtsBattleMode() = default;

void RtsBattleMode::OnActivateMode()
{
}

void RtsBattleMode::OnDeactivateMode()
{
}

void RtsBattleMode::OnBeforeWorldUpdate()
{
  m_pGameState->RenderUnitSelection();
}

void RtsBattleMode::RegisterInputActions()
{
}

void RtsBattleMode::OnProcessInput(const RtsMouseInputState& MouseInput)
{
  DoDefaultCameraInput(MouseInput);

  ezVec3 vPickedGroundPlanePos;
  if (m_pGameState->PickGroundPlanePosition(vPickedGroundPlanePos).Failed())
    return;

  const auto& unitSelection = m_pGameState->m_SelectedUnits;

  if (MouseInput.m_LeftClickState == ezKeyState::Released)
  {
    m_pGameState->SelectUnits();
  }

  if (MouseInput.m_RightClickState == ezKeyState::Released && !MouseInput.m_bRightMouseMoved)
  {
    RtsMsgNavigateTo msg;
    msg.m_vTargetPosition = vPickedGroundPlanePos.GetAsVec2();

    for (ezUInt32 i = 0; i < unitSelection.GetCount(); ++i)
    {
      ezGameObjectHandle hObject = unitSelection.GetObject(i);
      m_pMainWorld->SendMessage(hObject, msg);
    }
  }
}
