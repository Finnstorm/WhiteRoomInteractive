#include "ProgressEnemyCompatibility.h"
#include "ProgressEnemyCharacter.h"
#include "Components/ActorComponent.h"
bool UProgressEnemyCompatibility::RouteHealthEvent(UActorComponent* Component, FName EventName, double DamageAmount, double DeathDelay)
{
    auto* Enemy = Component ? Cast<AProgressEnemyCharacter>(Component->GetOwner()) : nullptr;
    return Enemy && Enemy->HandleHealthEvent(Component, EventName, DamageAmount, DeathDelay);
}
