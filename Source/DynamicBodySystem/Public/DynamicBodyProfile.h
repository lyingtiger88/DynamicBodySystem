#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DynamicBodyProfile.generated.h"

UENUM(BlueprintType)
enum class EDynamicBodyQuality : uint8
{
    Off,
    Low,
    Medium,
    High,
    Cinematic,
    Auto
};

UENUM(BlueprintType)
enum class EDynamicBodyRotationAxis : uint8
{
    Pitch,
    Yaw,
    Roll
};

/** A bend around ParentBone drives a pre-authored muscle morph on the mesh. */
USTRUCT(BlueprintType)
struct DYNAMICBODYSYSTEM_API FDynamicMuscleDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Muscle")
    FName JointBone;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Muscle")
    FName ParentBone;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Muscle")
    EDynamicBodyRotationAxis Axis = EDynamicBodyRotationAxis::Pitch;

    /** Joint angle in degrees at which activation is zero. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Muscle")
    float RestAngleDegrees = 0.f;

    /** Signed angular travel (degrees) to reach full activation. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Muscle", meta=(ClampMin="-180.0", ClampMax="180.0"))
    float FullActivationDeltaDegrees = 45.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Muscle")
    FName MorphTarget;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Muscle", meta=(ClampMin="0.0", ClampMax="1.0"))
    float MaximumWeight = 1.f;

    /** Low=1, Medium=2, High=3, Cinematic=4. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Muscle", meta=(ClampMin="1", ClampMax="4"))
    int32 DetailTier = 1;
};

/** One signed axis of spring motion, mapped to positive and negative morphs. */
USTRUCT(BlueprintType)
struct DYNAMICBODYSYSTEM_API FDynamicTissueRegion
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tissue")
    FName Name;

    /** Motion direction in skeletal-mesh component space. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tissue")
    FVector LocalAxis = FVector::UpVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tissue")
    FName PositiveMorphTarget;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tissue")
    FName NegativeMorphTarget;

    /** Mass in arbitrary tuning units; values at or below zero are clamped. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tissue", meta=(ClampMin="0.01"))
    float Mass = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tissue", meta=(ClampMin="0.0"))
    float Stiffness = 65.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tissue", meta=(ClampMin="0.0"))
    float Damping = 12.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tissue", meta=(ClampMin="0.1"))
    float MaximumDisplacementCm = 5.f;

    /** Scales component acceleration; 0 disables inertial response. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tissue", meta=(ClampMin="0.0"))
    float InertiaScale = 1.f;

    /** Usually 0: gravity/rest sag should be sculpted into the neutral mesh. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tissue")
    float GravityScale = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tissue", meta=(ClampMin="1", ClampMax="4"))
    int32 DetailTier = 2;
};

/** Mesh-specific bindings. Profiles are reusable across actors sharing a compatible rig. */
UCLASS(BlueprintType)
class DYNAMICBODYSYSTEM_API UDynamicBodyProfile : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dynamic Body")
    TArray<FDynamicMuscleDefinition> Muscles;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dynamic Body")
    TArray<FDynamicTissueRegion> TissueRegions;
};
