#include "NELogger.h"
#include "SpawnDataHelper.h"

#include "GlobalNamespace/BeatmapObjectSpawnMovementData_ObstacleSpawnData.hpp"
#include "GlobalNamespace/BeatmapObjectData.hpp"
#include "GlobalNamespace/NoteData.hpp"
#include "GlobalNamespace/BeatmapObjectSpawnController_InitData.hpp"

#include "custom-json-data/shared/CustomBeatmapData.h"

#include "tracks/shared/Vector.h"
#include "NECaches.h"

using namespace GlobalNamespace;



//float SpawnDataHelperF::GetSpawnAheadTime(BeatmapObjectSpawnMovementData *spawnMovementData, std::optional<float> inputNjs, std::optional<float> inputOffset) {
//    return spawnMovementData->moveDuration + (GetJumpDuration(inputNjs, inputOffset) * 0.5f);
//}



void SpawnDataHelper::GetNoteJumpValues(BeatmapObjectSpawnController::InitData *initData,
                                        BeatmapObjectSpawnMovementData *spawnMovementData,
                                        std::optional<float> const inputNoteJumpMovementSpeed,
                                        std::optional<float> const inputNoteJumpStartBeatOffset,
                                        float &jumpDuration, float &jumpDistance,
                                        NEVector::Vector3 &localMoveStartPos, NEVector::Vector3 &localMoveEndPos,
                                        NEVector::Vector3 &localJumpEndPos) {
    jumpDuration = GetJumpDuration(initData, spawnMovementData, inputNoteJumpMovementSpeed, inputNoteJumpStartBeatOffset);

    NEVector::Vector3 const forwardVec(spawnMovementData->forwardVec);
    NEVector::Vector3 const centerPos(spawnMovementData->centerPos);

    jumpDistance = (inputNoteJumpMovementSpeed.value_or(spawnMovementData->noteJumpMovementSpeed)) * jumpDuration;
    localMoveEndPos = centerPos + (forwardVec * (jumpDistance * 0.5f));
    localJumpEndPos = centerPos - (forwardVec * (jumpDistance * 0.5f));
    localMoveStartPos = centerPos + (forwardVec * (spawnMovementData->moveDistance + (jumpDistance * 0.5f)));
}

constexpr float Orig_LineYPosForLineLayer(GlobalNamespace::NoteLineLayer lineLayer)
{
    if (lineLayer == GlobalNamespace::NoteLineLayer::Base)
    {
        return 0.25f;
    }
    if (lineLayer == GlobalNamespace::NoteLineLayer::Upper)
    {
        return 0.85f;
    }
    return 1.45f;
}

float
SpawnDataHelper::LineYPosForLineLayer(BeatmapObjectSpawnMovementData *spawnMovementData, std::optional<float> height,
                                      NoteLineLayer noteLineLayer) {
    if (height) {
        return  0.25f
                + (height.value() * NECaches::get_noteLinesDistanceFast()); // offset by 0.25
    }

    return Orig_LineYPosForLineLayer(noteLineLayer);
}
