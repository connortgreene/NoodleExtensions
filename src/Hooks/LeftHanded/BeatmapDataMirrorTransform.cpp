#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"

#include "GlobalNamespace/BeatmapDataMirrorTransform.hpp"
#include "GlobalNamespace/BeatmapEventTypeExtensions.hpp"

#include "AssociatedData.h"

#include "NEHooks.h"
#include "custom-json-data/shared/CustomBeatmapData.h"
#include "custom-json-data/shared/VList.h"

using namespace GlobalNamespace;

MAKE_HOOK_MATCH(BeatmapDataMirrorTransform_CreateTransformedData, &BeatmapDataMirrorTransform::CreateTransformedData,
                GlobalNamespace::IReadonlyBeatmapData*, GlobalNamespace::IReadonlyBeatmapData* beatmapData) {
//    auto newBeatmap = BeatmapDataMirrorTransform_CreateTransformedData(beatmapData);
//
//    for (auto beatmapEventData : VList(reinterpret_cast<System::Collections::Generic::List_1<CustomJSONData::CustomBeatmapEventData*>*>(newBeatmap->get_beatmapEventsData()))) {
//        static auto CustomBeatmapEventDataKlass = classof(CustomJSONData::CustomBeatmapEventData*);
//
//        if (!BeatmapEventTypeExtensions::Rot(beatmapEventData->type) || beatmapEventData->klass != CustomBeatmapEventDataKlass)
//        {
//            continue;
//        }
//
//        auto& ad = getAD(beatmapEventData->customData);
//
//        // TODO: If this breaks, I blame Aero for having single float as rotation values
//        if (ad.objectData.rotation) {
//            auto vec = ad.objectData.cutDirection->get_eulerAngles();
//
//            if (vec.x == 0 && vec.z == 0) {
//                ad.objectData.cutDirection = NEVector::Quaternion::Euler(vec.y * -1);
//            }
//        }
//    }
//
//    return newBeatmap;
}

void InstallBeatmapDataMirrorTransformHooks(Logger &logger) {
//    INSTALL_HOOK(logger, BeatmapDataMirrorTransform_CreateTransformedData);
}

NEInstallHooks(InstallBeatmapDataMirrorTransformHooks);