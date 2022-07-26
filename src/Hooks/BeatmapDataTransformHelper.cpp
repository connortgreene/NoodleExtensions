#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

#include "GlobalNamespace/IJumpOffsetYProvider.hpp"
#include "GlobalNamespace/PlayerHeightDetector.hpp"
#include "GlobalNamespace/PlayerHeightToJumpOffsetYProvider.hpp"
#include "GlobalNamespace/BeatmapData.hpp"
#include "GlobalNamespace/BeatmapDataObstaclesMergingTransform.hpp"
#include "GlobalNamespace/BeatmapDataTransformHelper.hpp"
#include "GlobalNamespace/BeatmapLineData.hpp"
#include "GlobalNamespace/EnvironmentEffectsFilterPreset.hpp"
#include "GlobalNamespace/EnvironmentIntensityReductionOptions.hpp"
#include "GlobalNamespace/GameplayModifiers.hpp"
#include "GlobalNamespace/IReadonlyBeatmapData.hpp"
#include "GlobalNamespace/PracticeSettings.hpp"
#include "System/Collections/Generic/IEnumerable_1.hpp"
#include "System/Func_2.hpp"
#include "System/Linq/Enumerable.hpp"
#include "System/Linq/IOrderedEnumerable_1.hpp"
#include "UnityEngine/Resources.hpp"

#include "AssociatedData.h"
#include "NEHooks.h"
#include "NELogger.h"
#include "NECaches.h"
#include "custom-json-data/shared/CustomBeatmapData.h"
#include "GlobalNamespace/SortedList_1.hpp"
#include "GlobalNamespace/BeatmapObjectSpawnMovementData.hpp"
#include "SpawnDataHelper.h"
#include "tracks/shared/Json.h"

#include <optional>

using namespace GlobalNamespace;
using namespace System::Collections::Generic;

//
//IReadonlyBeatmapData *ReorderLineData(IReadonlyBeatmapData *beatmapData) {
//    auto *customBeatmapData = static_cast<CustomJSONData::CustomBeatmapData *>(beatmapData->GetCopy());
//    if (!customObstacleDataClass) {
//        customObstacleDataClass = classof(CustomJSONData::CustomObstacleData *);
//        customNoteDataClass = classof(CustomJSONData::CustomNoteData *);
//    }
//
//
//
//    auto notes = customBeatmapData->GetBeatmapItemsCpp<NoteData*>();
//    auto obstacles = customBeatmapData->GetBeatmapItemsCpp<ObstacleData*>();
//
//    std::vector<BeatmapObjectData*> objects;
//    objects.reserve(notes.size() + obstacles.size());
//
//    std::copy(notes.begin(), notes.end(), std::back_inserter(objects));
//    std::copy(obstacles.begin(), obstacles.end(), std::back_inserter(objects));
//
//    // loop through all objects in all lines of the beatmapData
//    for (BeatmapObjectData *beatmapObjectData : objects) {
//
//    }
//
//    return reinterpret_cast<IReadonlyBeatmapData *>(customBeatmapData);
//}

extern System::Collections::Generic::LinkedList_1<BeatmapDataItem*>* SortAndOrderList(CustomJSONData::CustomBeatmapData* beatmapData);


void LoadNoodleObjects(CustomJSONData::CustomBeatmapData *beatmap) {
    NELogger::GetLogger().info("BeatmapData klass name is %s",
                               beatmap->klass->name);

    static auto *customObstacleDataClass = classof(CustomJSONData::CustomObstacleData *);
    static auto *customNoteDataClass = classof(CustomJSONData::CustomNoteData *);

    TracksAD::BeatmapAssociatedData &beatmapAD = TracksAD::getBeatmapAD(beatmap->customData);

    if (!beatmapAD.valid) {
        TracksAD::readBeatmapDataAD(beatmap);
    }

    auto v2 = beatmap->v2orEarlier;

    bool mirror = true;

    if (beatmap->customData->value) {
        rapidjson::Value const& data = *beatmap->customData->value;
        mirror = NEJSON::ReadOptionalBool(data, v2 ? "_questNoteMirror" : "questNoteMirror").value_or(true);
    }

    auto notes = beatmap->GetBeatmapItemsCpp<NoteData*>();
    auto obstacles = beatmap->GetBeatmapItemsCpp<ObstacleData*>();


    auto doForObjects = [&](auto&& objects) constexpr {
        for (BeatmapObjectData *beatmapObjectData: objects) {
            CustomJSONData::CustomNoteData *noteData = nullptr;
            CustomJSONData::CustomObstacleData *obstacleData = nullptr;
            CustomJSONData::JSONWrapper *customDataWrapper;
            if (beatmapObjectData->klass == customObstacleDataClass) {
                obstacleData = (CustomJSONData::CustomObstacleData *) beatmapObjectData;
                customDataWrapper = obstacleData->customData;
            } else if (beatmapObjectData->klass == customNoteDataClass) {
                noteData = (CustomJSONData::CustomNoteData *) beatmapObjectData;
                customDataWrapper = noteData->customData;
            } else {
                continue;
            }


            BeatmapObjectAssociatedData &ad = getAD(customDataWrapper);

            ad.mirror = mirror;

            if (customDataWrapper->value) {
                rapidjson::Value const &customData = *customDataWrapper->value;

                if (ad.parsed)
                    continue;

                ad.objectData = ObjectCustomData(customData, noteData, obstacleData, v2);

                if (!ad.flip) {
                    ad.flip = NEJSON::ReadOptionalVector2_emptyY(customData, v2 ? NoodleExtensions::Constants::V2_FLIP : NoodleExtensions::Constants::FLIP);
                }

                auto animationKey = v2 ? NoodleExtensions::Constants::V2_ANIMATION
                                       : NoodleExtensions::Constants::ANIMATION;
                if (customData.HasMember(animationKey.data())) {
                    rapidjson::Value const &animation = customData[animationKey.data()];
                    ad.animationData = {beatmapAD, animation, v2};
                } else {
                    ad.animationData = AnimationObjectData();
                }
                ad.parsed = true;
            }
        }
    };

    CJDLogger::Logger.fmtLog<Paper::LogLevel::INF>("Reading Noodle objects");
    doForObjects(obstacles);
    doForObjects(notes);
}

