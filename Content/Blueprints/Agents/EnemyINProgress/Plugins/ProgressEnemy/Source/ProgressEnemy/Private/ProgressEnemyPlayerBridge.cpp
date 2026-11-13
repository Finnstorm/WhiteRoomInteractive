#include "ProgressEnemyPlayerBridge.h"
#include "ProgressEnemyCharacter.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UProgressEnemyPlayerBridge::UProgressEnemyPlayerBridge()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = .05f;
}
UProgressEnemyPlayerBridge* UProgressEnemyPlayerBridge::FindOrAdd(AActor* Player)
{
    if (!IsValid(Player)) return nullptr;
    if (auto* Existing = Player->FindComponentByClass<UProgressEnemyPlayerBridge>()) return Existing;
    auto* Bridge = NewObject<UProgressEnemyPlayerBridge>(Player);
    Player->AddInstanceComponent(Bridge);
    Bridge->RegisterComponent();
    return Bridge;
}
void UProgressEnemyPlayerBridge::TickComponent(float Delta, ELevelTick Type, FActorComponentTickFunction* Tick)
{
    Super::TickComponent(Delta, Type, Tick);
    APawn* Pawn = Cast<APawn>(GetOwner());
    auto* Input = Pawn ? Cast<UEnhancedInputComponent>(Pawn->InputComponent) : nullptr;
    if (Input && BoundInput.Get() != Input)
    {
        if (BoundInput.IsValid()) BoundInput->RemoveBindingByHandle(InputBindingHandle);
        const UInputAction* Action = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/Actions/IA_Finisher.IA_Finisher"));
        if (Action)
        {
            InputBindingHandle = Input->BindAction(Action, ETriggerEvent::Started, this, &UProgressEnemyPlayerBridge::FinisherPressed).GetHandle();
            BoundInput = Input;
        }
    }
    if (StunTime > 0)
    {
        StunTime -= Delta;
        if (StunTime <= 0) ReleaseStun();
    }
}
void UProgressEnemyPlayerBridge::FinisherPressed()
{
    ACharacter* Player = Cast<ACharacter>(GetOwner());
    auto* PC = Player ? Cast<APlayerController>(Player->GetController()) : nullptr;
    if (!PC || !PC->IsLocalController()) return;
    FVector Eye; FRotator Rotation;
    PC->GetPlayerViewPoint(Eye, Rotation);
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(ProgressEnemyFinisher), false, Player);
    if (GetWorld()->LineTraceSingleByChannel(Hit, Eye, Eye + Rotation.Vector() * 350.f, ECC_Visibility, Params))
        if (auto* Enemy = Cast<AProgressEnemyCharacter>(Hit.GetActor()))
            if (Enemy->CanBeFinished()) Enemy->BeginFinisher(Player);
}
void UProgressEnemyPlayerBridge::Stun(float Seconds)
{
    APawn* Pawn = Cast<APawn>(GetOwner());
    auto* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
    if (!PC || Seconds <= 0) return;
    if (StunnedController.Get() != PC)
    {
        ReleaseStun();
        StunnedController = PC;
        PC->SetIgnoreMoveInput(true);
    }
    StunTime = FMath::Max(StunTime, Seconds);
    if (auto* Character = Cast<ACharacter>(Pawn)) Character->GetCharacterMovement()->StopMovementImmediately();
}
void UProgressEnemyPlayerBridge::ReleaseStun()
{
    if (StunnedController.IsValid()) StunnedController->SetIgnoreMoveInput(false);
    StunnedController.Reset();
    StunTime = 0;
}
void UProgressEnemyPlayerBridge::EndPlay(const EEndPlayReason::Type Reason)
{
    if (BoundInput.IsValid()) BoundInput->RemoveBindingByHandle(InputBindingHandle);
    ReleaseStun();
    Super::EndPlay(Reason);
}
