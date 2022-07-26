#include "Animation/PlayerTrack.h"
#include "Animation/AnimationHelper.h"
#include "UnityEngine/GameObject.hpp"
#include "GlobalNamespace/PauseController.hpp"
#include "GlobalNamespace/PauseMenuManager.hpp"
#include "GlobalNamespace/MultiplayerLocalActivePlayerInGameMenuController.hpp"
#include "GlobalNamespace/BeatmapObjectSpawnController.hpp"
#include "GlobalNamespace/BeatmapObjectSpawnMovementData.hpp"
#include "System/Action.hpp"
#include "NECaches.h"

using namespace TrackParenting;
using namespace UnityEngine;
using namespace GlobalNamespace;
using namespace System;
using namespace Animation;

// Events.cpp
extern BeatmapObjectSpawnController *spawnController;

static Action *didPauseEventAction;
static Action *didResumeEventAction;

DEFINE_TYPE(TrackParenting, PlayerTrack);

void PlayerTrack::ctor() {
    startPos = NEVector::Vector3::zero();
    startRot = NEVector::Quaternion::identity();
    startLocalRot = NEVector::Quaternion::identity();
    startScale = NEVector::Vector3::one();
}

void PlayerTrack::AssignTrack(Track *track) {
    PlayerTrack::track = track;

    if (!instance) {
        GameObject *gameObject = GameObject::Find("LocalPlayerGameCore");
        GameObject *noodleObject = GameObject::New_ctor("NoodlePlayerTrack");
        origin = noodleObject->get_transform();
        origin->SetParent(gameObject->get_transform()->get_parent(), true);
        gameObject->get_transform()->SetParent(origin, true);

        instance = noodleObject->AddComponent<PlayerTrack*>();
        pauseController = Object::FindObjectOfType<PauseController*>();

        if (pauseController) {
            didPauseEventAction = il2cpp_utils::MakeAction<Action*>(PlayerTrack::OnDidPauseEvent);
            pauseController->add_didPauseEvent(didPauseEventAction);
            didResumeEventAction = il2cpp_utils::MakeAction<Action*>(PlayerTrack::OnDidResumeEvent);
            pauseController->add_didResumeEvent(didResumeEventAction);
        }

        MonoBehaviour* pauseMenuManager = pauseController->pauseMenuManager ?: (MonoBehaviour*) NECaches::GameplayCoreContainer->Resolve<MultiplayerLocalActivePlayerInGameMenuController*>();
        if (pauseMenuManager)
        {
            pauseMenuManager->get_transform()->SetParent(origin, false);
        }

        startLocalRot = origin->get_localRotation();
        startPos = origin->get_localPosition();
        instance->UpdateData(true);
    }
}

void PlayerTrack::OnDidPauseEvent() {
    NELogger::GetLogger().debug("PlayerTrack::OnDidPauseEvent");
    IL2CPP_CATCH_HANDLER(
            instance->set_enabled(false);
    )
}

void PlayerTrack::OnDidResumeEvent() {
    NELogger::GetLogger().debug("PlayerTrack::OnDidResumeEvent");
    IL2CPP_CATCH_HANDLER(
            instance->set_enabled(true);
    )
}

void PlayerTrack::OnDestroy() {
    NELogger::GetLogger().debug("PlayerTrack::OnDestroy");
    if (pauseController) {
        // NELogger::GetLogger().debug("Removing action didPauseEvent %p", didPauseEventAction);
        // pauseController->remove_didPauseEvent(didPauseEventAction);
    }
    instance = nullptr;
    track = nullptr;
}

void PlayerTrack::UpdateDataOld() {
    float noteLinesDistance = 0.6f; //spawnController->NECaches::get_noteLinesDistanceFast();

    std::optional<NEVector::Quaternion> rotation = getPropertyNullable<NEVector::Quaternion>(track,
                                                                                             track->properties.rotation);
    std::optional<NEVector::Vector3> position = getPropertyNullable<NEVector::Vector3>(track,
                                                                                       track->properties.position);
    std::optional<NEVector::Quaternion> localRotation = getPropertyNullable<NEVector::Quaternion>(track,
                                                                                                  track->properties.localRotation);

    if (NECaches::LeftHandedMode) {
        rotation = Animation::MirrorQuaternionNullable(rotation);
        localRotation = Animation::MirrorQuaternionNullable(localRotation);
        position = Animation::MirrorVectorNullable(position);
    }

    NEVector::Quaternion worldRotationQuaternion = startRot;
    NEVector::Vector3 positionVector = startPos;
    if (rotation.has_value() || position.has_value()) {
        NEVector::Quaternion rotationOffset = rotation.value_or(NEVector::Quaternion::identity());
        worldRotationQuaternion = worldRotationQuaternion * rotationOffset;
        NEVector::Vector3 positionOffset = position.value_or(NEVector::Vector3::zero());
        positionVector = worldRotationQuaternion * ((positionOffset * noteLinesDistance) + startPos);
    }

    worldRotationQuaternion = worldRotationQuaternion * startLocalRot;
    if (localRotation.has_value()) {
        worldRotationQuaternion = worldRotationQuaternion * *localRotation;
    }


    origin->set_localRotation(worldRotationQuaternion);
    origin->set_localPosition(positionVector);
}

void PlayerTrack::Update() {
    UpdateData(false);
}

void PlayerTrack::UpdateData(bool force) {
    if (!track) return;

    if (track->v2) {
        return UpdateDataOld();
    }

    if (force) {
        lastCheckedTime = 0;
    }

    float noteLinesDistance = NECaches::get_noteLinesDistanceFast();

    const auto &properties = track->properties;
    auto const rotation = getPropertyNullableFast<NEVector::Quaternion>(track, properties.rotation,
                                                                        lastCheckedTime);
    auto const localRotation = getPropertyNullableFast<NEVector::Quaternion>(track, properties.localRotation,
                                                                             lastCheckedTime);
    auto const position = getPropertyNullableFast<NEVector::Vector3>(track, properties.position, lastCheckedTime);
    const auto localPosition = getPropertyNullableFast<NEVector::Vector3>(track, properties.localPosition,
                                                                          lastCheckedTime);
    const auto scale = getPropertyNullableFast<NEVector::Vector3>(track, properties.scale, lastCheckedTime);


    auto transform = origin;

    if (localRotation) {
        transform->set_localRotation(localRotation.value());
    } else if (rotation) {
        transform->set_rotation(rotation.value());
    }

    if (localPosition) {
        transform->set_localPosition(localPosition.value());
    }else if (position) {
        transform->set_position(position.value());
    }

    if (scale) {
        transform->set_localScale(scale.value());
    }

    lastCheckedTime = getCurrentTime();
}
