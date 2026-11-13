#include "ProgressEnemyCharacter.h"
#include "ProgressEnemyPlayerBridge.h"
#include "ProgressEnemyReflection.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DamageEvents.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"

using namespace ProgressEnemyReflection;

AProgressEnemyCharacter::AProgressEnemyCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = .05f;
    AIControllerClass = AAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    bUseControllerRotationYaw = false;
    SetCanBeDamaged(true);
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0, 400, 0);
    GetCapsuleComponent()->InitCapsuleSize(42, 96);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    GetMesh()->SetRelativeLocationAndRotation(FVector(0, 0, -96), FRotator(0, -90, 0));
    FinisherPrompt = CreateDefaultSubobject<UWidgetComponent>(TEXT("FinisherPrompt"));
    FinisherPrompt->SetupAttachment(GetRootComponent());
    FinisherPrompt->SetRelativeLocation(FVector(0, 0, 125));
    FinisherPrompt->SetWidgetSpace(EWidgetSpace::Screen);
    FinisherPrompt->SetDrawSize(FVector2D(80, 80));
    FinisherPrompt->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FinisherPrompt->SetHiddenInGame(true);
    TaserBeam = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TaserBeam"));
    TaserBeam->SetupAttachment(GetRootComponent());
    TaserBeam->SetAbsolute(true, true, true);
    TaserBeam->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TaserBeam->SetCanEverAffectNavigation(false);
    TaserBeam->SetCastShadow(false);
    TaserBeam->SetHiddenInGame(true);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> BeamMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BeamMaterial(TEXT("/Game/LevelPrototyping/Materials/MI_Solid_Blue.MI_Solid_Blue"));
    if (BeamMesh.Succeeded()) TaserBeam->SetStaticMesh(BeamMesh.Object);
    if (BeamMaterial.Succeeded()) TaserBeam->SetMaterial(0, BeamMaterial.Object);
    Tags.Add(TEXT("Enemy"));
    Tags.Add(TEXT("ProgressEnemy"));
}

void AProgressEnemyCharacter::PostInitializeComponents()
{
    Super::PostInitializeComponents();
    if (!GetWorld() || !GetWorld()->IsGameWorld()) return;
    auto EnsureComponent = [this](const TCHAR* Path, FName Name) -> UActorComponent*
    {
        UClass* Class = LoadClass<UActorComponent>(nullptr, Path);
        if (!Class) return nullptr;
        if (auto* Existing = GetComponentByClass(Class)) return Existing;
        auto* Component = NewObject<UActorComponent>(this, Class, Name);
        AddInstanceComponent(Component);
        Component->RegisterComponent();
        return Component;
    };
    EnemyHealth = EnsureComponent(TEXT("/Game/Blueprints/CombatSystem/Health/AC_EnemyHealth.AC_EnemyHealth_C"), TEXT("AC_EnemyHealth"));
    EnemyCombat = EnsureComponent(TEXT("/Game/Blueprints/CombatSystem/Combat/AC_EnemyCombat.AC_EnemyCombat_C"), TEXT("AC_EnemyCombat"));
}

void AProgressEnemyCharacter::BeginPlay()
{
    Super::BeginPlay();
    PatrolOrigin = GetActorLocation();
    ApplyDifficulty(Difficulty, true);
    if (!GetController()) SpawnDefaultController();
    if (EnemyHealth && !bHealthInitialized) HandleHealthEvent(EnemyHealth, TEXT("ReceiveBeginPlay"), 0, 0);
    if (auto* Player = UGameplayStatics::GetPlayerPawn(this, 0)) UProgressEnemyPlayerBridge::FindOrAdd(Player);
    DecisionTime = FMath::FRandRange(DecisionInterval, DecisionInterval * 2);
}

int32 AProgressEnemyCharacter::GetMaximumBlockHits() const
{
    const int64 Hits = int64(FMath::Max(1, BaseBlockHits)) + int64(FMath::Max(0, Difficulty.Block)) * FMath::Max(0, BlockHitsPerLevel);
    return int32(FMath::Clamp<int64>(Hits, 1, 10000));
}

