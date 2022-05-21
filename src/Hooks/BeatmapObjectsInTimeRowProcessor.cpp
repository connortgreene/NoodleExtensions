#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "custom-json-data/shared/CustomBeatmapData.h"

#include "GlobalNamespace/BeatmapObjectsInTimeRowProcessor.hpp"
#include "GlobalNamespace/BeatmapObjectsInTimeRowProcessor_TimeSliceContainer_1.hpp"
#include "GlobalNamespace/StaticBeatmapObjectSpawnMovementData.hpp"
#include "GlobalNamespace/NoteCutDirectionExtensions.hpp"
#include "GlobalNamespace/Vector2Extensions.hpp"
#include "GlobalNamespace/BeatmapObjectsInTimeRowProcessor_SliderTailData.hpp"

#include "AssociatedData.h"
#include "NEHooks.h"
#include "custom-json-data/shared/VList.h"

#include "NEUtils.hpp"

using namespace GlobalNamespace;
using namespace CustomJSONData;

MAKE_HOOK_MATCH(BeatmapObjectsInTimeRowProcessor_HandleCurrentTimeSliceAllNotesAndSlidersDidFinishTimeSlice,
                &BeatmapObjectsInTimeRowProcessor::HandleCurrentTimeSliceAllNotesAndSlidersDidFinishTimeSlice,
                void,
                BeatmapObjectsInTimeRowProcessor* self,
                GlobalNamespace::BeatmapObjectsInTimeRowProcessor::TimeSliceContainer_1<::GlobalNamespace::BeatmapDataItem*>* allObjectsTimeSlice, float nextTimeSliceTime) {
    if (!Hooks::isNoodleHookEnabled())
        return BeatmapObjectsInTimeRowProcessor_HandleCurrentTimeSliceAllNotesAndSlidersDidFinishTimeSlice(self,
                                                                                                           allObjectsTimeSlice,
                                                                                                           nextTimeSliceTime);


    auto notesInColumnsReusableProcessingListOfLists = self->notesInColumnsReusableProcessingListOfLists;
    for (auto const &l: notesInColumnsReusableProcessingListOfLists) {
        l->Clear();
    }

    auto items = VList(allObjectsTimeSlice->items);


    auto enumerable = NoodleExtensions::of_type<NoteData *>(items);
    auto enumerable2 = NoodleExtensions::of_type<SliderData *>(items);
    auto enumerable3 = NoodleExtensions::of_type<BeatmapObjectsInTimeRowProcessor::SliderTailData *>(items);
    for (auto noteData: enumerable) {

        // TRANSPILE HERE
        // CLAMP
        auto list = VList(self->notesInColumnsReusableProcessingListOfLists[std::clamp(noteData->lineIndex, 0, 3)]);
        // TRANSPILE HERE

        bool flag = false;

        for (int j = 0; j < list.size(); j++) {
            if (list[j]->noteLineLayer > noteData->noteLineLayer) {
                list.insert_at(j, noteData);
                flag = true;
                break;
            }
        }
        if (!flag) {
            list.push_back(noteData);
        }
    }
    for (auto const& notesInColumnsReusableProcessingListOfList : self->notesInColumnsReusableProcessingListOfLists) {
        auto list2 = VList(notesInColumnsReusableProcessingListOfList);
        for (int l = 0; l < list2.size(); l++) {
            list2[l]->SetBeforeJumpNoteLineLayer((NoteLineLayer) l);
        }
    }
    for (auto sliderData: enumerable2) {
        for (auto noteData2: enumerable) {
            if (BeatmapObjectsInTimeRowProcessor::SliderHeadPositionOverlapsWithNote(sliderData, noteData2)) {
                sliderData->SetHasHeadNote(true);
                sliderData->SetHeadBeforeJumpLineLayer(noteData2->beforeJumpNoteLineLayer);
                if (sliderData->sliderType == SliderData::Type::Burst) {
                    noteData2->ChangeToBurstSliderHead();
                    if (noteData2->cutDirection == sliderData->tailCutDirection) {
                        auto line = StaticBeatmapObjectSpawnMovementData::Get2DNoteOffset(noteData2->lineIndex,
                                                                                          self->numberOfLines,
                                                                                          noteData2->noteLineLayer) -
                                    StaticBeatmapObjectSpawnMovementData::Get2DNoteOffset(sliderData->tailLineIndex,
                                                                                          self->numberOfLines,
                                                                                          sliderData->tailLineLayer);
                        float num = Vector2Extensions::SignedAngleToLine(
                                NoteCutDirectionExtensions::Direction(noteData2->cutDirection), line);
                        if (std::abs(num) <= 40.0f) {
                            noteData2->SetCutDirectionAngleOffset(num);
                            sliderData->SetCutDirectionAngleOffset(num, num);
                        }
                    }
                } else {
                    noteData2->ChangeToSliderHead();
                }
            }
        }
    }
    for (auto sliderTailData: enumerable3) {
        auto slider = sliderTailData->slider;
        for (auto noteData3: enumerable) {
            if (BeatmapObjectsInTimeRowProcessor::SliderTailPositionOverlapsWithNote(slider, noteData3)) {
                slider->SetHasTailNote(true);
                slider->SetTailBeforeJumpLineLayer(noteData3->beforeJumpNoteLineLayer);
                noteData3->ChangeToSliderTail();
            }
        }

    }
}

