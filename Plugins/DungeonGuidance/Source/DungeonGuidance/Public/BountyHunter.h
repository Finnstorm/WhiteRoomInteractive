#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "GameFramework/Actor.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BountyHunter.generated.h"
class UAnimInstance;
class UMaterialInterface;
class UInputAction;
class UEnhancedInputComponent;
class UAnimMontage;
class UAnimSequence;
class UStaticMeshComponent;
class USphereComponent;
class UProjectileMovementComponent;
class USkeletalMeshComponent;
class UCharacterMovementComponent;
USTRUCT(BlueprintType)
struct DUNGEONGUIDANCE_API FBountyHunterUpgrade : public FTableRowBase
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty Hunter") bool UnlockGun = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty Hunter") int32 ExtraBullets = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty Hunter") float ShotCooldownReduction = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty Hunter") float BulletDamageBonus = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty Hunter") bool UnlockStun = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty Hunter") float StunDurationBonus = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty Hunter") bool UnlockKillRefund = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty Hunter") float WindupReduction = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty Hunter") float BulletSpeedBonus = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty Hunter") float BulletRangeBonus = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty Hunter") bool UnlockExplosion = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty Hunter") float ExplosionKnockbackBonus = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty Hunter") float ExplosionDamageBonus = 0;
};
UCLASS(BlueprintType)
class DUNGEONGUIDANCE_API UBountyHunterSettings : public UDataAsset
{
    GENERATED_BODY()
  public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun", meta = (ClampMin = "1")) int32 BaseBullets = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun", meta = (ClampMin = "0")) float BulletDamage = 40;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun", meta = (ClampMin = "0.01")) float ShotCooldown = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun", meta = (ClampMin = "0.01")) float WindupTime = .35;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile", meta = (ClampMin = "1"))
    float BulletSpeed = 2000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile", meta = (ClampMin = "1"))
    float BulletRange = 1000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile", meta = (ClampMin = "0"))
    float GravityAfterRange = 4;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile", meta = (ClampMin = "0.1"))
    float ProjectileLifeSeconds = 3;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stun", meta = (ClampMin = "0")) float StunDuration = 2;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stun", meta = (ClampMin = "0", ClampMax = "1"))
    float StunnedSpeedMultiplier = .2;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion", meta = (ClampMin = "1"))
    float ExplosionRadius = 200;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion", meta = (ClampMin = "0"))
    float ExplosionDamage = 20;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion", meta = (ClampMin = "0"))
    float ExplosionKnockback = 700;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion", meta = (ClampMin = "0"))
    float ExplosionUpward = 150;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation") float AnimationFireTime = .35;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation") float BlendInTime = .12;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation") float BlendOutTime = .16;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation") FTransform GunGripOffset;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance") TObjectPtr<UMaterialInterface> GunMaterial;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    TObjectPtr<UMaterialInterface> ProjectileMaterial;
};
USTRUCT(BlueprintType)
struct DUNGEONGUIDANCE_API FBountyHunterStats
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category = "Bounty Hunter") bool Unlocked = false;
    UPROPERTY(BlueprintReadOnly, Category = "Bounty Hunter") bool Equipped = false;
    UPROPERTY(BlueprintReadOnly, Category = "Bounty Hunter") int32 MaxBullets = 1;
    UPROPERTY(BlueprintReadOnly, Category = "Bounty Hunter") float Damage = 40;
    UPROPERTY(BlueprintReadOnly, Category = "Bounty Hunter") float Cooldown = 1;
    UPROPERTY(BlueprintReadOnly, Category = "Bounty Hunter") float Windup = .35;
    UPROPERTY(BlueprintReadOnly, Category = "Bounty Hunter") float Speed = 2000;
    UPROPERTY(BlueprintReadOnly, Category = "Bounty Hunter") float Range = 1000;
    UPROPERTY(BlueprintReadOnly, Category = "Bounty Hunter") bool Stun = false;
    UPROPERTY(BlueprintReadOnly, Category = "Bounty Hunter") float StunDuration = 2;
    UPROPERTY(BlueprintReadOnly, Category = "Bounty Hunter") bool KillRefund = false;
    UPROPERTY(BlueprintReadOnly, Category = "Bounty Hunter") bool Explosion = false;
    UPROPERTY(BlueprintReadOnly, Category = "Bounty Hunter") float ExplosionDamage = 20;
    UPROPERTY(BlueprintReadOnly, Category = "Bounty Hunter") float Knockback = 700;
};
UCLASS(ClassGroup = (Abilities), meta = (BlueprintSpawnableComponent))
class DUNGEONGUIDANCE_API UBountyHunterTargetComponent : public UActorComponent
{
    GENERATED_BODY()
  public:
    UBountyHunterTargetComponent();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty Hunter") bool bCarriesStunGun = false;
    UPROPERTY(BlueprintReadOnly, Category = "Bounty Hunter") float StunRemaining = 0;
    UFUNCTION(BlueprintCallable, Category = "Bounty Hunter") void ApplyStun(float Duration, float SpeedMultiplier);
    UFUNCTION(BlueprintCallable, Category = "Status") void ApplyFright(float Duration, float SpeedMultiplier);
    virtual void TickComponent(float Dt, ELevelTick Tick, FActorComponentTickFunction *Function) override;
    virtual void EndPlay(EEndPlayReason::Type Reason) override;

