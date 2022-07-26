#include "Animation/ParentObject.h"

#include "beatsaber-hook/shared/utils/il2cpp-type-check.hpp"

#include <utility>
#include "Animation/AnimationHelper.h"
#include "UnityEngine/GameObject.hpp"
#include "GlobalNamespace/BeatmapObjectSpawnController.hpp"
#include "GlobalNamespace/BeatmapObjectSpawnMovementData.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

#include "NECaches.h"

using namespace TrackParenting;
using namespace UnityEngine;
using namespace GlobalNamespace;
using namespace Animation;

// Events.cpp
extern BeatmapObjectSpawnController *spawnController;

DEFINE_TYPE(TrackParenting, ParentObject);



void ParentObject::OnEnable() {
    OnTransformParentChanged();
}

void ParentObject::Update() {
    UpdateData(false);
}

void ParentObject::OnTransformParentChanged() {
    UpdateData(true);
}

void ParentObject::UpdateData(bool force) {
    if (!track) return;

    if (track->v2) {
        UpdateDataOld(force);
        return;
    }
    if (force) {
        lastCheckedTime = 0;
    }

    float noteLinesDistance = NECaches::get_noteLinesDistanceFast();

    const auto& properties = track->properties;
    auto const rotation = getPropertyNullableFast<NEVector::Quaternion>(track, properties.rotation, lastCheckedTime);
    auto const localRotation = getPropertyNullableFast<NEVector::Quaternion>(track, properties.localRotation, lastCheckedTime);
    auto const position = getPropertyNullableFast<NEVector::Vector3>(track, properties.position, lastCheckedTime);
    const auto localPosition = getPropertyNullableFast<NEVector::Vector3>(track, properties.localPosition, lastCheckedTime);
    const auto scale = getPropertyNullableFast<NEVector::Vector3>(track, properties.scale, lastCheckedTime);


    auto transform = origin;

    if (rotation)
    {
        transform->set_rotation(rotation.value());
    }
    else if (localRotation)
    {
        transform->set_localRotation(localRotation.value());
    }


    if (position)
    {
        transform->set_position(position.value());
    } else if (localPosition) {
        transform->set_localPosition(localPosition.value());
    }

    if (scale)
    {
        transform->set_localScale(scale.value());
    }

    lastCheckedTime = getCurrentTime();
}

void ParentObject::UpdateDataOld(bool forced) {
    if (forced) {
        lastCheckedTime = 0;
    }
    float noteLinesDistance = NECaches::get_noteLinesDistanceFast();

    std::optional<NEVector::Quaternion> rotation = getPropertyNullableFast<NEVector::Quaternion>(track, track->properties.rotation, 0);
    std::optional<NEVector::Vector3> position = getPropertyNullableFast<NEVector::Vector3>(track, track->properties.position, 0);
    std::optional<NEVector::Quaternion> localRotation = getPropertyNullableFast<NEVector::Quaternion>(track, track->properties.localRotation, 0);
    std::optional<NEVector::Vector3> scale = getPropertyNullableFast<NEVector::Vector3>(track, track->properties.scale, 0);


    NEVector::Quaternion worldRotationQuaternion = startRot;
    NEVector::Vector3 positionVector = worldRotationQuaternion * (startPos * noteLinesDistance);
    if (rotation.has_value() || position.has_value()) {
        NEVector::Quaternion rotationOffset = rotation.value_or(NEVector::Quaternion::identity());
        worldRotationQuaternion = worldRotationQuaternion * rotationOffset;
        NEVector::Vector3 positionOffset = position.value_or(NEVector::Vector3::zero());
        positionVector = worldRotationQuaternion * ((positionOffset + startPos) * noteLinesDistance);
    }

    worldRotationQuaternion = worldRotationQuaternion * startLocalRot;

    if (localRotation.has_value()) {
        worldRotationQuaternion = worldRotationQuaternion * *localRotation;
    }

    Vector3 scaleVector = startScale;
    if (scale.has_value()) {
        scaleVector = startScale * scale.value();
    }



    origin->set_localPosition(positionVector);
    origin->set_localRotation(worldRotationQuaternion);
    origin->set_localScale(scaleVector);

    lastCheckedTime = getCurrentTime();
}



static void logTransform(Transform* transform, int hierarchy = 0) {
    if (hierarchy != 0) {
        std::string tab = std::string(hierarchy * 4, ' ');
        NELogger::GetLogger().debug("%i%sChild: %s %i", hierarchy, tab.c_str(), to_utf8(csstrtostr(transform->get_gameObject()->get_name())).c_str(), transform->GetChildCount());
    } else {
        NELogger::GetLogger().debug("Self: %s %i", to_utf8(csstrtostr(transform->get_gameObject()->get_name())).c_str(), transform->GetChildCount());
    }
    for (int i = 0; i < transform->GetChildCount(); i++) {
        auto childTransform = transform->GetChild(i);
        logTransform(childTransform, hierarchy + 1);
    }
}

