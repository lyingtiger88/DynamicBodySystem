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

    void ClearMorphs(const UDynamicBodyProfile* InProfile);
    void ResetSimulation();
    void UpdateMuscles(int32 ActiveTier);
    void SimulateTissues(float Step, const FVector& LocalAcceleration, int32 ActiveTier);
    void ApplyTissues(int32 ActiveTier);
};