void AProgressEnemyCharacter::SetDifficultyLevels(int32 Damage, int32 Block, int32 Health, int32 MovementSpeed)
{
    FProgressEnemyDifficulty Levels;
    Levels.Damage = Damage; Levels.Block = Block; Levels.Health = Health; Levels.MovementSpeed = MovementSpeed;
    ApplyDifficulty(Levels);
}
void AProgressEnemyCharacter::ApplyDifficulty(FProgressEnemyDifficulty Levels, bool bRefillHealth)
{
    const float Fraction = MaximumHealth > 0 ? CurrentHealth / MaximumHealth : 1.f;
    const int32 OldMaximum = GetMaximumBlockHits();
    Difficulty = Levels;
    Difficulty.Damage = FMath::Clamp(Levels.Damage, 0, 10000);
    Difficulty.Block = FMath::Clamp(Levels.Block, 0, 10000);
    Difficulty.Health = FMath::Clamp(Levels.Health, 0, 10000);
    Difficulty.MovementSpeed = FMath::Clamp(Levels.MovementSpeed, 0, 10000);
    IntelligenceLevel = (Difficulty.Damage + Difficulty.Block + Difficulty.Health + Difficulty.MovementSpeed) * .25f;
    IntelligenceAlpha = FMath::Clamp(IntelligenceLevel / FMath::Max(.01f, IntelligenceLevelForMaximum), 0.f, 1.f);
    DecisionInterval = FMath::Lerp(.35f, .1f, IntelligenceAlpha);
    PathUpdateInterval = FMath::Lerp(.4f, .15f, IntelligenceAlpha);
    DecisionTime = FMath::Min(DecisionTime, DecisionInterval);
    RepathTime = FMath::Min(RepathTime, PathUpdateInterval);
    MaximumHealth = FMath::Max(1.f, BaseHealth + FMath::Max(0.f, HealthPerLevel) * Difficulty.Health);
    if (State != EProgressEnemyState::Dead)
        CurrentHealth = bRefillHealth ? MaximumHealth : FMath::Clamp(Fraction * MaximumHealth, 0.f, MaximumHealth);
    if (bRefillHealth) Resources.BlockHits = GetMaximumBlockHits();
    else if (Resources.BlockRecharge <= 0)
        Resources.BlockHits = FMath::Clamp(Resources.BlockHits + GetMaximumBlockHits() - OldMaximum, 0, GetMaximumBlockHits());
    UpdateHealthPresentation();
    SyncResources();
}
void AProgressEnemyCharacter::SyncResources()
{
    TaserShotsRemaining = Resources.TaserShots;
    BlockHitsRemaining = Resources.BlockHits;
    TaserRechargeRemaining = Resources.TaserRecharge;
    BlockRechargeRemaining = Resources.BlockRecharge;
}
void AProgressEnemyCharacter::SetState(EProgressEnemyState NewState)
{
    if (State == NewState) return;
    EProgressEnemyState Previous = State;
    if (Previous == EProgressEnemyState::Blocking) StopBlock();
    State = NewState;
    if (NewState == EProgressEnemyState::Chase) DecisionTime = DecisionInterval;
    if (NewState == EProgressEnemyState::Search) SearchTime = 0;
    bHasMoveGoal = false;
    RepathTime = 0;
    OnStateChanged.Broadcast(Previous, State);
}
bool AProgressEnemyCharacter::IsLivingPlayer(const AActor* Actor) const
{
    const APawn* Pawn = Cast<APawn>(Actor);
    return IsValid(Pawn) && Pawn->IsPlayerControlled() && Pawn->CanBeDamaged() && !Bool(const_cast<APawn*>(Pawn), TEXT("FinisherActive"));
}
bool AProgressEnemyCharacter::HasClearPathTo(const AActor* Actor) const
{
    if (!IsValid(Actor)) return false;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(ProgressEnemySight), false, this);
    FVector End = Actor->GetActorLocation() + FVector(0, 0, 45);
    return !GetWorld()->LineTraceSingleByChannel(Hit, GetPawnViewLocation(), End, ECC_Visibility, Params) || Hit.GetActor() == Actor;
}
bool AProgressEnemyCharacter::Sees(const AActor* Actor) const
{
    if (!IsLivingPlayer(Actor)) return false;
    FVector Offset = Actor->GetActorLocation() - GetActorLocation();
    if (Offset.SizeSquared() > FMath::Square(SightRadius)) return false;
    if (FVector::DotProduct(GetActorForwardVector(), Offset.GetSafeNormal2D()) < FMath::Cos(FMath::DegreesToRadians(SightHalfAngle))) return false;
    return HasClearPathTo(Actor);
}
void AProgressEnemyCharacter::NotifyPlayerDamage(AActor* Source)
{
    if (!IsLivingPlayer(Source) || State == EProgressEnemyState::Dead || State == EProgressEnemyState::Finishing) return;
    Target = Source;
    LastKnownPlayerLocation = Source->GetActorLocation();
    UnseenTime = 0;
    if (State != EProgressEnemyState::Attacking && State != EProgressEnemyState::Blocking) SetState(EProgressEnemyState::Chase);
}

