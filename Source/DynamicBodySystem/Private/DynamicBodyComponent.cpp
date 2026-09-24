#include "DynamicBodyComponent.h"
#include "DynamicBodySubsystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

UDynamicBodyComponent::UDynamicBodyComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UDynamicBodyComponent::BeginPlay()
{
    Super::BeginPlay();
    if (!TargetMesh && GetOwner())
    {
        TargetMesh = GetOwner()->FindComponentByClass<USkeletalMeshComponent>();
    }
    if (TargetMesh)
    {
        AddTickPrerequisiteComponent(TargetMesh);
    }
    ResetSimulation();
}

void UDynamicBodyComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ClearMorphs(Profile);
    Super::EndPlay(EndPlayReason);
}

void UDynamicBodyComponent::SetProfile(UDynamicBodyProfile* NewProfile)
{
    ClearMorphs(Profile);
    Profile = NewProfile;
    ResetSimulation();
    ExertionIntensity = 0.f;
    VascularIntensity = 0.f;
    bOutputsCleared = false;
}

void UDynamicBodyComponent::SetExertionIntensity(float Intensity)
{
    ExertionIntensity = FMath::IsFinite(Intensity) ? FMath::Clamp(Intensity, 0.f, 1.f) : 0.f;
}

EDynamicBodyQuality UDynamicBodyComponent::GetEffectiveQuality() const
{
    UWorld* World = GetWorld();
    const UDynamicBodySubsystem* Subsystem = World ? World->GetSubsystem<UDynamicBodySubsystem>() : nullptr;
    if (Subsystem)
    {
        return Subsystem->ResolveQuality(GetOwner(), QualityOverride);
    }
    return QualityOverride == EDynamicBodyQuality::Auto ? EDynamicBodyQuality::Medium : QualityOverride;
}

void UDynamicBodyComponent::ResetSimulation()
{
    TissueStates.SetNum(Profile ? Profile->TissueRegions.Num() : 0);
    for (FTissueState& State : TissueStates)
    {
        State = FTissueState();
    }
    TimeAccumulator = 0.f;
    PreviousVelocity = FVector::ZeroVector;
    bHasPreviousLocation = false;
}

void UDynamicBodyComponent::ClearMorphs(const UDynamicBodyProfile* InProfile)
{
    if (!TargetMesh || !InProfile)
    {
        return;
    }
    for (const FDynamicMuscleDefinition& Muscle : InProfile->Muscles)
    {
        if (!Muscle.MorphTarget.IsNone()) TargetMesh->SetMorphTarget(Muscle.MorphTarget, 0.f);
    }
    for (const FDynamicTissueRegion& Region : InProfile->TissueRegions)
    {
        if (!Region.PositiveMorphTarget.IsNone()) TargetMesh->SetMorphTarget(Region.PositiveMorphTarget, 0.f);
        if (!Region.NegativeMorphTarget.IsNone()) TargetMesh->SetMorphTarget(Region.NegativeMorphTarget, 0.f);
    }
    for (const FDynamicVascularRegion& Region : InProfile->Vascular.Regions)
    {
        if (!Region.MorphTarget.IsNone()) TargetMesh->SetMorphTarget(Region.MorphTarget, 0.f);
    }
    if (InProfile->Vascular.PrimitiveDataIndex >= 0)
    {
        TargetMesh->SetCustomPrimitiveDataFloat(InProfile->Vascular.PrimitiveDataIndex, 0.f);
    }
    LastAppliedVascular = -1.f;
}

void UDynamicBodyComponent::AdvanceVascular(float DeltaTime)
{
    const FDynamicVascularSettings& Settings = Profile->Vascular;
    if (!Settings.bEnabled)
    {
        VascularIntensity = 0.f;
        return;
    }

    const float Threshold = FMath::Clamp(Settings.ExertionThreshold, 0.f, 0.99f);
    const float Target = FMath::Clamp((ExertionIntensity - Threshold) / (1.f - Threshold), 0.f, 1.f);
    const float ResponseSeconds = FMath::Max(Target > VascularIntensity ? Settings.BuildSeconds : Settings.RecoverySeconds, 0.01f);
    const float Blend = 1.f - FMath::Exp(-FMath::Max(DeltaTime, 0.f) / ResponseSeconds);
    VascularIntensity = FMath::Clamp(FMath::Lerp(VascularIntensity, Target, Blend), 0.f, 1.f);
}

