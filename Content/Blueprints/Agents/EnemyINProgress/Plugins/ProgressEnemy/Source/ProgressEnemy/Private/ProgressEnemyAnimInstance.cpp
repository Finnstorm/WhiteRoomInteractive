#include "ProgressEnemyAnimInstance.h"
#include "GameFramework/Pawn.h"
void UProgressEnemyAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);
    if (const APawn* Pawn = TryGetPawnOwner()) Speed = Pawn->GetVelocity().Size2D();
}