float AProgressEnemyCharacter::TakeDamage(float DamageAmount, const FDamageEvent& Event, AController* EventInstigator, AActor* Causer)
{
    if (!HasAuthority() || !CanBeDamaged() || State == EProgressEnemyState::Dead || State == EProgressEnemyState::Finishing || !FMath::IsFinite(DamageAmount) || DamageAmount <= 0) return 0;
    AActor* Source = EventInstigator ? EventInstigator->GetPawn() : Causer;
    if (!IsLivingPlayer(Source) && Causer && Causer->GetInstigator()) Source = Causer->GetInstigator();
    NotifyPlayerDamage(Source);
    AActor* DirectionSource = IsValid(Causer) ? Causer : Source;
    bool Frontal = DirectionSource && FVector::DotProduct(GetActorForwardVector(), (DirectionSource->GetActorLocation() - GetActorLocation()).GetSafeNormal2D()) >= FMath::Cos(FMath::DegreesToRadians(BlockHalfAngle));
    const float Adjusted = Resources.Absorb(DamageAmount, State == EProgressEnemyState::Blocking && bEnableBlock && Frontal, BlockRechargeSeconds);
    // Broadcast Unreal point/any-damage events once. This enemy's health is routed below,
    // rather than binding the shared component's old point-damage handler a second time.
    const float Applied = Super::TakeDamage(Adjusted, Event, EventInstigator, Causer);
    ApplyHealthDamage(Applied);
    if (State != EProgressEnemyState::Dead && Adjusted == DamageAmount)
    {
        bImpactPending = false;
        if (State == EProgressEnemyState::Attacking)
        {
            ActionTime = 0;
            SetState(EProgressEnemyState::Chase);
        }
        if (HitReactionMontage) PlayAnimMontage(HitReactionMontage);
        DecisionTime = DecisionInterval * (4.f / 3.5f);
    }
    if (State == EProgressEnemyState::Blocking && Resources.BlockHits == 0) SetState(EProgressEnemyState::Chase);
    SyncResources();
    return Applied;
}