MAKE_HOOK_MATCH(BeatmapObjectsInTimeRowProcessor_ProcessAllNotesInTimeRow,
                &BeatmapObjectsInTimeRowProcessor::HandleCurrentTimeSliceAllNotesAndSlidersDidFinishTimeSlice, void,
                BeatmapObjectsInTimeRowProcessor *self,
                GlobalNamespace::BeatmapObjectsInTimeRowProcessor::TimeSliceContainer_1<::GlobalNamespace::BeatmapDataItem*>* allObjectsTimeSlice,
                float nextTimeSliceTime) {
    if (!Hooks::isNoodleHookEnabled())
        return BeatmapObjectsInTimeRowProcessor_ProcessAllNotesInTimeRow(self, allObjectsTimeSlice, nextTimeSliceTime);

    auto items = allObjectsTimeSlice->items;

    std::vector<CustomNoteData*> customNotes;

    for (auto o : VList(items)) {
        if (o && il2cpp_utils::AssignableFrom<CustomNoteData*>(o->klass)) customNotes.emplace_back((CustomNoteData*) o);
    }

    if (customNotes.empty())
        return BeatmapObjectsInTimeRowProcessor_ProcessAllNotesInTimeRow(self, allObjectsTimeSlice, nextTimeSliceTime);




    std::unordered_map<float, std::vector<CustomNoteData *>> notesInColumn;
    for (auto noteData : customNotes) {
        float lineIndex = noteData->lineIndex - 2.0f;
        float lineLayer = noteData->noteLineLayer;
        if (noteData->customData->value) {
            rapidjson::Value const& customData = *noteData->customData->value;
            auto pos = customData.FindMember(NoodleExtensions::Constants::V2_POSITION.data());
            if (pos != customData.MemberEnd()) {
                int size = pos->value.Size();
                if (size >= 1) {
                    lineIndex = pos->value[0].GetFloat();
                }
                if (size >= 2) {
                    lineLayer = pos->value[1].GetFloat();
                }
            }
        }

        std::vector<CustomNoteData *> &list = notesInColumn[lineIndex];

        bool flag = false;
        for (int k = 0; k < list.size(); k++) {
            float listLineLayer = list[k]->noteLineLayer;
            if (noteData->customData->value) {
                rapidjson::Value const& customData = *noteData->customData->value;
                auto listPos = customData.FindMember(NoodleExtensions::Constants::V2_POSITION.data());
                if (listPos != customData.MemberEnd()) {
                    if (listPos->value.Size() >= 2) {
                        listLineLayer = listPos->value[1].GetFloat();
                    }
                }
            }

            if (listLineLayer > lineLayer) {
                list.insert(list.begin() + k, noteData);
                flag = true;
                break;
            }
        }

        if (!flag) {
            list.push_back(noteData);
        }
    }

    for (auto &pair : notesInColumn) {
        auto &list = pair.second;
        for (int m = 0; m < list.size(); m++) {
            BeatmapObjectAssociatedData &ad = getAD(list[m]->customData);
            ad.startNoteLineLayer = (float) m;
        }
    }

    BeatmapObjectsInTimeRowProcessor_ProcessAllNotesInTimeRow(self, allObjectsTimeSlice, nextTimeSliceTime);
}

