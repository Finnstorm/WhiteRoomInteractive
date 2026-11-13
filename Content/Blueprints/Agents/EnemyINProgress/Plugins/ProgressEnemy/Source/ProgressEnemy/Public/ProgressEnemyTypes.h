#pragma once
#include "CoreMinimal.h"
#include "ProgressEnemyTypes.generated.h"

UENUM(BlueprintType)
enum class EProgressEnemyState : uint8 { Patrol, Chase, Search, Attacking, Blocking, Finishing, Dead };
UENUM(BlueprintType)
enum class EProgressEnemyAttack : uint8 { Light, Heavy, Taser };

/** Independent, zero-based progress levels. Negative inputs are treated as zero. */
USTRUCT(BlueprintType)
struct PROGRESSENEMY_API FProgressEnemyDifficulty
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Difficulty", meta=(ClampMin="0")) int32 Damage = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Difficulty", meta=(ClampMin="0")) int32 Block = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Difficulty", meta=(ClampMin="0")) int32 Health = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Difficulty", meta=(ClampMin="0")) int32 MovementSpeed = 0;
};

/** Small runtime model shared by the AI and its automation tests. */
struct FProgressEnemyResources
{
    int32 TaserShots = 2;
    int32 BlockHits = 3;
    float TaserRecharge = 0;
    float BlockRecharge = 0;
    void Tick(float Delta, int32 MaxBlockHits)
    {
        if (TaserRecharge > 0 && (TaserRecharge = FMath::Max(0.f, TaserRecharge - Delta)) == 0) TaserShots = 2;
        if (BlockRecharge > 0 && (BlockRecharge = FMath::Max(0.f, BlockRecharge - Delta)) == 0) BlockHits = FMath::Max(1, MaxBlockHits);
    }
    bool SpendTaser(float Cooldown)
    {
        if (TaserShots <= 0 || TaserRecharge > 0) return false;
        if (--TaserShots == 0) TaserRecharge = FMath::Max(.01f, Cooldown);
        return true;
    }
    float Absorb(float Damage, bool Guarding, float Cooldown)
    {
        if (Damage <= 0 || !Guarding || BlockHits <= 0 || BlockRecharge > 0) return Damage;
        if (--BlockHits == 0) BlockRecharge = FMath::Max(.01f, Cooldown);
        return Damage * .25f;
    }
};