void AProgressEnemyCharacter::Tick(float Delta)
{
    Super::Tick(Delta);
    if (BeamTime > 0 && (BeamTime -= Delta) <= 0) TaserBeam->SetHiddenInGame(true);
    if (!HasAuthority() || State == EProgressEnemyState::Dead || State == EProgressEnemyState::Finishing) return;
    Resources.Tick(Delta, GetMaximumBlockHits());
    SyncResources();
    DecisionTime -= Delta;
    RepathTime -= Delta;
    APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
    if (IsLivingPlayer(Player)) UProgressEnemyPlayerBridge::FindOrAdd(Player);
    bVisible = Sees(Player);
    const bool Near = IsLivingPlayer(Player) && FVector::DistSquared(GetActorLocation(), Player->GetActorLocation()) <= FMath::Square(ProximityRadius) && HasClearPathTo(Player);
    ProximityTime = Near ? ProximityTime + Delta : 0;
    if (bVisible || ProximityTime >= ProximityNoticeSeconds)
    {
        Target = Player;
        LastKnownPlayerLocation = Player->GetActorLocation();
        UnseenTime = 0;
        if (State == EProgressEnemyState::Patrol || State == EProgressEnemyState::Search) SetState(EProgressEnemyState::Chase);
    }
    else UnseenTime += Delta;
    if (Target && !IsLivingPlayer(Target))
    {
        Target = nullptr;
        bImpactPending = false;
        ActionTime = 0;
        if (State != EProgressEnemyState::Patrol) SetState(EProgressEnemyState::Search);
    }
    if (State == EProgressEnemyState::Attacking)
    {
        ActionTime -= Delta;
        if (bImpactPending && (ImpactTime -= Delta) <= 0) ResolveAttack();
        if (ActionTime <= 0) SetState(EProgressEnemyState::Chase);
        return;
    }
    if (State == EProgressEnemyState::Blocking)
    {
        StopMoving();
        if (IsValid(Target)) SetActorRotation(FRotator(0, (Target->GetActorLocation() - GetActorLocation()).Rotation().Yaw, 0));
        ActionTime -= Delta;
        if (!bEnableBlock || Resources.BlockHits <= 0 || ActionTime <= 0) { SetState(EProgressEnemyState::Chase); DecisionTime = DecisionInterval * (5.f / 3.5f); }
        return;
    }
    Think(Delta);
    if (bDrawDebug)
    {
        DrawDebugSphere(GetWorld(), LastKnownPlayerLocation, 25, 8, FColor::Yellow, false, .06f);
        DrawDebugString(GetWorld(), GetActorLocation() + FVector(0,0,130), UEnum::GetValueAsString(State), nullptr, FColor::White, .06f);
    }
}
void AProgressEnemyCharacter::Think(float Delta)
{
    const float SpeedScale = 1 + FMath::Max(0.f, SpeedIncreasePerLevel) * Difficulty.MovementSpeed;
    GetCharacterMovement()->MaxWalkSpeed = FMath::Clamp((State == EProgressEnemyState::Chase ? ChaseSpeed : PatrolSpeed) * SpeedScale, 1.f, FMath::Max(1.f, MaximumSpeed));
    if (State == EProgressEnemyState::Chase)
    {
        if (IsValid(Target) && (bVisible || ProximityTime >= ProximityNoticeSeconds))
        {
            const float Distance = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
            if (DecisionTime <= 0)
            {
                if (Distance <= MeleeRange)
                {
                    if (bEnableBlock && FMath::FRand() < GetBlockDecisionChance() && TryBlock()) return;
                    EProgressEnemyAttack Attack = bEnableHeavyMelee && (!bEnableLightMelee || FMath::FRand() < GetHeavyAttackChance()) ? EProgressEnemyAttack::Heavy : EProgressEnemyAttack::Light;
                    if (TryAttack(Attack)) return;
                }
                if ((Distance >= TaserMinimumRange || (!bEnableLightMelee && !bEnableHeavyMelee)) && TryAttack(EProgressEnemyAttack::Taser)) return;
                DecisionTime = DecisionInterval;
            }
            // Only extrapolate observed movement. Lost targets still use the last seen point.
            const FVector Lead = bVisible ? Target->GetVelocity().GetClampedToMaxSize(450) * (.45f * IntelligenceAlpha) : FVector::ZeroVector;
            MoveTo(LastKnownPlayerLocation + Lead, FMath::Max(30.f, MeleeRange * .65f));
        }
        else
        {
            // Never follow the live player transform after losing awareness.
            MoveTo(LastKnownPlayerLocation, 70);
            if (UnseenTime >= SightMemorySeconds && FVector::Dist2D(GetActorLocation(), LastKnownPlayerLocation) < 150) SetState(EProgressEnemyState::Search);
            else if (UnseenTime > SightMemorySeconds + 8) SetState(EProgressEnemyState::Search);
        }
        return;
    }
    if (State == EProgressEnemyState::Search)
    {
        SearchTime += Delta;
        if (SearchTime >= SearchDuration) { Target = nullptr; SetState(EProgressEnemyState::Patrol); return; }
    }
    if (bHasMoveGoal)
    {
        auto* AI = Cast<AAIController>(GetController());
        if (FVector::Dist2D(GetActorLocation(), MoveGoal) < 100 || (AI && AI->GetMoveStatus() == EPathFollowingStatus::Idle))
        {
            bHasMoveGoal = false;
            PauseTime = PatrolPause;
        }
    }
    if (!bHasMoveGoal)
    {
        PauseTime -= Delta;
        if (PauseTime <= 0)
        {
            if (RandomGoal(State == EProgressEnemyState::Search ? LastKnownPlayerLocation : PatrolOrigin, State == EProgressEnemyState::Search ? SearchRadius : PatrolRadius))
                MoveTo(MoveGoal, 60);
            else PauseTime = 1; // Navigation may still be rebuilding.
        }
    }
}
float AProgressEnemyCharacter::GetHeavyAttackChance() const
{
    if (!IsValid(Target)) return FMath::Clamp(HeavyChance, 0.f, 1.f);
    const bool GoodOpening = Target->GetVelocity().Size2D() <= 75 && FVector::Dist(GetActorLocation(), Target->GetActorLocation()) <= MeleeRange * .8f;
    return FMath::Lerp(FMath::Clamp(HeavyChance, 0.f, 1.f), GoodOpening ? .8f : 0.f, IntelligenceAlpha);
}
float AProgressEnemyCharacter::GetBlockDecisionChance() const
{
    const float Wounded = 1 - FMath::Clamp(CurrentHealth / FMath::Max(1.f, MaximumHealth), 0.f, 1.f);
    return FMath::Clamp(BlockChance + IntelligenceAlpha * Wounded * .4f, 0.f, .85f);
}
void AProgressEnemyCharacter::MoveTo(const FVector& Goal, float Acceptance)
{
    if (RepathTime > 0) return;
    RepathTime = PathUpdateInterval;
    if (auto* AI = Cast<AAIController>(GetController()))
    {
        const auto Result = AI->MoveToLocation(Goal, Acceptance, true, true, false, true, nullptr, false);
        MoveGoal = Goal;
        bHasMoveGoal = Result != EPathFollowingRequestResult::Failed;
    }
}
bool AProgressEnemyCharacter::RandomGoal(const FVector& Origin, float Radius)
{
    auto* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    FNavLocation Point;
    if (!Nav || !Nav->GetRandomReachablePointInRadius(Origin, Radius, Point)) return false;
    MoveGoal = Point.Location;
    return true;
}
void AProgressEnemyCharacter::StopMoving()
{
    if (auto* AI = Cast<AAIController>(GetController())) AI->StopMovement();
    GetCharacterMovement()->StopMovementImmediately();
    bHasMoveGoal = false;
}
bool AProgressEnemyCharacter::IsAttackEnabled(EProgressEnemyAttack Attack) const
{
    return Attack == EProgressEnemyAttack::Light ? bEnableLightMelee : Attack == EProgressEnemyAttack::Heavy ? bEnableHeavyMelee : bEnableTaser;
}
void AProgressEnemyCharacter::SetAttackEnabled(EProgressEnemyAttack Attack, bool bEnabled)
{
    if (Attack == EProgressEnemyAttack::Light) bEnableLightMelee = bEnabled;
    else if (Attack == EProgressEnemyAttack::Heavy) bEnableHeavyMelee = bEnabled;
    else bEnableTaser = bEnabled;
    if (!bEnabled && State == EProgressEnemyState::Attacking && ActiveAttack == Attack)
    {
        bImpactPending = false;
        StopAnimMontage();
        SetState(EProgressEnemyState::Chase);
    }
}
bool AProgressEnemyCharacter::TryAttack(EProgressEnemyAttack Attack)
{
    if (!HasAuthority() || State != EProgressEnemyState::Chase || !IsAttackEnabled(Attack) || !IsLivingPlayer(Target) || !HasClearPathTo(Target)) return false;
    const float Distance = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
    const bool Taser = Attack == EProgressEnemyAttack::Taser;
    if (Taser ? (Distance > TaserRange || (Distance < TaserMinimumRange && (bEnableLightMelee || bEnableHeavyMelee)) || Resources.TaserShots <= 0 || Resources.TaserRecharge > 0) : Distance > MeleeRange) return false;
    StopMoving();
    SetActorRotation(FRotator(0, (Target->GetActorLocation() - GetActorLocation()).Rotation().Yaw, 0));
    ActiveAttack = Attack;
    const float Rate = FMath::Max(.01f, Attack == EProgressEnemyAttack::Heavy ? HeavyAnimationSpeed : LightAnimationSpeed);
    ImpactTime = Taser ? TaserWindup : MeleeWindup / Rate;
    ActionTime = ImpactTime + (Taser ? TaserShotInterval : MeleeRecovery / Rate);
    bImpactPending = true;
    SetState(EProgressEnemyState::Attacking);
    UAnimMontage* Montage = Taser ? TaserMontage.Get() : MeleeMontages.Num() ? MeleeMontages[FMath::RandRange(0, MeleeMontages.Num()-1)].Get() : nullptr;
    if (Montage) PlayAnimMontage(Montage, Taser ? 1.f : Rate);
    else if (Taser && BlockAnimation && GetMesh()->GetAnimInstance())
        GetMesh()->GetAnimInstance()->PlaySlotAnimationAsDynamicMontage(BlockAnimation, TEXT("DefaultSlot"), .1f, .1f);
    OnAttackStarted.Broadcast(Attack, ImpactTime);
    return true;
}
void AProgressEnemyCharacter::ResolveAttack()
{
    bImpactPending = false;
    if (!IsAttackEnabled(ActiveAttack) || !IsLivingPlayer(Target)) return;
    const bool Taser = ActiveAttack == EProgressEnemyAttack::Taser;
    const float Range = Taser ? TaserRange : MeleeRange;
    if (Taser && !Resources.SpendTaser(TaserCooldown)) return;
    // A fired shot is spent even when the player dodges behind cover during windup.
    FVector Start = GetPawnViewLocation();
    FVector End = Target->GetActorLocation() + FVector(0, 0, 45);
    if (Taser)
    {
        FHitResult VisualHit;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(ProgressEnemyTaser), false, this);
        if (GetWorld()->LineTraceSingleByChannel(VisualHit, Start, End, ECC_Visibility, Params)) End = VisualHit.ImpactPoint;
        OnTaserFired.Broadcast(Start, End);
        TaserBeam->SetWorldLocation((Start + End) * .5f);
        TaserBeam->SetWorldRotation(FRotationMatrix::MakeFromZ(End - Start).Rotator());
        TaserBeam->SetWorldScale3D(FVector(.025f, .025f, FVector::Dist(Start, End) / 100.f));
        TaserBeam->SetHiddenInGame(false);
        BeamTime = .15f;
        if (bDrawDebug) DrawDebugLine(GetWorld(), Start, End, FColor::Cyan, false, .3f, 0, 2);
    }
    if (FVector::Dist(GetActorLocation(), Target->GetActorLocation()) > Range || !HasClearPathTo(Target)) { SyncResources(); return; }
    if (!Taser && FVector::DotProduct(GetActorForwardVector(), (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D()) < .25f) return;
    // The shared combat component applies point damage, camera shake and hit stop.
    // Using its public ApplyDamageToPlayer function also respects player blocking.
    FHitResult Hit(Target, Cast<UPrimitiveComponent>(Target->GetRootComponent()), Target->GetActorLocation(), (GetActorLocation()-Target->GetActorLocation()).GetSafeNormal());
    const float Base = Taser ? TaserDamage : ActiveAttack == EProgressEnemyAttack::Heavy ? HeavyDamage : LightDamage;
    const float Amount = FMath::Max(0.f, Base) * (1 + FMath::Max(0.f, DamageIncreasePerLevel) * Difficulty.Damage);
    if (auto* Hits = CastField<FArrayProperty>(Property(EnemyCombat, TEXT("HitActors"))))
        FScriptArrayHelper(Hits, Hits->ContainerPtrToValuePtr<void>(EnemyCombat)).EmptyValues();
    bool AppliedViaComponent = Call(EnemyCombat, TEXT("ApplyDamageToPlayer"), [&](UFunction* F, uint8* Params)
    {
        ParamNumber(F, Params, TEXT("Amount"), Amount);
        if (auto* P = FindFProperty<FBoolProperty>(F, TEXT("Hit"))) P->SetPropertyValue_InContainer(Params, true);
        if (auto* P = FindFProperty<FArrayProperty>(F, TEXT("Hits")))
        {
            FScriptArrayHelper Array(P, P->ContainerPtrToValuePtr<void>(Params));
            const int32 Index = Array.AddValue();
            if (auto* Inner = CastField<FStructProperty>(P->Inner)) Inner->CopyCompleteValue(Array.GetRawPtr(Index), &Hit);
        }
    });
    if (!AppliedViaComponent) UGameplayStatics::ApplyPointDamage(Target, Amount, (Target->GetActorLocation()-Start).GetSafeNormal(), Hit, GetController(), this, UDamageType::StaticClass());
    if (Taser && !Bool(Target, TEXT("Blocking")))
        if (auto* Bridge = UProgressEnemyPlayerBridge::FindOrAdd(Target)) Bridge->Stun(TaserStunSeconds);
    SyncResources();
}
bool AProgressEnemyCharacter::TryBlock()
{
    if (!HasAuthority() || State != EProgressEnemyState::Chase || !bEnableBlock || Resources.BlockHits <= 0 || Resources.BlockRecharge > 0) return false;
    StopMoving();
    SetState(EProgressEnemyState::Blocking);
    ActionTime = FMath::Max(.01f, BlockDuration);
    PlayBlock();
    return true;
}
void AProgressEnemyCharacter::PlayBlock()
{
    if (BlockAnimation && GetMesh()->GetAnimInstance())
        GetMesh()->GetAnimInstance()->PlaySlotAnimationAsDynamicMontage(BlockAnimation, TEXT("DefaultSlot"), .1f, .15f, 1.f, 100);
}
void AProgressEnemyCharacter::StopBlock()
{
    if (GetMesh()->GetAnimInstance()) GetMesh()->GetAnimInstance()->Montage_Stop(.1f);
}

