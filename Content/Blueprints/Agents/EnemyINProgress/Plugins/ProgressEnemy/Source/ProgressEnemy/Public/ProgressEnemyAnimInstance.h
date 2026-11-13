#pragma once
#include "Animation/AnimInstance.h"
#include "ProgressEnemyAnimInstance.generated.h"
UCLASS(Transient, Blueprintable)
class PROGRESSENEMY_API UProgressEnemyAnimInstance : public UAnimInstance
{
    GENERATED_BODY()
public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Enemy") float Speed = 0;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;
};
