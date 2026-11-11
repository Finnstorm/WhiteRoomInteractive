#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Entertainer.generated.h"
class UInputAction;
class UAnimMontage;
class UEnhancedInputComponent;
class UProgressBar;
class UTextBlock;
class UEntertainerCaptureBridge;
USTRUCT(BlueprintType)
struct DUNGEONGUIDANCE_API FEntertainerUpgrade : public FTableRowBase
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entertainer Upgrade") bool Unlock = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entertainer Upgrade") float Refund = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entertainer Upgrade") float CostReduction = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entertainer Upgrade") bool Fright = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entertainer Upgrade") float FrightDurationBonus = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entertainer Upgrade") float FrightRadiusBonus = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entertainer Upgrade") float SlowBonus = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entertainer Upgrade") float TargetRangeBonus = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entertainer Upgrade") float WindupReduction = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entertainer Upgrade") float CooldownReduction = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entertainer Upgrade") int32 ExtraTargets = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entertainer Upgrade") float SecondaryRadiusBonus = 0;
};
UCLASS(BlueprintType)
class DUNGEONGUIDANCE_API UEntertainerSettings : public UDataAsset
{
    GENERATED_BODY()
  public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo") float Capacity = 100;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo") float HitScore = 8;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo") float KillScore = 12;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo") float BlockScore = 6;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo") float FinisherScore = 20;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo") float DodgeScore = 10;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo") float VarietyScore = 4;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo") float RepeatWindow = 3;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo") float RepeatMultiplier = .25;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo") float DecayDelay = 3;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo") float DecayPerSecond = 8;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo") float DamagePenalty = 20;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
    TArray<float> RankThresholds = {
        0,
        15,
        30,
        45,
        60,
        80,
        100
    };
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
    TArray<FString> RankNames = {
        TEXT("D"),
        TEXT("C"),
        TEXT("B"),
        TEXT("A"),
        TEXT("S"),
        TEXT("SS"),
        TEXT("SSS")
    };
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snap") float Cost = 100;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snap") float Cooldown = 20;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snap") float Windup = .5;
    // Target selection only. Projection distances remain owned by the existing capture Blueprint.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snap", meta = (Units = "cm")) float TargetRange = 800;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snap") float AnimationContactTime = .5;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snap") float CaptureTimeout = 3;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fright") float FrightRadius = 300;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fright") float FrightDuration = 2;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fright") float SlowFraction = .4;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Finale") float SecondaryRadius = 300;
};
USTRUCT(BlueprintType)
struct DUNGEONGUIDANCE_API FEntertainerStats
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category = "Entertainer") bool Unlocked = false;
    UPROPERTY(BlueprintReadOnly, Category = "Entertainer") bool Equipped = false;
    UPROPERTY(BlueprintReadOnly, Category = "Entertainer") float Cost = 100;
    UPROPERTY(BlueprintReadOnly, Category = "Entertainer") float Refund = 0;
    UPROPERTY(BlueprintReadOnly, Category = "Entertainer") float Cooldown = 20;
    UPROPERTY(BlueprintReadOnly, Category = "Entertainer") float Windup = .5;
    UPROPERTY(BlueprintReadOnly, Category = "Entertainer") float TargetRange = 800;
    UPROPERTY(BlueprintReadOnly, Category = "Entertainer") bool Fright = false;
    UPROPERTY(BlueprintReadOnly, Category = "Entertainer") float FrightRadius = 300;
    UPROPERTY(BlueprintReadOnly, Category = "Entertainer") float FrightDuration = 2;
    UPROPERTY(BlueprintReadOnly, Category = "Entertainer") float SlowFraction = .4;
    UPROPERTY(BlueprintReadOnly, Category = "Entertainer") int32 Targets = 1;
    UPROPERTY(BlueprintReadOnly, Category = "Entertainer") float SecondaryRadius = 300;
};
UCLASS(ClassGroup = (Abilities), meta = (BlueprintSpawnableComponent))
class DUNGEONGUIDANCE_API UEntertainerComponent : public UActorComponent
{
    GENERATED_BODY()
  public:
    UEntertainerComponent();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entertainer") TObjectPtr<UEntertainerSettings> Settings;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entertainer") TObjectPtr<UDataTable> Upgrades;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entertainer") TObjectPtr<UInputAction> ActivateAction;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entertainer") TObjectPtr<UAnimMontage> SnapMontage;
    UPROPERTY(BlueprintReadOnly, Category = "Entertainer") FEntertainerStats Stats;
    UPROPERTY(BlueprintReadOnly, Category = "Entertainer") float Combo = 0;
    UPROPERTY(BlueprintReadOnly, Category = "Entertainer") float CooldownRemaining = 0;
    UPROPERTY(BlueprintReadOnly, Category = "Entertainer") bool Pending = false;
    UPROPERTY(BlueprintReadOnly, Category = "Entertainer") FString LastAction;
    UFUNCTION(BlueprintCallable, Category = "Entertainer") bool TryActivate();
    UFUNCTION(BlueprintCallable, Category = "Entertainer") void Contact();
    UFUNCTION(BlueprintCallable, Category = "Entertainer") void Cancel();
    UFUNCTION(BlueprintCallable, Category = "Entertainer") void ResetRun();
    UFUNCTION(BlueprintCallable, Category = "Entertainer") void RefreshUpgrades();
    UFUNCTION(BlueprintPure, Category = "Entertainer") FString Rank() const;
    void Score(FName Action, FName Category, float Amount);
    void TookDamage();
    bool CompleteCapture(UEntertainerCaptureBridge *Bridge, AActor *Target, AActor *Projection, int32 Token,
                         bool Primary);
    bool Alive() const;
    bool HasIssuedContact() const
    {
        return ContactIssued;
    }
    static FEntertainerStats CalculateStats(const UEntertainerSettings *Settings, const UDataTable *Table,
                                            const TArray<FName> &IDs, bool Equipped);
    virtual void TickComponent(float Dt, ELevelTick Tick, FActorComponentTickFunction *Fn) override;
    virtual void EndPlay(EEndPlayReason::Type Reason) override;

  private:
    TWeakObjectPtr<UEnhancedInputComponent> BoundInput;
    TWeakObjectPtr<AActor> PrimaryTarget;
    TArray<TWeakObjectPtr<UEntertainerCaptureBridge>> Captures;
    TMap<FName, double> ActionTimes;
    FName LastCategory;
    double Age = 0, LastScore = -1000, Started = 0;
    float RefreshAge = 0, Reserved = 0;
    int32 CastToken = 0;
    bool ContactIssued = false, PrimaryCommitted = false;
    FEntertainerStats CastStats;
    void InputActivate()
    {
        TryActivate();
    }
    AActor *SelectTarget() const;
    bool ValidTarget(AActor *Target) const;
    void Frighten(FVector Center);
};
UCLASS(ClassGroup = (Abilities), meta = (BlueprintSpawnableComponent))
class DUNGEONGUIDANCE_API UEntertainerCaptureBridge : public UActorComponent
{
    GENERATED_BODY()
  public:
    UEntertainerCaptureBridge();
    bool Reserve(UEntertainerComponent *Source, AActor *Target, int32 Token, bool Primary);
    bool ValidateProjection(AActor *Target) const;
    void Start();
    void Abort();
    UFUNCTION(BlueprintCallable, Category = "Entertainer Capture") void PrepareCapture(AActor *Target);
    UFUNCTION(BlueprintCallable, Category = "Entertainer Capture") void RestoreCapture();
    UFUNCTION(BlueprintCallable, Category = "Entertainer Capture") bool Complete(AActor *Target, AActor *Projection);
    virtual void EndPlay(EEndPlayReason::Type Reason) override;