void UDynamicBodyComponent::ApplyVascular(int32 ActiveTier)
{
    const FDynamicVascularSettings& Settings = Profile->Vascular;
    const float Threshold = FMath::Clamp(Settings.VisibilityThreshold, 0.f, 0.99f);
    const float VisibleIntensity = Settings.bEnabled && ActiveTier >= 2
        ? FMath::Clamp((VascularIntensity - Threshold) / (1.f - Threshold), 0.f, 1.f) : 0.f;

    // A reserved Custom Primitive Data slot drives a character-specific skin mask without an MID per character.
    if (Settings.PrimitiveDataIndex >= 0 &&
        (LastAppliedVascular < 0.f || FMath::Abs(VisibleIntensity - LastAppliedVascular) >= 0.01f
         || (VisibleIntensity == 0.f && LastAppliedVascular != 0.f)))
    {
        TargetMesh->SetCustomPrimitiveDataFloat(Settings.PrimitiveDataIndex, VisibleIntensity);
        LastAppliedVascular = VisibleIntensity;
    }
    for (const FDynamicVascularRegion& Region : Settings.Regions)
    {
        if (!Region.MorphTarget.IsNone())
        {
            const float Weight = Region.DetailTier <= ActiveTier
                ? VisibleIntensity * FMath::Clamp(Region.MaximumWeight, 0.f, 1.f) : 0.f;
            TargetMesh->SetMorphTarget(Region.MorphTarget, Weight);
        }
    }
}

void UDynamicBodyComponent::UpdateMuscles(int32 ActiveTier)
{
    for (const FDynamicMuscleDefinition& Muscle : Profile->Muscles)
    {
        if (Muscle.MorphTarget.IsNone()) continue;
        float Weight = 0.f;
        if (Muscle.DetailTier <= ActiveTier && !Muscle.JointBone.IsNone() && !Muscle.ParentBone.IsNone()
            && FMath::Abs(Muscle.FullActivationDeltaDegrees) > KINDA_SMALL_NUMBER)
        {
            const int32 JointIndex = TargetMesh->GetBoneIndex(Muscle.JointBone);
            const int32 ParentIndex = TargetMesh->GetBoneIndex(Muscle.ParentBone);
            if (JointIndex != INDEX_NONE && ParentIndex != INDEX_NONE)
            {
                const FQuat Joint = TargetMesh->GetBoneTransform(JointIndex).GetRotation();
                const FQuat Parent = TargetMesh->GetBoneTransform(ParentIndex).GetRotation();
                const FRotator Local = (Parent.Inverse() * Joint).Rotator();
                const float Angle = Muscle.Axis == EDynamicBodyRotationAxis::Pitch ? Local.Pitch
                    : Muscle.Axis == EDynamicBodyRotationAxis::Yaw ? Local.Yaw : Local.Roll;
                const float Delta = FMath::FindDeltaAngleDegrees(Muscle.RestAngleDegrees, Angle);
                Weight = FMath::Clamp(Delta / Muscle.FullActivationDeltaDegrees, 0.f, 1.f)
                    * FMath::Clamp(Muscle.MaximumWeight, 0.f, 1.f);
            }
        }
        TargetMesh->SetMorphTarget(Muscle.MorphTarget, Weight);
    }
}

