#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "DynamicBodyProfile.h"
#include "DynamicBodySubsystem.generated.h"

class AActor;

/** World-wide quality selection; the host game saves/restores this value in its user settings. */
UCLASS()
class DYNAMICBODYSYSTEM_API UDynamicBodySubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="Dynamic Body")
    void SetGlobalQuality(EDynamicBodyQuality NewQuality) { GlobalQuality = NewQuality; }

    UFUNCTION(BlueprintPure, Category="Dynamic Body")
    EDynamicBodyQuality GetGlobalQuality() const { return GlobalQuality; }

    /** Returns the effective tier after the local camera distance limit. */
    EDynamicBodyQuality ResolveQuality(const AActor* Actor, EDynamicBodyQuality Override) const;

private:
    UPROPERTY()
    EDynamicBodyQuality GlobalQuality = EDynamicBodyQuality::Medium;
};
