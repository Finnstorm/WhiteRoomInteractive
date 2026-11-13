#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ProgressEnemyTypes.h"
#include "ProgressEnemyCharacter.generated.h"

class UAnimMontage;
class UAnimSequence;
class UWidgetComponent;
class AAIController;
class UStaticMeshComponent;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FProgressEnemyStateChanged, EProgressEnemyState, Previous, EProgressEnemyState, Current);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FProgressEnemyAttackStarted, EProgressEnemyAttack, Attack, float, Windup);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FProgressEnemyTaserFired, FVector, Start, FVector, End);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FProgressEnemyDied);

UCLASS(Blueprintable)
class PROGRESSENEMY_API AProgressEnemyCharacter : public ACharacter
{
    GENERATED_BODY()
public:
    AProgressEnemyCharacter();
    virtual void PostInitializeComponents() override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void Tick(float DeltaSeconds) override;
    virtual float TakeDamage(float DamageAmount, const FDamageEvent& Event, AController* EventInstigator, AActor* Causer) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Difficulty", meta=(ExposeOnSpawn=true)) FProgressEnemyDifficulty Difficulty;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Difficulty", meta=(ClampMin="1")) float BaseHealth = 100;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Difficulty", meta=(ClampMin="0")) float HealthPerLevel = 20;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Difficulty", meta=(ClampMin="0")) float DamageIncreasePerLevel = .15f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Difficulty", meta=(ClampMin="1")) int32 BaseBlockHits = 3;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Difficulty", meta=(ClampMin="0")) int32 BlockHitsPerLevel = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Difficulty", meta=(ClampMin="0")) float SpeedIncreasePerLevel = .08f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Difficulty", meta=(ClampMin="1")) float MaximumSpeed = 750;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Intelligence", meta=(ClampMin=".01")) float IntelligenceLevelForMaximum = 10;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Enemy|Intelligence") float IntelligenceLevel = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Enemy|Intelligence") float IntelligenceAlpha = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Enemy|Intelligence") float DecisionInterval = .35f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Enemy|Intelligence") float PathUpdateInterval = .4f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Movement", meta=(ClampMin="1")) float PatrolSpeed = 160;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Movement", meta=(ClampMin="1")) float ChaseSpeed = 360;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Movement", meta=(ClampMin="1")) float PatrolRadius = 900;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Movement", meta=(ClampMin="0")) float PatrolPause = 1.5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Movement", meta=(ClampMin=".1")) float SearchDuration = 10;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Movement", meta=(ClampMin="1")) float SearchRadius = 500;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Senses", meta=(ClampMin="1")) float SightRadius = 1800;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Senses", meta=(ClampMin="0", ClampMax="180")) float SightHalfAngle = 65;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Senses", meta=(ClampMin="1")) float ProximityRadius = 250;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Senses", meta=(ClampMin=".1")) float ProximityNoticeSeconds = 2.5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Senses", meta=(ClampMin="0")) float SightMemorySeconds = 2;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Attacks") bool bEnableLightMelee = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Attacks") bool bEnableHeavyMelee = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Attacks") bool bEnableTaser = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Attacks") bool bEnableBlock = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Attacks", meta=(ClampMin="0")) float LightDamage = 12;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Attacks", meta=(ClampMin="0")) float HeavyDamage = 25;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Attacks", meta=(ClampMin="0")) float TaserDamage = 8;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Attacks", meta=(ClampMin="1")) float MeleeRange = 160;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Attacks", meta=(ClampMin="1")) float TaserRange = 1100;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Attacks", meta=(ClampMin="0")) float TaserMinimumRange = 230;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Attacks", meta=(ClampMin=".01")) float LightAnimationSpeed = 1.35f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Attacks", meta=(ClampMin=".01")) float HeavyAnimationSpeed = .75f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Attacks", meta=(ClampMin=".01")) float MeleeWindup = .5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Attacks", meta=(ClampMin=".01")) float MeleeRecovery = .8f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Attacks", meta=(ClampMin="0", ClampMax="1")) float HeavyChance = .35f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Taser", meta=(ClampMin=".01")) float TaserWindup = .45f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Taser", meta=(ClampMin=".01")) float TaserShotInterval = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Taser", meta=(ClampMin=".01")) float TaserCooldown = 6;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Taser", meta=(ClampMin="0")) float TaserStunSeconds = 1.25f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Block", meta=(ClampMin=".01")) float BlockDuration = 1.2f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Block", meta=(ClampMin=".01")) float BlockRechargeSeconds = 6;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Block", meta=(ClampMin="0", ClampMax="1")) float BlockChance = .25f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Block", meta=(ClampMin="0", ClampMax="180")) float BlockHalfAngle = 100;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Presentation") TArray<TObjectPtr<UAnimMontage>> MeleeMontages;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Presentation") TObjectPtr<UAnimSequence> BlockAnimation;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Presentation") TObjectPtr<UAnimMontage> TaserMontage;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Presentation") TObjectPtr<UAnimMontage> HitReactionMontage;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Presentation") TObjectPtr<UAnimMontage> EnemyFinisherMontage;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Presentation") TObjectPtr<UAnimMontage> PlayerFinisherMontage;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Presentation") bool bDrawDebug = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Presentation", meta=(ClampMin="0")) float CorpseLifetime = 8;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Enemy|Components") TObjectPtr<UActorComponent> EnemyHealth;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Enemy|Components") TObjectPtr<UActorComponent> EnemyCombat;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Enemy|Components") TObjectPtr<UWidgetComponent> FinisherPrompt;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Enemy|Components") TObjectPtr<UStaticMeshComponent> TaserBeam;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Enemy|Runtime") EProgressEnemyState State = EProgressEnemyState::Patrol;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Enemy|Runtime") TObjectPtr<AActor> Target;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Enemy|Runtime") FVector LastKnownPlayerLocation;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Enemy|Runtime") float CurrentHealth = 100;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Enemy|Runtime") float MaximumHealth = 100;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Enemy|Runtime") int32 TaserShotsRemaining = 2;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Enemy|Runtime") int32 BlockHitsRemaining = 3;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Enemy|Runtime") float TaserRechargeRemaining = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Enemy|Runtime") float BlockRechargeRemaining = 0;
    UPROPERTY(BlueprintAssignable, Category="Enemy|Events") FProgressEnemyStateChanged OnStateChanged;
    UPROPERTY(BlueprintAssignable, Category="Enemy|Events") FProgressEnemyAttackStarted OnAttackStarted;
    UPROPERTY(BlueprintAssignable, Category="Enemy|Events") FProgressEnemyTaserFired OnTaserFired;
    UPROPERTY(BlueprintAssignable, Category="Enemy|Events") FProgressEnemyDied OnDied;

    UFUNCTION(BlueprintCallable, Category="Enemy|Difficulty") void SetDifficultyLevels(int32 Damage, int32 Block, int32 Health, int32 MovementSpeed);
    UFUNCTION(BlueprintCallable, Category="Enemy|Difficulty") void ApplyDifficulty(FProgressEnemyDifficulty Levels, bool bRefillHealth = false);
    UFUNCTION(BlueprintCallable, Category="Enemy|Attacks") void SetAttackEnabled(EProgressEnemyAttack Attack, bool bEnabled);
    UFUNCTION(BlueprintCallable, Category="Enemy|Attacks") bool TryAttack(EProgressEnemyAttack Attack);
    UFUNCTION(BlueprintCallable, Category="Enemy|Block") bool TryBlock();
    UFUNCTION(BlueprintCallable, Category="Enemy|Senses") void NotifyPlayerDamage(AActor* Source);
    UFUNCTION(BlueprintCallable, Category="Enemy|Health") void BeginFinisher(ACharacter* Player);
    UFUNCTION(BlueprintPure, Category="Enemy|Health") bool CanBeFinished() const;
    UFUNCTION(BlueprintPure, Category="Enemy|Runtime") int32 GetMaximumBlockHits() const;
    UFUNCTION(BlueprintPure, Category="Enemy|Intelligence") float GetHeavyAttackChance() const;
    UFUNCTION(BlueprintPure, Category="Enemy|Intelligence") float GetBlockDecisionChance() const;
    // Used only by the compatibility router inside AC_EnemyHealth.
    bool HandleHealthEvent(UActorComponent* Component, FName Event, double Damage, double Delay);

