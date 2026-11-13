#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProgressEnemyCompatibility.generated.h"
UCLASS()
class PROGRESSENEMY_API UProgressEnemyCompatibility : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    /** True consumes only events belonging to the new native enemy; false preserves the old graph. */
    UFUNCTION(BlueprintCallable, Category="Enemy|Compatibility")
    static bool RouteHealthEvent(UActorComponent* Component, FName EventName, double DamageAmount = 0, double DeathDelay = 0);
};
