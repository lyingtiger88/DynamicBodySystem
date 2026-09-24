#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DynamicBodyProfile.h"
#include "DynamicBodyComponent.generated.h"

class USkeletalMeshComponent;

UCLASS(ClassGroup=(Animation), meta=(BlueprintSpawnableComponent))
class DYNAMICBODYSYSTEM_API UDynamicBodyComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UDynamicBodyComponent();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dynamic Body")
    TObjectPtr<UDynamicBodyProfile> Profile;

    /** Optional explicit target. Otherwise uses the owner's first skeletal mesh. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dynamic Body")
    TObjectPtr<USkeletalMeshComponent> TargetMesh;

    /** Auto uses the world-wide graphics setting. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dynamic Body")
    EDynamicBodyQuality QualityOverride = EDynamicBodyQuality::Auto;

    UFUNCTION(BlueprintCallable, Category="Dynamic Body")
    void SetProfile(UDynamicBodyProfile* NewProfile);

    UFUNCTION(BlueprintPure, Category="Dynamic Body")
    EDynamicBodyQuality GetEffectiveQuality() const;

    /** Call when the gameplay exertion level changes (0 at rest, 1 at maximum effort). */
    UFUNCTION(BlueprintCallable, Category="Dynamic Body|Vascular")
    void SetExertionIntensity(float Intensity);

    /** Smoothed 0..1 vascular response, including the recovery period at rest. */
    UFUNCTION(BlueprintPure, Category="Dynamic Body|Vascular")
    float GetVascularIntensity() const { return VascularIntensity; }

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    struct FTissueState
    {
        float Displacement = 0.f;
        float Velocity = 0.f;
    };

    TArray<FTissueState> TissueStates;
    FVector PreviousLocation = FVector::ZeroVector;
    FVector PreviousVelocity = FVector::ZeroVector;
    float TimeAccumulator = 0.f;
    bool bHasPreviousLocation = false;
    float ExertionIntensity = 0.f;
    float VascularIntensity = 0.f;
    float LastAppliedVascular = -1.f;
    bool bOutputsCleared = false;

    void ClearMorphs(const UDynamicBodyProfile* InProfile);
    void ResetSimulation();
    void UpdateMuscles(int32 ActiveTier);
    void SimulateTissues(float Step, const FVector& LocalAcceleration, int32 ActiveTier);
    void ApplyTissues(int32 ActiveTier);
    void AdvanceVascular(float DeltaTime);
    void ApplyVascular(int32 ActiveTier);
};
