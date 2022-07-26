#include "FakeNoteHelper.h"

#include "GlobalNamespace/NoteController.hpp"
#include "GlobalNamespace/ObstacleController.hpp"

#include "custom-json-data/shared/CustomBeatmapData.h"

#include "NELogger.h"
#include "AssociatedData.h"

#include "System/Collections/Generic/List_1.hpp"
#include "System/Collections/Generic/IList_1.hpp"
#include "custom-json-data/shared/VList.h"

using namespace GlobalNamespace;

bool FakeNoteHelper::GetFakeNote(NoteData *noteData) {
    auto *customNoteData = reinterpret_cast<CustomJSONData::CustomNoteData *>(noteData);
    if (!customNoteData->customData->value) {
        return false;
    }
    BeatmapObjectAssociatedData &ad = getAD(customNoteData->customData);
    return ad.objectData.fake && *ad.objectData.fake;
}

bool FakeNoteHelper::GetCuttable(NoteData *noteData) {
    auto *customNoteData = reinterpret_cast<CustomJSONData::CustomNoteData *>(noteData);
    if (!customNoteData->customData->value) {
        return true;
    }
    BeatmapObjectAssociatedData &ad = getAD(customNoteData->customData);
    return !ad.objectData.uninteractable || !*ad.objectData.uninteractable;
}

List<ObstacleController *>*
FakeNoteHelper::ObstacleFakeCheck(VList<GlobalNamespace::ObstacleController*> intersectingObstacles) {
//    auto filteredInner = List<GlobalNamespace::ObstacleController*>::New_ctor();

    for (auto const& obstacle : intersectingObstacles) {
        if (!obstacle || !obstacle->obstacleData)
            continue;

        auto customData = reinterpret_cast<CustomJSONData::CustomObstacleData*>(obstacle->obstacleData)->customData;

        bool add = !customData || !customData->value;

        if (!add) {
            auto const &ad = getAD(customData);
            add = !ad.objectData.fake.value_or(false);
        }

        if (!add) {
            (*intersectingObstacles)->Remove(obstacle);
        }
    }

    return intersectingObstacles;
}