  private:
    TWeakObjectPtr<UEntertainerComponent> Source;
    TWeakObjectPtr<AActor> Enemy;
    int32 Token = 0;
    bool Primary = false, Active = false, Started = false;
    struct FDepthState
    {
        TWeakObjectPtr<UPrimitiveComponent> Mesh;
        bool Enabled;
        int32 Stencil;
    };
    TArray<FDepthState> DepthStates;
};
UCLASS()
class DUNGEONGUIDANCE_API UEntertainerCombatTracker : public UActorComponent
{
    GENERATED_BODY()
  public:
    float PreviousHP = 0;
    bool Suppress = false, KillAwarded = false, Finisher = false;
    TWeakObjectPtr<AActor> DamageSource;
    FName Category = TEXT("Melee");
};
UCLASS()
class DUNGEONGUIDANCE_API UEntertainerCombatEvents : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable, Category = "Entertainer Combat") static void BeforeDamage(UActorComponent *Health);
    UFUNCTION(BlueprintCallable, Category = "Entertainer Combat") static void AfterDamage(UActorComponent *Health);
    UFUNCTION(BlueprintCallable, Category = "Entertainer Combat")
    static void SuccessfulBlock(UActorComponent *Combat, float Incoming, float Applied);
    UFUNCTION(BlueprintCallable, Category = "Entertainer Combat") static void BeginFinisher(UActorComponent *Health);
    UFUNCTION(BlueprintCallable, Category = "Entertainer Combat") static void AttackStarted(AActor *Attacker);
    UFUNCTION(BlueprintCallable, Category = "Entertainer Combat") static void AttackResolved(AActor *Attacker);
    static void SetDamageSource(AActor *Enemy, AActor *Source, FName Category, bool Suppress = false);
};
UCLASS()
class DUNGEONGUIDANCE_API UAnimNotify_EntertainerContact : public UAnimNotify
{
    GENERATED_BODY()
  public:
    virtual void Notify(USkeletalMeshComponent *Mesh, UAnimSequenceBase *Animation,
                        const FAnimNotifyEventReference &Ref) override;
};
UCLASS()
class DUNGEONGUIDANCE_API UEntertainerComboWidget : public UUserWidget
{
    GENERATED_BODY()
  public:
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ComboText;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ComboState;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ComboAction;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UProgressBar> ComboBar;
    virtual void NativeTick(const FGeometry &Geometry, float Dt) override;
};
