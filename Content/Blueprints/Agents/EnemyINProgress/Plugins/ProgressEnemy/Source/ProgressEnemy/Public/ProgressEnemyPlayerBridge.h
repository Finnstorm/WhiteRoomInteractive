#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProgressEnemyPlayerBridge.generated.h"

class UEnhancedInputComponent;
class APlayerController;

/** Adds compatibility without changing the player's Blueprint or input mappings. */
UCLASS()
class PROGRESSENEMY_API UProgressEnemyPlayerBridge : public UActorComponent
{
    GENERATED_BODY()
public:
    UProgressEnemyPlayerBridge();
    static UProgressEnemyPlayerBridge* FindOrAdd(AActor* Player);
    void Stun(float Seconds);
    virtual void TickComponent(float Delta, ELevelTick Type, FActorComponentTickFunction* Tick) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    TWeakObjectPtr<UEnhancedInputComponent> BoundInput;
    TWeakObjectPtr<APlayerController> StunnedController;
    uint32 InputBindingHandle = 0;
    float StunTime = 0;
    void FinisherPressed();
    void ReleaseStun();
};
