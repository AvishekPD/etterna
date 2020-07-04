#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/AdjustSync.h"
#include "Etterna/Singletons/GameManager.h"
#include "Etterna/Models/Misc/GamePreferences.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Models/Misc/InputEventPlus.h"
#include "Etterna/Models/NoteLoaders/NotesLoaderSM.h"
#include "Etterna/Models/NoteLoaders/NotesLoaderSSC.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "Etterna/Models/Misc/ScreenDimensions.h"
#include "ScreenGameplaySyncMachine.h"
#include "Etterna/Models/Songs/SongUtil.h"
#include "Etterna/Singletons/NetworkSyncManager.h"
#include "Etterna/Singletons/ProfileManager.h"

REGISTER_SCREEN_CLASS(ScreenGameplaySyncMachine);

void
ScreenGameplaySyncMachine::Init()
{
	GAMESTATE->m_bPlayingMulti = false;
	// The server crashes if syncing is attempted while connected to SMO. -Kyz
	NSMAN->ReportSongOver();

	GAMESTATE->m_PlayMode.Set(PLAY_MODE_REGULAR);
	GAMESTATE->SetCurrentStyle(
	  GAMEMAN->GetHowToPlayStyleForGame(GAMESTATE->m_pCurGame), PLAYER_INVALID);
	AdjustSync::ResetOriginalSyncData();

	std::string sFile = THEME->GetPathO("ScreenGameplaySyncMachine", "music");
	// Allow themers to use either a .ssc or .sm file for this. -aj
	SSCLoader loaderSSC;
	SMLoader loaderSM;
	if (sFile.Right(4) == ".ssc")
		loaderSSC.LoadFromSimfile(sFile, m_Song);
	else
		loaderSM.LoadFromSimfile(sFile, m_Song);

	m_Song.SetSongDir(Dirname(sFile));
	m_Song.TidyUpData();

	GAMESTATE->m_pCurSong.Set(&m_Song);
	// Needs proper StepsType -freem
	vector<Steps*> vpSteps;
	SongUtil::GetPlayableSteps(&m_Song, vpSteps);
	ASSERT_M(vpSteps.size() > 0,
			 "No playable steps for ScreenGameplaySyncMachine");
	Steps* pSteps = vpSteps[0];
	GAMESTATE->m_pCurSteps.Set(pSteps);

	GamePreferences::m_AutoPlay.Set(PC_HUMAN);

	PROFILEMAN->LoadLocalProfileFromMachine(PLAYER_1);
	GAMESTATE->LoadCurrentSettingsFromProfile(PLAYER_1);

	ScreenGameplayNormal::Init();

	SO_GROUP_ASSIGN(GAMESTATE->m_SongOptions,
					ModsLevel_Stage,
					m_AutosyncType,
					AutosyncType_Machine);

	ClearMessageQueue(); // remove all of the messages set in ScreenGameplay
						 // that animate "ready", "here we go", etc.

	GAMESTATE->m_bGameplayLeadIn.Set(false);

	m_DancingState = STATE_DANCING;

	m_textSyncInfo.SetName("SyncInfo");
	m_textSyncInfo.LoadFromFont(THEME->GetPathF(m_sName, "SyncInfo"));
	ActorUtil::LoadAllCommands(m_textSyncInfo, m_sName);
	this->AddChild(&m_textSyncInfo);

	this->SubscribeToMessage(Message_AutosyncChanged);

	RefreshText();
}

void
ScreenGameplaySyncMachine::Update(float fDelta)
{
	ScreenGameplayNormal::Update(fDelta);
	RefreshText();
}

bool
ScreenGameplaySyncMachine::Input(const InputEventPlus& input)
{
	// Hack to make this work from Player2's controls
	InputEventPlus _input = input;

	if (_input.GameI.controller != GameController_Invalid)
		_input.GameI.controller = GameController_1;
	if (_input.pn != PLAYER_INVALID)
		_input.pn = PLAYER_1;

	// Hacky way to never allow skipping to "eval" from this screen
	AbortGiveUp(false);

	return ScreenGameplay::Input(_input);
}

void
ScreenGameplaySyncMachine::HandleScreenMessage(const ScreenMessage SM)
{
	if (SM == SM_NotesEnded) {
		ResetAndRestartCurrentSong();
		return; // handled
	}

	ScreenGameplayNormal::HandleScreenMessage(SM);

	if (SM == SM_GoToPrevScreen || SM == SM_GoToNextScreen) {
		GAMESTATE->m_pCurSteps.Set(nullptr);
		GAMESTATE->SetCurrentStyle(nullptr, PLAYER_INVALID);
		GAMESTATE->m_pCurSong.Set(nullptr);
	}
}

void
ScreenGameplaySyncMachine::ResetAndRestartCurrentSong()
{
	m_pSoundMusic->Stop();
	ReloadCurrentSong();
	StartPlayingSong(4, 0);

	// reset autosync
	AdjustSync::ResetAutosync();
}

static LocalizedString OLD_OFFSET("ScreenGameplaySyncMachine", "Old offset");
static LocalizedString NEW_OFFSET("ScreenGameplaySyncMachine", "New offset");
static LocalizedString COLLECTING_SAMPLE("ScreenGameplaySyncMachine",
										 "Collecting sample");
static LocalizedString STANDARD_DEVIATION("ScreenGameplaySyncMachine",
										  "Standard deviation");
void
ScreenGameplaySyncMachine::RefreshText()
{
	float fNew = PREFSMAN->m_fGlobalOffsetSeconds;
	float fOld = AdjustSync::s_fGlobalOffsetSecondsOriginal;
	float fStdDev = AdjustSync::s_fStandardDeviation;
	std::string s;
	s += OLD_OFFSET.GetValue() + ssprintf(": %0.3f\n", fOld);
	s += NEW_OFFSET.GetValue() + ssprintf(": %0.3f\n", fNew);
	s += STANDARD_DEVIATION.GetValue() + ssprintf(": %0.3f\n", fStdDev);
	s += COLLECTING_SAMPLE.GetValue() +
		 ssprintf(": %d / %d",
				  AdjustSync::s_iAutosyncOffsetSample + 1,
				  AdjustSync::OFFSET_SAMPLE_COUNT);

	m_textSyncInfo.SetText(s);
}