bool AProgressEnemyCharacter::HandleHealthEvent(UActorComponent* Component, FName Event, double Damage, double Delay)
{
    EnemyHealth = Component;
    if (Event == TEXT("ReceiveBeginPlay"))
    {
        if (bHealthInitialized) return true;
        bHealthInitialized = true;
        SetObject(Component, TEXT("CharacterActor"), this);
        SetObject(Component, TEXT("Widget"), FinisherPrompt);
        ApplyDifficulty(Difficulty, true);
        auto* PC = UGameplayStatics::GetPlayerController(this, 0);
        if (PC && PC->GetLocalPlayer()) Call(Component, TEXT("SpawnHealthBar"));
        UpdateHealthPresentation();
    }
    else if (Event == TEXT("ProcessDamage")) ApplyHealthDamage(float(Damage));
    else if (Event == TEXT("Finisher")) BeginFinisher(Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0)));
    else if (Event == TEXT("FinisherVis")) UpdateHealthPresentation();
    else if (Event == TEXT("FinisherVisOff")) { SetBool(Component, TEXT("CanFinish"), false); FinisherPrompt->SetHiddenInGame(true); }
    else if (Event == TEXT("MoveEnemy")) { /* Native finisher handles safe alignment. */ }
    else return false;
    return true;
}
void AProgressEnemyCharacter::ApplyHealthDamage(float Amount)
{
    if (State == EProgressEnemyState::Dead || State == EProgressEnemyState::Finishing || !FMath::IsFinite(Amount) || Amount <= 0) return;
    CurrentHealth = FMath::Clamp(CurrentHealth - Amount, 0.f, MaximumHealth);
    UpdateHealthPresentation();
    if (CurrentHealth <= 0) Die();
}
bool AProgressEnemyCharacter::CanBeFinished() const
{
    return State != EProgressEnemyState::Dead && State != EProgressEnemyState::Finishing && CurrentHealth > 0 && CurrentHealth <= MaximumHealth * Number(EnemyHealth, TEXT("FinisherThreshhold"), .3);
}
void AProgressEnemyCharacter::UpdateHealthPresentation()
{
    SetNumber(EnemyHealth, TEXT("MaxHealth"), MaximumHealth);
    SetNumber(EnemyHealth, TEXT("CurrentHealth"), CurrentHealth);
    SetBool(EnemyHealth, TEXT("CanFinish"), CanBeFinished());
    UObject* Bar = Object(EnemyHealth, TEXT("HealthBar"));
    Call(Bar, TEXT("SetFillLevel"), [&](UFunction* F, uint8* Params) { ParamNumber(F, Params, TEXT("Level"), CurrentHealth / MaximumHealth); });
    if (FinisherPrompt)
    {
        if (CanBeFinished() && !FinisherPrompt->GetWidget())
        {
            UClass* WidgetClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/Blueprints/CombatSystem/Combat/Finisher/WBP_Finish.WBP_Finish_C"));
            auto* PC = UGameplayStatics::GetPlayerController(this, 0);
            if (WidgetClass && PC && PC->GetLocalPlayer())
            {
                UUserWidget* Widget = CreateWidget<UUserWidget>(GetWorld(), WidgetClass);
                FinisherPrompt->SetWidget(Widget);
                SetObject(EnemyHealth, TEXT("FinishWidget"), Widget);
            }
        }
        FinisherPrompt->SetHiddenInGame(!CanBeFinished());
    }
}
void AProgressEnemyCharacter::BeginFinisher(ACharacter* Player)
{
    if (!Player || !CanBeFinished() || !IsLivingPlayer(Player) || FVector::Dist(Player->GetActorLocation(), GetActorLocation()) > 350 || !HasClearPathTo(Player)) return;
    StopMoving();
    bImpactPending = false;
    SetState(EProgressEnemyState::Finishing);
    SetCanBeDamaged(false);
    FinishingPlayer = Player;
    PlayerPreviousMovement = Player->GetCharacterMovement()->MovementMode;
    PlayerPreviousCustomMovement = Player->GetCharacterMovement()->CustomMovementMode;
    Player->GetCharacterMovement()->DisableMovement();
    GetCharacterMovement()->DisableMovement();
    SetBool(Player, TEXT("FinisherActive"), true);
    SetBool(EnemyHealth, TEXT("FinisherActive"), true);
    SetBool(EnemyHealth, TEXT("CanFinish"), false);
    FinisherPrompt->SetHiddenInGame(true);
    Player->SetActorRotation(FRotator(0, (GetActorLocation() - Player->GetActorLocation()).Rotation().Yaw, 0));
    SetActorRotation(FRotator(0, (Player->GetActorLocation() - GetActorLocation()).Rotation().Yaw, 0));
    float Duration = 2.5f;
    if (PlayerFinisherMontage) Duration = FMath::Max(.1f, Player->PlayAnimMontage(PlayerFinisherMontage));
    if (EnemyFinisherMontage) PlayAnimMontage(EnemyFinisherMontage);
    GetWorldTimerManager().SetTimer(FinishTimer, this, &AProgressEnemyCharacter::FinishFinisher, Duration, false);
}
void AProgressEnemyCharacter::ReleasePlayer()
{
    if (FinishingPlayer.IsValid())
    {
        auto* Player = FinishingPlayer.Get();
        SetBool(Player, TEXT("FinisherActive"), false);
        if (Player->CanBeDamaged() && Player->GetCharacterMovement()->MovementMode == MOVE_None)
            Player->GetCharacterMovement()->SetMovementMode(PlayerPreviousMovement, PlayerPreviousCustomMovement);
    }
    FinishingPlayer.Reset();
}
void AProgressEnemyCharacter::FinishFinisher()
{
    ReleasePlayer();
    CurrentHealth = 0;
    SetBool(EnemyHealth, TEXT("FinisherActive"), false);
    Die();
}
void AProgressEnemyCharacter::Die()
{
    if (State == EProgressEnemyState::Dead) return;
    StopMoving();
    bImpactPending = false;
    SetState(EProgressEnemyState::Dead);
    SetCanBeDamaged(false);
    GetCharacterMovement()->DisableMovement();
    StopAnimMontage();
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
    GetMesh()->SetSimulatePhysics(true);
    FinisherPrompt->SetHiddenInGame(true);
    TArray<UWidgetComponent*> Widgets;
    GetComponents(Widgets);
    for (auto* Widget : Widgets) Widget->SetHiddenInGame(true);
    UpdateHealthPresentation();
    SetActorTickEnabled(false);
    TaserBeam->SetHiddenInGame(true);
    OnDied.Broadcast();
    // Preserve the existing kill/sanity interaction, without assuming the manager exists.
    UClass* SanityClass = LoadClass<AActor>(nullptr, TEXT("/Game/ShaderMats/Sanity.Sanity_C"));
    if (SanityClass) Call(UGameplayStatics::GetActorOfClass(this, SanityClass), TEXT("SanityActivation"));
    SetLifeSpan(FMath::Max(.1f, CorpseLifetime));
}
void AProgressEnemyCharacter::EndPlay(const EEndPlayReason::Type Reason)
{
    GetWorldTimerManager().ClearTimer(FinishTimer);
    ReleasePlayer();
    Super::EndPlay(Reason);
}