private:
    FProgressEnemyResources Resources;
    FVector PatrolOrigin, MoveGoal;
    float ProximityTime = 0, UnseenTime = 0, SearchTime = 0, PauseTime = 0;
    float ActionTime = 0, ImpactTime = 0, DecisionTime = 0, RepathTime = 0;
    float BeamTime = 0;
    bool bHasMoveGoal = false, bImpactPending = false, bHealthInitialized = false, bVisible = false;
    EProgressEnemyAttack ActiveAttack = EProgressEnemyAttack::Light;
    TWeakObjectPtr<ACharacter> FinishingPlayer;
    TEnumAsByte<EMovementMode> PlayerPreviousMovement;
    uint8 PlayerPreviousCustomMovement = 0;
    FTimerHandle FinishTimer;
    void SetState(EProgressEnemyState NewState);
    bool IsAttackEnabled(EProgressEnemyAttack Attack) const;
    bool HasClearPathTo(const AActor* Actor) const;
    bool Sees(const AActor* Actor) const;
    bool IsLivingPlayer(const AActor* Actor) const;
    void Think(float Delta);
    void MoveTo(const FVector& Goal, float Acceptance);
    bool RandomGoal(const FVector& Origin, float Radius);
    void StopMoving();
    void ResolveAttack();
    void UpdateHealthPresentation();
    void ApplyHealthDamage(float Amount);
    void Die();
    void FinishFinisher();
    void ReleasePlayer();
    void PlayBlock();
    void StopBlock();
    void SyncResources();
};
