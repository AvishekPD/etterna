#ifndef NOTE_DATA_WITH_SCORING_H
#define NOTE_DATA_WITH_SCORING_H

#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Models/Misc/PlayerNumber.h"

struct RadarValues;
class NoteData;
class PlayerStageStats;
struct TapNote;

/** @brief NoteData with scores for each TapNote and HoldNote. */
namespace NoteDataWithScoring {
/**
 * @brief Has the current row of NoteData been judged completely?
 * @param in the entire Notedata.
 * @param iRow the row to check.
 * @plnum If valid, only consider notes for that PlayerNumber
 * @return true if it has been completley judged, or false otherwise. */
bool
IsRowCompletelyJudged(const NoteData& in, const unsigned& row);

TapNoteScore
MinTapNoteScore(const NoteData& in, const unsigned& row);

const TapNote&
LastTapNoteWithResult(const NoteData& in, const unsigned& row);

void
GetActualRadarValues(const NoteData& in,
					 const PlayerStageStats& pss,
					 const float& song_seconds,
					 RadarValues& out);
};

#endif
