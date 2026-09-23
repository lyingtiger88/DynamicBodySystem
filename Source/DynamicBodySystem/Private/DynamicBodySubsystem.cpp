#include "DynamicBodySubsystem.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

EDynamicBodyQuality UDynamicBodySubsystem::ResolveQuality(const AActor* Actor, EDynamicBodyQuality Override) const
{
    EDynamicBodyQuality Quality = Override == EDynamicBodyQuality::Auto ? GlobalQuality : Override;
    // Auto is deliberately deterministic until the host game supplies a hardware preset.
    if (Quality == EDynamicBodyQuality::Auto)
    {
        Quality = EDynamicBodyQuality::Medium;
    }

    if (!Actor || Quality == EDynamicBodyQuality::Off)
    {
        return Quality;
    }

    UWorld* World = GetWorld();
    const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
    const APawn* ViewPawn = Controller ? Controller->GetPawn() : nullptr;
    if (!ViewPawn || ViewPawn == Actor)
    {
        return Quality;
    }

    const float DistanceSq = FVector::DistSquared(ViewPawn->GetActorLocation(), Actor->GetActorLocation());
    if (DistanceSq > FMath::Square(5000.f))
    {
        return EDynamicBodyQuality::Off;
    }
    if (DistanceSq > FMath::Square(2500.f))
    {
        return EDynamicBodyQuality::Low;
    }
    if (DistanceSq > FMath::Square(1000.f))
    {
        return static_cast<EDynamicBodyQuality>(FMath::Min(static_cast<int32>(Quality), static_cast<int32>(EDynamicBodyQuality::Medium)));
    }
    return Quality;
}
