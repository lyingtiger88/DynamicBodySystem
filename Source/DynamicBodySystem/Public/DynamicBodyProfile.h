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

/** A subtle, authored vein-bulge morph. It should affect only the named anatomical region. */
USTRUCT(BlueprintType)
struct DYNAMICBODYSYSTEM_API FDynamicVascularRegion
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vascular")
    FName MorphTarget;

    /** Scale of the authored full morph. Keep the neutral mesh and output natural. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vascular", meta=(ClampMin="0.0", ClampMax="1.0"))
    float MaximumWeight = 0.25f;

    /** Detailed vein geometry defaults to High and above. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vascular", meta=(ClampMin="1", ClampMax="4"))
    int32 DetailTier = 3;
};

/** Optional skin material and vein morph response to sustained exertion. */
USTRUCT(BlueprintType)
struct DYNAMICBODYSYSTEM_API FDynamicVascularSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vascular")
    bool bEnabled = false;

    /** Exertion below this 0..1 input does not build a vascular response. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vascular", meta=(ClampMin="0.0", ClampMax="0.99"))
    float ExertionThreshold = 0.55f;

    /** Seconds to approach an active exertion level. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vascular", meta=(ClampMin="0.01"))
    float BuildSeconds = 3.f;

    /** Seconds to recover after exertion stops; the effect remains visible briefly at rest. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vascular", meta=(ClampMin="0.01"))
    float RecoverySeconds = 12.f;

    /** Response below this value is visually suppressed. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vascular", meta=(ClampMin="0.0", ClampMax="0.99"))
    float VisibilityThreshold = 0.18f;

    /** Reserved index in the mesh's Custom Primitive Data; -1 disables material output. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vascular", meta=(ClampMin="-1"))
    int32 PrimitiveDataIndex = -1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vascular")
    TArray<FDynamicVascularRegion> Regions;
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

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dynamic Body")
    FDynamicVascularSettings Vascular;
};