MAKE_HOOK_MATCH(BeatmapObjectsInTimeRowProcessor_ProcessColorNotesInTimeRow,
                &BeatmapObjectsInTimeRowProcessor::HandleCurrentTimeSliceColorNotesDidFinishTimeSlice, void,
                BeatmapObjectsInTimeRowProcessor* self,
                GlobalNamespace::BeatmapObjectsInTimeRowProcessor::TimeSliceContainer_1<::GlobalNamespace::NoteData*>* currentTimeSlice,
                float nextTimeSliceTime) {
    if (!Hooks::isNoodleHookEnabled())
        return BeatmapObjectsInTimeRowProcessor_ProcessColorNotesInTimeRow(self, currentTimeSlice, nextTimeSliceTime);

    auto *colorNotesData = reinterpret_cast<List<CustomNoteData *> *>(currentTimeSlice->items);

    int const customNoteCount = colorNotesData->get_Count();

    if (customNoteCount == 2) {
        std::array<float, 2> lineIndexes{}, lineLayers{};

        for (int i = 0; i < customNoteCount; i++) {
            auto noteData = colorNotesData->get_Item(i);

            float lineIndex = noteData->lineIndex - 2.0f;
            float lineLayer = noteData->noteLineLayer;
            if (noteData->customData->value) {
                rapidjson::Value const& customData = *noteData->customData->value;
                auto pos = customData.FindMember(NoodleExtensions::Constants::V2_POSITION.data());
                if (pos != customData.MemberEnd()) {
                    int size = pos->value.Size();
                    if (size >= 1) {
                        lineIndex = pos->value[0].GetFloat();
                    }
                    if (size >= 2) {
                        lineLayer = pos->value[1].GetFloat();
                    }
                }
            }

            lineIndexes[i] = lineIndex;
            lineLayers[i] = lineLayer;
        }

        auto firstNote = colorNotesData->get_Item(0);
        auto secondNote = colorNotesData->get_Item(1);

        if (firstNote->get_colorType() != secondNote->get_colorType() &&
            ((firstNote->get_colorType() == ColorType::ColorA && lineIndexes[0] > lineIndexes[1]) ||
             (firstNote->get_colorType() == ColorType::ColorB && lineIndexes[0] < lineIndexes[1]))) {
            for (int i = 0; i < customNoteCount; i++) {
                // god aero I hate the mess of code you've made

                auto noteData = colorNotesData->get_Item(i);

                NEVector::Vector2 flipVec;

                flipVec.x = lineIndexes[1 - i];


                float flipYSide = (lineIndexes[i] > lineIndexes[1 - i]) ? 1 : -1;
                if ((lineIndexes[i] > lineIndexes[1 - i] && lineLayers[i] < lineLayers[1 - i]) ||
                    (lineIndexes[i] < lineIndexes[1 - i] &&
                     lineLayers[i] > lineLayers[1 - i])) {
                    flipYSide *= -1.0f;
                }

                flipVec.y = flipYSide;

                auto &noteAD = getAD(noteData->customData);
                noteAD.flip = flipVec;
            }
        }
    }


    BeatmapObjectsInTimeRowProcessor_ProcessColorNotesInTimeRow(self, currentTimeSlice, nextTimeSliceTime);
}


void InstallBeatmapObjectsInTimeRowProcessorHooks(Logger &logger) {
    INSTALL_HOOK(logger, BeatmapObjectsInTimeRowProcessor_ProcessAllNotesInTimeRow);
    INSTALL_HOOK(logger, BeatmapObjectsInTimeRowProcessor_ProcessColorNotesInTimeRow);

    INSTALL_HOOK(logger, BeatmapObjectsInTimeRowProcessor_HandleCurrentTimeSliceAllNotesAndSlidersDidFinishTimeSlice)
}

NEInstallHooks(InstallBeatmapObjectsInTimeRowProcessorHooks);