void LoadNoodleEvent(TracksAD::BeatmapAssociatedData &beatmapAD, CustomJSONData::CustomEventData const *customEventData,
                     bool v2) {
    bool isType = false;

    auto typeHash = customEventData->typeHash;

#define TYPE_GET(jsonName, varName)                                \
    static auto jsonNameHash_##varName = std::hash<std::string_view>()(jsonName); \
    if (!isType && typeHash == (jsonNameHash_##varName))                      \
        isType = true;

    TYPE_GET(NoodleExtensions::Constants::ASSIGN_TRACK_PARENT, AssignTrackParent)
    TYPE_GET(NoodleExtensions::Constants::ASSIGN_PLAYER_TO_TRACK, AssignPlayerToTrack)

    if (!isType) {
        return;
    }
    CRASH_UNLESS(customEventData->data);
    rapidjson::Value const& eventData = *customEventData->data;
    auto& eventAD = getEventAD(customEventData);

    if (eventAD.parsed)
        return;

    if (typeHash == jsonNameHash_AssignTrackParent) {
        eventAD.parentTrackEventData.emplace(eventData, beatmapAD, v2);
    } else if (typeHash == jsonNameHash_AssignPlayerToTrack) {
        std::string_view trackName(eventData[v2 ? NoodleExtensions::Constants::V2_TRACK.data() : NoodleExtensions::Constants::TRACK.data()].GetString());
        Track *track = &beatmapAD.tracks.try_emplace(std::string(trackName), v2).first->second;
        NELogger::GetLogger().debug("Assigning player to track %s at %p",
                                    trackName.data(), track);
        eventAD.playerTrackEventData.emplace(track);
    }

    eventAD.parsed = true;
}

void LoadNoodleEvents(CustomJSONData::CustomBeatmapData* beatmap) {
    auto &beatmapAD = TracksAD::getBeatmapAD(beatmap->customData);

    auto v2 = beatmap->v2orEarlier;

    if (!beatmapAD.valid) {
        TracksAD::readBeatmapDataAD(beatmap);
    }

    // Parse events
    for (auto const& customEventData : beatmap->GetBeatmapItemsCpp<CustomJSONData::CustomEventData*>()) {
        LoadNoodleEvent(beatmapAD, customEventData, v2);
    }
}

MAKE_HOOK_MATCH(BeatmapDataTransformHelper_CreateTransformedBeatmapData,
                &BeatmapDataTransformHelper::CreateTransformedBeatmapData, IReadonlyBeatmapData *,
                GlobalNamespace::IReadonlyBeatmapData* beatmapData, ::GlobalNamespace::IPreviewBeatmapLevel* beatmapLevel,
                ::GlobalNamespace::GameplayModifiers* gameplayModifiers, bool leftHanded,
                ::GlobalNamespace::EnvironmentEffectsFilterPreset environmentEffectsFilterPreset,
                ::GlobalNamespace::EnvironmentIntensityReductionOptions* environmentIntensityReductionOptions,
                ::GlobalNamespace::MainSettingsModelSO* mainSettingsModel) {
    auto result = BeatmapDataTransformHelper_CreateTransformedBeatmapData(
        beatmapData, beatmapLevel, gameplayModifiers, leftHanded,
        environmentEffectsFilterPreset, environmentIntensityReductionOptions,
        mainSettingsModel);

    if (!Hooks::isNoodleHookEnabled())
        return result;

    auto customBeatmap = reinterpret_cast<CustomJSONData::CustomBeatmapData *>(result);


    LoadNoodleObjects(reinterpret_cast<CustomJSONData::CustomBeatmapData *>(result));
    //    auto linkedList = il2cpp_utils::cast<GlobalNamespace::SortedList_1<BeatmapDataItem*>>(customBeatmap->allBeatmapData);
//    linkedList->items = SortAndOrderList(customBeatmap);
//    linkedList->lastUsedNode = linkedList->items->get_Last();

    auto *transformedBeatmapData = result; // ReorderLineData(result);

    LoadNoodleEvents(reinterpret_cast<CustomJSONData::CustomBeatmapData *>(transformedBeatmapData));

    return transformedBeatmapData;
}

void InstallBeatmapDataTransformHelperHooks(Logger &logger) {
    INSTALL_HOOK_ORIG(logger, BeatmapDataTransformHelper_CreateTransformedBeatmapData);
}

NEInstallHooks(InstallBeatmapDataTransformHelperHooks);