void ParentObject::AssignTrack(ParentTrackEventData const& parentTrackEventData) {
    static ConstString ParentName("ParentObject");

    GameObject *parentGameObject = GameObject::New_ctor(ParentName);
    ParentObject *instance = parentGameObject->AddComponent<ParentObject *>();

    static auto get_transform = il2cpp_utils::il2cpp_type_check::FPtrWrapper<&UnityEngine::GameObject::get_transform>::get();
    static auto get_transformMB = il2cpp_utils::il2cpp_type_check::FPtrWrapper<&UnityEngine::MonoBehaviour::get_transform>::get();

    instance->origin = get_transform(parentGameObject);
    instance->track = parentTrackEventData.parentTrack;
    instance->worldPositionStays = parentTrackEventData.worldPositionStays;

    Transform *transform = get_transformMB(instance);
    if (instance->track->v2) {
        if (parentTrackEventData.pos.has_value()) {
            instance->startPos = *parentTrackEventData.pos;
            transform->set_localPosition(
                    instance->startPos * NECaches::get_noteLinesDistanceFast());
        }

        if (parentTrackEventData.rot.has_value()) {
            instance->startRot = *parentTrackEventData.rot;
            instance->startLocalRot = instance->startRot;
            transform->set_localPosition(instance->startRot * NEVector::Vector3(transform->get_localPosition()));
            transform->set_localRotation(instance->startRot);
        }

        if (parentTrackEventData.localRot.has_value()) {
            instance->startLocalRot = instance->startRot * *parentTrackEventData.localRot;
            transform->set_localRotation(NEVector::Quaternion(transform->get_localRotation()) * instance->startLocalRot);
        } else {
            if (parentTrackEventData.pos.has_value()) {
                transform->set_position(instance->startPos);
            } else if (parentTrackEventData.localPos.has_value()) {
                transform->set_localPosition(instance->startPos);
            }

            if (parentTrackEventData.rot.has_value()) {
                transform->set_localRotation(instance->startRot);
            } else if (parentTrackEventData.localRot.has_value()) {
                transform->set_localRotation(instance->startLocalRot);
            }

            if (parentTrackEventData.scale.has_value()) {
                transform->set_localScale(instance->startScale);
            }
        }
    }

    if (parentTrackEventData.scale.has_value()) {
        instance->startScale = *parentTrackEventData.scale;
        transform->set_localScale(instance->startScale);
    }

    auto startTime = std::chrono::high_resolution_clock::now();
    parentTrackEventData.parentTrack->AddGameObject(parentGameObject);

    for (auto &track : parentTrackEventData.childrenTracks) {
        if (track == parentTrackEventData.parentTrack) {
            NELogger::GetLogger().error("How could a track contain itself?");
        }

        for (auto &parentObject: ParentController::parentObjects) {
            track->gameObjectModificationEvent -= {&ParentObject::HandleGameObject, parentObject};
            parentObject->childrenTracks.erase(track);
        }

        for (auto &gameObject: track->gameObjects) {
            instance->ParentToObject(get_transform(gameObject));
        }
        instance->childrenTracks.emplace(track);
        track->gameObjectModificationEvent += {&ParentObject::HandleGameObject, instance};
    }

    ParentController::parentObjects.emplace_back(instance);
}

void ParentObject::ParentToObject(Transform *transform) {
    static auto SetParent = il2cpp_utils::il2cpp_type_check::FPtrWrapper<static_cast<void (Transform::*)(UnityEngine::Transform *, bool)>(&UnityEngine::Transform::SetParent)>::get();

    SetParent(transform, origin, worldPositionStays);
}

void ParentObject::ResetTransformParent(Transform *transform) {
    static auto SetParent = il2cpp_utils::il2cpp_type_check::FPtrWrapper<static_cast<void (Transform::*)(UnityEngine::Transform *, bool)>(&UnityEngine::Transform::SetParent)>::get();

    SetParent(transform, nullptr, false);
}

void ParentObject::HandleGameObject(Track *track, UnityEngine::GameObject *go, bool removed) {
    static auto get_transform = il2cpp_utils::il2cpp_type_check::FPtrWrapper<&UnityEngine::GameObject::get_transform>::get();


    if (removed) {
        ResetTransformParent(get_transform(go));
    } else {
        ParentToObject(get_transform(go));
    }
}

ParentObject::~ParentObject() {
    for (auto& childTrack : childrenTracks) {
        childTrack->gameObjectModificationEvent -= {&ParentObject::HandleGameObject, this};
    }
    // just in case
    track->gameObjectModificationEvent -= {&ParentObject::HandleGameObject, this};
}

void ParentController::OnDestroy() {
    NELogger::GetLogger().debug("Clearing parent objects");
    parentObjects.clear();
}