  private:
    TWeakObjectPtr<UCharacterMovementComponent> Movement;
    float OriginalSpeed = 0, AppliedSpeed = 0, Multiplier = .2;
    float FrightRemaining = 0, FrightMultiplier = .6;
    void UpdateSlow();
    void Restore();
};
UCLASS(Blueprintable)
class DUNGEONGUIDANCE_API ABountyHunterGun : public AActor
{
    GENERATED_BODY()
  public:
    ABountyHunterGun();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bounty Hunter") TObjectPtr<UStaticMeshComponent> Barrel;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bounty Hunter") TObjectPtr<UStaticMeshComponent> Grip;
};
class UBountyHunterComponent;
UCLASS(Blueprintable)
class DUNGEONGUIDANCE_API ABountyHunterProjectile : public AActor
{
    GENERATED_BODY()
  public:
    ABountyHunterProjectile();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bounty Hunter") TObjectPtr<USphereComponent> Collision;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bounty Hunter")
    TObjectPtr<UProjectileMovementComponent> Movement;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bounty Hunter") TObjectPtr<UStaticMeshComponent> Visual;
    UPROPERTY(BlueprintReadOnly, Category = "Bounty Hunter") FBountyHunterStats ShotStats;
    UPROPERTY(Transient) TObjectPtr<UBountyHunterSettings> Settings;
    void Launch(UBountyHunterComponent *Source, const FBountyHunterStats &Stats, UBountyHunterSettings *Tuning,
                FVector Direction);
    virtual void Tick(float Dt) override;
    UFUNCTION()
    void Hit(UPrimitiveComponent *Component, AActor *Other, UPrimitiveComponent *OtherComponent, FVector NormalImpulse,
             const FHitResult &Impact);

  private:
    TWeakObjectPtr<UBountyHunterComponent> SourceComponent;
    FVector PreviousPosition;
    float DistanceTravelled = 0;
    bool bImpacted = false;
};
UCLASS(ClassGroup = (Abilities), meta = (BlueprintSpawnableComponent))
class DUNGEONGUIDANCE_API UBountyHunterComponent : public UActorComponent
{
    GENERATED_BODY()
  public:
    UBountyHunterComponent();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty Hunter") TObjectPtr<UBountyHunterSettings> Settings;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty Hunter") TObjectPtr<UDataTable> Upgrades;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty Hunter") TObjectPtr<UInputAction> FireAction;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty Hunter") TObjectPtr<UInputAction> DebugReloadAction;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty Hunter") TSubclassOf<ABountyHunterGun> GunClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounty Hunter")
    TSubclassOf<ABountyHunterProjectile> ProjectileClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation") TObjectPtr<UAnimMontage> FireMontage;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation") TObjectPtr<UAnimMontage> BlockMontage;
    UPROPERTY(BlueprintReadOnly, Category = "Bounty Hunter") FBountyHunterStats Stats;
    UPROPERTY(BlueprintReadOnly, Category = "Bounty Hunter") int32 Bullets = 0;
    UPROPERTY(BlueprintReadOnly, Category = "Bounty Hunter") float CooldownRemaining = 0;
    UPROPERTY(BlueprintReadOnly, Category = "Bounty Hunter") bool bWindingUp = false;
    UPROPERTY(BlueprintReadOnly, Category = "Animation") float ArmAimWeight = 0;
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Bounty Hunter") TObjectPtr<ABountyHunterGun> Gun;
    UFUNCTION(BlueprintCallable, Category = "Bounty Hunter") void RefreshUpgrades();
    UFUNCTION(BlueprintCallable, Category = "Bounty Hunter") bool TryFire();
    UFUNCTION(BlueprintCallable, Category = "Bounty Hunter") void RefillBullets();
    UFUNCTION(BlueprintCallable, Category = "Bounty Hunter") void FireAtNotify();
    UFUNCTION(BlueprintCallable, Category = "Bounty Hunter") void CancelGunAction();
    UFUNCTION(BlueprintCallable, Category = "Bounty Hunter")
    void ResolveImpact(const FHitResult &Impact, const FBountyHunterStats &FiredStats);
    static FBountyHunterStats CalculateStats(const UBountyHunterSettings *Settings, const UDataTable *Upgrades,
                                             const TArray<FName> &Unlocked, bool Equipped);
    virtual void BeginPlay() override;
    virtual void TickComponent(float Dt, ELevelTick Tick, FActorComponentTickFunction *Function) override;
    virtual void EndPlay(EEndPlayReason::Type Reason) override;

  private:
    TWeakObjectPtr<UEnhancedInputComponent> BoundInput;
    float RefreshAccumulator = 0;
    double LastShot = -1000;
    bool bAmmoInitialized = false, bBlocking = false, bShotIssued = false;
    bool Alive() const;
    bool IsBlocking() const;
    USkeletalMeshComponent *PlayerMesh() const;
    bool DamageEnemy(AActor *Target, float Damage, const FHitResult &Hit, bool AllowRefund);
    void InputFire()
    {
        TryFire();
    }
    void UpdateBlockAnimation(bool Active);
};
UCLASS()
class DUNGEONGUIDANCE_API UAnimNotify_BountyHunterFire : public UAnimNotify
{
    GENERATED_BODY()
  public:
    virtual void Notify(USkeletalMeshComponent *Mesh, UAnimSequenceBase *Animation,
                        const FAnimNotifyEventReference &Ref) override;
};

UCLASS()
class DUNGEONGUIDANCE_API UBountyHunterAnimationLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable, Category = "Ability Animation") static void UpdateArmAim(UAnimInstance *Anim);
};