void UDynamicBodyComponent::SimulateTissues(float Step, const FVector& LocalAcceleration, int32 ActiveTier)
{
    for (int32 Index = 0; Index < Profile->TissueRegions.Num(); ++Index)
    {
        const FDynamicTissueRegion& Region = Profile->TissueRegions[Index];
        FTissueState& State = TissueStates[Index];
        if (Region.DetailTier > ActiveTier)
        {
            State = FTissueState();
            continue;
        }
        const FVector Axis = Region.LocalAxis.GetSafeNormal();
        const float MaxDisplacement = FMath::Max(Region.MaximumDisplacementCm, 0.1f);
        const float Mass = FMath::Max(Region.Mass, 0.01f);
        const float GravityAlongAxis = FVector::DotProduct(FVector(0.f, 0.f, -980.f),
            TargetMesh->GetComponentQuat().RotateVector(Axis));
        const float Drive = -FVector::DotProduct(LocalAcceleration, Axis) * Region.InertiaScale
            + GravityAlongAxis * Region.GravityScale;
        const float Force = Drive - Region.Stiffness * State.Displacement - Region.Damping * State.Velocity;
        State.Velocity += (Force / Mass) * Step;
        State.Displacement += State.Velocity * Step;
        if (FMath::Abs(State.Displacement) >= MaxDisplacement)
        {
            State.Displacement = FMath::Clamp(State.Displacement, -MaxDisplacement, MaxDisplacement);
            State.Velocity = 0.f;
        }
    }
}

void UDynamicBodyComponent::ApplyTissues(int32 ActiveTier)
{
    for (int32 Index = 0; Index < Profile->TissueRegions.Num(); ++Index)
    {
        const FDynamicTissueRegion& Region = Profile->TissueRegions[Index];
        const float SignedWeight = Region.DetailTier <= ActiveTier
            ? TissueStates[Index].Displacement / FMath::Max(Region.MaximumDisplacementCm, 0.1f) : 0.f;
        if (!Region.PositiveMorphTarget.IsNone())
            TargetMesh->SetMorphTarget(Region.PositiveMorphTarget, FMath::Clamp(SignedWeight, 0.f, 1.f));
        if (!Region.NegativeMorphTarget.IsNone())
            TargetMesh->SetMorphTarget(Region.NegativeMorphTarget, FMath::Clamp(-SignedWeight, 0.f, 1.f));
    }
}

void UDynamicBodyComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!Profile || !TargetMesh || !IsValid(TargetMesh)) return;
    if (TissueStates.Num() != Profile->TissueRegions.Num()) ResetSimulation();
    AdvanceVascular(DeltaTime);

    const EDynamicBodyQuality Quality = GetEffectiveQuality();
    const int32 Tier = Quality == EDynamicBodyQuality::Off ? 0 : static_cast<int32>(Quality);
    if (Tier == 0 || !TargetMesh->IsVisible())
    {
        if (!bOutputsCleared)
        {
            ClearMorphs(Profile);
            ResetSimulation();
            bOutputsCleared = true;
        }
        return;
    }

    bOutputsCleared = false;
    UpdateMuscles(Tier);
    const FVector Location = TargetMesh->GetComponentLocation();
    if (!bHasPreviousLocation || DeltaTime <= SMALL_NUMBER || FVector::DistSquared(Location, PreviousLocation) > FMath::Square(300.f))
    {
        PreviousLocation = Location;
        PreviousVelocity = FVector::ZeroVector;
        bHasPreviousLocation = true;
        ApplyTissues(Tier);
        ApplyVascular(Tier);
        return;
    }

    const float SafeDelta = FMath::Max(DeltaTime, 0.001f);
    const FVector Velocity = (Location - PreviousLocation) / SafeDelta;
    const FVector WorldAcceleration = ((Velocity - PreviousVelocity) / SafeDelta).GetClampedToMaxSize(10000.f);
    PreviousLocation = Location;
    PreviousVelocity = Velocity;
    const FVector LocalAcceleration = TargetMesh->GetComponentQuat().UnrotateVector(WorldAcceleration);
    const float Step = Tier == 1 ? 1.f / 30.f : Tier == 2 ? 1.f / 45.f : 1.f / 60.f;
    TimeAccumulator = FMath::Min(TimeAccumulator + FMath::Min(DeltaTime, 0.1f), 4.f * Step);
    int32 Steps = 0;
    while (TimeAccumulator >= Step && Steps++ < 4)
    {
        SimulateTissues(Step, LocalAcceleration, Tier);
        TimeAccumulator -= Step;
    }
    ApplyTissues(Tier);
    ApplyVascular(Tier);
}
