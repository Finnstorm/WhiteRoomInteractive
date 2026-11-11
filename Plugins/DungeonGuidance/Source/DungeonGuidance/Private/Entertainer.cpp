#include "Entertainer.h"
#include "BountyHunter.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "EnhancedInputComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "LatentActions.h"
#include "Misc/ScopeExit.h"
#include "UObject/UnrealType.h"
#include "UObject/StructOnScope.h"
namespace EntertainerImpl
{
UObject *Object(UObject *O, FName N)
{
    if (O)
        if (auto *P = FindFProperty<FObjectPropertyBase>(O->GetClass(), N))
            return P->GetObjectPropertyValue_InContainer(O);
    return nullptr;
}
double Number(UObject *O, FName N, double Fallback = 0)
{
    if (O)
        if (auto *P = FindFProperty<FNumericProperty>(O->GetClass(), N))
        {
            auto *V = P->ContainerPtrToValuePtr<void>(O);
            return P->IsFloatingPoint() ? P->GetFloatingPointPropertyValue(V) : P->GetSignedIntPropertyValue(V);
        }
    return Fallback;
}
bool Flag(UObject *O, FName N)
{
    if (O)
        if (auto *P = FindFProperty<FBoolProperty>(O->GetClass(), N))
            return P->GetPropertyValue_InContainer(O);
    return false;
}
void Flag(UObject *O, FName N, bool V)
{
    if (O)
        if (auto *P = FindFProperty<FBoolProperty>(O->GetClass(), N))
            P->SetPropertyValue_InContainer(O, V);
}
void Arg(UFunction *F, void *Data, FName N, UObject *V)
{
    if (auto *P = FindFProperty<FObjectPropertyBase>(F, N))
        P->SetObjectPropertyValue_InContainer(Data, V);
}
UActorComponent *Health(AActor *A)
{
    if (!IsValid(A) || A->IsActorBeingDestroyed())
        return nullptr;
    for (auto *C : TInlineComponentArray<UActorComponent *>(A))
        for (auto *K = C->GetClass(); K; K = K->GetSuperClass())
            if (K->GetName().Contains(TEXT("EnemyHealth")))
                return C;
    return nullptr;
}
UEntertainerCombatTracker *Tracker(AActor *A)
{
    if (!A)
        return nullptr;
    auto *T = A->FindComponentByClass<UEntertainerCombatTracker>();
    if (!T)
    {
        T = NewObject<UEntertainerCombatTracker>(A);
        A->AddInstanceComponent(T);
        T->RegisterComponent();
    }
    return T;
}
UEntertainerComponent *Player(UObject *Context)
{
    auto *A = UGameplayStatics::GetPlayerPawn(Context, 0);
    return A ? A->FindComponentByClass<UEntertainerComponent>() : nullptr;
}
UAnimInstance *Anim(AActor *A)
{
    auto *C = Cast<ACharacter>(A);
    return C && C->GetMesh() ? C->GetMesh()->GetAnimInstance() : nullptr;
}
UEntertainerCaptureBridge *FreeRig(UWorld *W)
{
    for (TActorIterator<AActor> I(W); I; ++I)
        if (auto *B = I->FindComponentByClass<UEntertainerCaptureBridge>())
            if (!Flag(*I, TEXT("InUse")))
                return B;
    return nullptr;
}
struct FAttackWatch
{
    TWeakObjectPtr<AActor> Player;
    FVector Start;
    double Time = 0;
    float HP = 0;
    bool Dashed = false;
};
// World/actor keys are weak; entries are removed at resolution and pruned at the next attack.
TMap<TWeakObjectPtr<AActor>, FAttackWatch> Attacks;
} // namespace EntertainerImpl
using namespace EntertainerImpl;
UEntertainerComponent::UEntertainerComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}
FEntertainerStats UEntertainerComponent::CalculateStats(const UEntertainerSettings *S, const UDataTable *T,
                                                        const TArray<FName> &IDs, bool Equipped)
{
    FEntertainerStats R;
    if (!S)
        return R;
    R.Equipped = Equipped;
    R.Cost = S->Cost;
    R.Cooldown = S->Cooldown;
    R.Windup = S->Windup;
    R.TargetRange = S->TargetRange;
    R.FrightRadius = S->FrightRadius;
    R.FrightDuration = S->FrightDuration;
    R.SlowFraction = S->SlowFraction;
    R.SecondaryRadius = S->SecondaryRadius;
    FEntertainerUpgrade Sum;
    if (T && T->GetRowStruct() == FEntertainerUpgrade::StaticStruct())
        for (FName ID : TSet<FName>(IDs))
        {
            if (auto *U = T->FindRow<FEntertainerUpgrade>(ID, TEXT("Entertainer"), false))
            {
                Sum.Unlock |= U->Unlock;
                Sum.Refund += U->Refund;
                Sum.CostReduction += U->CostReduction;
                Sum.Fright |= U->Fright;
                Sum.FrightDurationBonus += U->FrightDurationBonus;
                Sum.FrightRadiusBonus += U->FrightRadiusBonus;
                Sum.SlowBonus += U->SlowBonus;
                Sum.TargetRangeBonus += U->TargetRangeBonus;
                Sum.WindupReduction += U->WindupReduction;
                Sum.CooldownReduction += U->CooldownReduction;
                Sum.ExtraTargets += U->ExtraTargets;
                Sum.SecondaryRadiusBonus += U->SecondaryRadiusBonus;
            }
        }
    R.Unlocked = Sum.Unlock;
    R.Refund = Sum.Refund;
    R.Cost = FMath::Clamp(S->Cost - Sum.CostReduction, 0.f, FMath::Max(0.f, S->Capacity));
    R.Cooldown = FMath::Max(0.f, S->Cooldown * (1 - Sum.CooldownReduction));
    R.Windup = FMath::Max(.05f, S->Windup * (1 - Sum.WindupReduction));
    R.TargetRange = FMath::Max(1.f, S->TargetRange * (1 + Sum.TargetRangeBonus));
    R.Fright = Sum.Fright;
    R.FrightDuration = FMath::Max(0.f, S->FrightDuration * (1 + Sum.FrightDurationBonus));
    R.FrightRadius = FMath::Max(0.f, S->FrightRadius * (1 + Sum.FrightRadiusBonus));
    R.SlowFraction = FMath::Clamp(S->SlowFraction + Sum.SlowBonus, 0.f, 1.f);
    R.Targets = FMath::Clamp(1 + Sum.ExtraTargets, 1, 16);
    R.SecondaryRadius = FMath::Max(0.f, S->SecondaryRadius * (1 + Sum.SecondaryRadiusBonus));
    return R;
}
bool UEntertainerComponent::Alive() const
{
    return IsValid(GetOwner()) && !GetOwner()->IsActorBeingDestroyed() &&
           Number(Object(GetOwner(), TEXT("AC_PlayerHealth")), TEXT("CurrentHealth")) > 0;
}
void UEntertainerComponent::RefreshUpgrades()
{
    auto *Save = Object(UGameplayStatics::GetGameInstance(this), TEXT("SaveGameRef"));
    if (!Save || !Settings)
        return;
    TArray<FName> IDs;
    if (auto *P = FindFProperty<FArrayProperty>(Save->GetClass(), TEXT("UnlockedSkillNodeIDs")))
    {
        FScriptArrayHelper A(P, P->ContainerPtrToValuePtr<void>(Save));
        if (auto *Inner = CastField<FNameProperty>(P->Inner))
            for (int I = 0; I < A.Num(); ++I)
                IDs.Add(Inner->GetPropertyValue(A.GetRawPtr(I)));
    }
    bool Equipped = false;
    if (auto *P = FindFProperty<FNameProperty>(Save->GetClass(), TEXT("EquippedSkillTree")))
        Equipped = P->GetPropertyValue_InContainer(Save) == TEXT("Entertainer");
    Stats = CalculateStats(Settings, Upgrades, IDs, Equipped);
    if (Pending && (!Stats.Unlocked || !Stats.Equipped))
        Cancel();
    if (!Stats.Unlocked)
    {
        Combo = 0;
        ActionTimes.Reset();
        LastCategory = NAME_None;
    }
}
void UEntertainerComponent::Score(FName Action, FName Category, float Amount)
{
    if (!Settings || !Alive())
        return;
    RefreshUpgrades();
    if (!Stats.Unlocked || !Stats.Equipped || Amount <= 0)
        return;
    const double *Previous = ActionTimes.Find(Action);
    bool Repeat = Previous && Age - *Previous < Settings->RepeatWindow;
    float Award = Amount * (Repeat ? Settings->RepeatMultiplier : 1.f);
    if (!Repeat && !LastCategory.IsNone() && LastCategory != Category)
        Award += Settings->VarietyScore;
    ActionTimes.Add(Action, Age);
    LastCategory = Category;
    LastScore = Age;
    Combo = FMath::Clamp(Combo + Award, 0.f, Settings->Capacity);
    LastAction = FString::Printf(TEXT("%s  +%.0f%s"), *Action.ToString(), Award, Repeat ? TEXT(" (repeat)") : TEXT(""));
}
void UEntertainerComponent::TookDamage()
{
    if (Settings)
    {
        Combo = FMath::Max(0.f, Combo - Settings->DamagePenalty);
        LastAction = TEXT("Damage taken");
    }
}
FString UEntertainerComponent::Rank() const
{
    FString Result = TEXT("D");
    if (Settings)
        for (int I = 0; I < FMath::Min(Settings->RankNames.Num(), Settings->RankThresholds.Num()); ++I)
            if (Combo + KINDA_SMALL_NUMBER >= Settings->RankThresholds[I])
                Result = Settings->RankNames[I];
    return Result;
}
bool UEntertainerComponent::ValidTarget(AActor *Target) const
{
    return Target != GetOwner() && Health(Target) && Number(Health(Target), TEXT("CurrentHealth")) > 0 &&
           !Flag(Health(Target), TEXT("FinisherActive"));
}
AActor *UEntertainerComponent::SelectTarget() const
{
    auto *Pawn = Cast<APawn>(GetOwner());
    auto *PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
    if (!PC)
        return nullptr;
    FVector Eye;
    FRotator Facing;
    PC->GetPlayerViewPoint(Eye, Facing);
    FHitResult H;
    FCollisionQueryParams Q(SCENE_QUERY_STAT(EntertainerTarget), false, GetOwner());
    GetWorld()->LineTraceSingleByChannel(H, Eye, Eye + Facing.Vector() * Stats.TargetRange, ECC_Visibility, Q);
    return ValidTarget(H.GetActor()) ? H.GetActor() : nullptr;
}
bool UEntertainerComponent::TryActivate()
{
    RefreshUpgrades();
    auto *Animation = Anim(GetOwner());
    if (!Settings || !Alive() || !Stats.Unlocked || !Stats.Equipped || Pending || CooldownRemaining > 0 ||
        Combo + KINDA_SMALL_NUMBER < Stats.Cost || !Animation || !SnapMontage ||
        Flag(Object(GetOwner(), TEXT("AC_CombatSystem")), TEXT("BlockActive")))
        return false;
    auto *Target = SelectTarget();
    auto *Bridge = FreeRig(GetWorld());
    if (!Target || !Bridge || !Bridge->ValidateProjection(Target))
        return false;
    CastStats = Stats;
    ++CastToken;
    Reserved = CastStats.Cost;
    Started = Age;
    PrimaryCommitted = false;
    ContactIssued = false;
    PrimaryTarget = Target;
    Pending = true;
    if (!Bridge->Reserve(this, Target, CastToken, true))
    {
        Cancel();
        return false;
    }
    Captures.Add(Bridge);
    if (Animation->Montage_Play(SnapMontage, Settings->AnimationContactTime / CastStats.Windup) <= 0)
    {
        Cancel();
        return false;
    }
    LastAction = TEXT("Snap...");
    return true;
}
void UEntertainerComponent::Contact()
{
    if (!Pending || ContactIssued)
        return;
    RefreshUpgrades();
    if (!Pending || !Alive() || Combo + KINDA_SMALL_NUMBER < Reserved || SelectTarget() != PrimaryTarget.Get())
    {
        Cancel();
        return;
    }
    ContactIssued = true;
    if (Captures.Num() == 0 || !Captures[0].IsValid() || !Captures[0]->ValidateProjection(PrimaryTarget.Get()))
    {
        Cancel();
        return;
    }
    Captures[0]->Start();
}
bool UEntertainerComponent::CompleteCapture(UEntertainerCaptureBridge *Bridge, AActor *Target, AActor *Projection,
                                            int32 Token, bool Primary)
{
    RefreshUpgrades();
    if (!Captures.ContainsByPredicate(
            [Bridge](const TWeakObjectPtr<UEntertainerCaptureBridge> &B) { return B.Get() == Bridge; }) ||
        !Pending || Token != CastToken || !ContactIssued || !Alive() || !Stats.Equipped || !Stats.Unlocked ||
        !ValidTarget(Target) || !IsValid(Projection) || !Object(Projection, TEXT("CapturedPose")))
        return false;
    if (Primary)
    {
        if (Target != PrimaryTarget.Get() || PrimaryCommitted || Combo + KINDA_SMALL_NUMBER < Reserved)
            return false;
        PrimaryCommitted = true;
        Combo = FMath::Clamp(Combo - Reserved + CastStats.Refund, 0.f, Settings->Capacity);
        Reserved = 0;
        CooldownRemaining = CastStats.Cooldown;
        LastAction = TEXT("Encore!");
        if (CastStats.Fright)
            Frighten(Projection->GetActorLocation());
        // Queue extras only after primary success. A busy pool or invalid extra never cancels the primary.
        TArray<AActor *> Candidates;
        for (TActorIterator<AActor> I(GetWorld()); I; ++I)
            if (*I != Target && ValidTarget(*I) &&
                FVector::DistSquared(I->GetActorLocation(), Target->GetActorLocation()) <=
                    FMath::Square(CastStats.SecondaryRadius))
                Candidates.Add(*I);
        Candidates.Sort([&](const AActor &A, const AActor &B) {
            return FVector::DistSquared(A.GetActorLocation(), Target->GetActorLocation()) <
                   FVector::DistSquared(B.GetActorLocation(), Target->GetActorLocation());
        });
        int Extra = 0;
        for (auto *A : Candidates)
        {
            if (Extra >= CastStats.Targets - 1)
                break;
            auto *B = FreeRig(GetWorld());
            if (!B)
                break;
            if (B->ValidateProjection(A) && B->Reserve(this, A, CastToken, false))
            {
                Captures.Add(B);
                ++Extra;
                B->Start();
            }
        }
    }
    else if (!PrimaryCommitted)
        return false;
    UEntertainerCombatEvents::SetDamageSource(Target, GetOwner(), TEXT("Ability"), true);
    // Run the established health/death bookkeeping once; the capture graph then removes the actor.
    auto *H = Health(Target);
    if (auto *F = H ? H->FindFunction(TEXT("ProcessDamage")) : nullptr)
    {
        FStructOnScope Args(F);
        if (auto *P = FindFProperty<FNumericProperty>(F, TEXT("DamageAmount")))
            P->SetFloatingPointPropertyValue(P->ContainerPtrToValuePtr<void>(Args.GetStructMemory()),
                                             Number(H, TEXT("CurrentHealth")));
        if (auto *P = FindFProperty<FNumericProperty>(F, TEXT("DeathDelay")))
            P->SetFloatingPointPropertyValue(P->ContainerPtrToValuePtr<void>(Args.GetStructMemory()), 2);
        H->ProcessEvent(F, Args.GetStructMemory());
    }
    return true;
}
void UEntertainerComponent::Frighten(FVector Center)
{
    for (TActorIterator<AActor> I(GetWorld()); I; ++I)
        if (ValidTarget(*I) &&
            FVector::DistSquared(I->GetActorLocation(), Center) <= FMath::Square(CastStats.FrightRadius))
        {
            auto *S = I->FindComponentByClass<UBountyHunterTargetComponent>();
            if (!S)
            {
                S = NewObject<UBountyHunterTargetComponent>(*I);
                I->AddInstanceComponent(S);
                S->RegisterComponent();
            }
            S->ApplyFright(CastStats.FrightDuration, 1 - CastStats.SlowFraction);
        }
}
void UEntertainerComponent::Cancel()
{
    ++CastToken;
    auto Copies = Captures;
    Captures.Reset();
    Pending = false;
    Reserved = 0;
    PrimaryTarget.Reset();
    for (auto Weak : Copies)
        if (auto *B = Weak.Get())
            B->Abort();
    if (auto *A = Anim(GetOwner()))
        if (SnapMontage && A->Montage_IsActive(SnapMontage))
            A->Montage_Stop(.12f, SnapMontage);
}
void UEntertainerComponent::ResetRun()
{
    Cancel();
    Combo = 0;
    CooldownRemaining = 0;
    LastAction.Empty();
    ActionTimes.Reset();
    LastCategory = NAME_None;
    LastScore = -1000;
}
void UEntertainerComponent::TickComponent(float Dt, ELevelTick Tick, FActorComponentTickFunction *Fn)
{
    Super::TickComponent(Dt, Tick, Fn);
    Age += Dt;
    CooldownRemaining = FMath::Max(0.f, CooldownRemaining - Dt);
    RefreshAge += Dt;
    if (RefreshAge >= .1f)
    {
        RefreshUpgrades();
        RefreshAge = 0;
    }
    if (!Alive())
    {
        ResetRun();
        return;
    }
    auto *Pawn = Cast<APawn>(GetOwner());
    auto *Input = Pawn ? Cast<UEnhancedInputComponent>(Pawn->InputComponent) : nullptr;
    if (Input && Input != BoundInput.Get() && ActivateAction)
    {
        Input->BindAction(ActivateAction, ETriggerEvent::Started, this, &UEntertainerComponent::InputActivate);
        BoundInput = Input;
    }
    if (Settings && Age - LastScore > Settings->DecayDelay)
        Combo = FMath::Max(FMath::Min(Combo, Reserved),
                           Combo - Settings->DecayPerSecond *
                                       FMath::Max(0., Age - FMath::Max(Age - Dt, LastScore + Settings->DecayDelay)));
    for (auto &Pair : Attacks)
        if (Pair.Value.Player.Get() == GetOwner() && Flag(GetOwner(), TEXT("DashBoostActive")))
            Pair.Value.Dashed = true;
    if (Pending)
    {
        if (Age - Started > CastStats.Windup + (Settings ? Settings->CaptureTimeout : 3))
        {
            Cancel();
            return;
        }
        auto *A = Anim(GetOwner());
        if (!PrimaryCommitted && (!A || !A->Montage_IsActive(SnapMontage)))
        {
            Cancel();
            return;
        }
        if (ContactIssued)
        {
            bool Busy = false;
            for (auto Weak : Captures)
                if (auto *B = Weak.Get())
                    Busy |= Flag(B->GetOwner(), TEXT("InUse"));
            if (!Busy)
            {
                if (!PrimaryCommitted)
                    LastAction = TEXT("Projection failed");
                Cancel();
            }
        }
    }
}
void UEntertainerComponent::EndPlay(EEndPlayReason::Type Reason)
{
    if (auto *Input = BoundInput.Get())
        Input->ClearBindingsForObject(this);
    ResetRun();
    Super::EndPlay(Reason);
}
UEntertainerCaptureBridge::UEntertainerCaptureBridge()
{
    PrimaryComponentTick.bCanEverTick = false;
}
bool UEntertainerCaptureBridge::ValidateProjection(AActor *Target) const
{
    if (!Health(Target) || Number(Health(Target), TEXT("CurrentHealth")) <= 0)
        return false;
    auto *F = GetOwner()->FindFunction(TEXT("CallculateDecalPos"));
    if (!F)
        return false;
    // The legacy search uses actor scratch variables. Eligibility probes must not
    // append their hits to the later visual capture or change its trace origin.
    auto *HitsProperty = FindFProperty<FArrayProperty>(GetOwner()->GetClass(), TEXT("Hits"));
    auto *HitType = HitsProperty ? CastField<FStructProperty>(HitsProperty->Inner) : nullptr;
    if (!HitType || HitType->Struct != FHitResult::StaticStruct())
        return false;
    auto *Hits = HitsProperty->ContainerPtrToValuePtr<TArray<FHitResult>>(GetOwner());
    auto SavedHits = MoveTemp(*Hits);
    auto *OriginProperty = FindFProperty<FStructProperty>(GetOwner()->GetClass(), TEXT("TraceOrigin"));
    auto *Origin = OriginProperty && OriginProperty->Struct == TBaseStructure<FTransform>::Get()
                       ? OriginProperty->ContainerPtrToValuePtr<FTransform>(GetOwner())
                       : nullptr;
    const FTransform SavedOrigin = Origin ? *Origin : FTransform::Identity;
    ON_SCOPE_EXIT
    {
        *Hits = MoveTemp(SavedHits);
        if (Origin)
            *Origin = SavedOrigin;
    };
    FStructOnScope Args(F);
    Arg(F, Args.GetStructMemory(), TEXT("Enemy"), Target);
    GetOwner()->ProcessEvent(F, Args.GetStructMemory());
    auto *P = FindFProperty<FArrayProperty>(F, TEXT("Surfaces"));
    if (!P)
        return false;
    FScriptArrayHelper A(P, P->ContainerPtrToValuePtr<void>(Args.GetStructMemory()));
    for (int I = 0; I < A.Num(); ++I)
        if (reinterpret_cast<FHitResult *>(A.GetRawPtr(I))->IsValidBlockingHit())
            return true;
    return false;
}
bool UEntertainerCaptureBridge::Reserve(UEntertainerComponent *C, AActor *Target, int32 T, bool IsPrimary)
{
    if (Active || Flag(GetOwner(), TEXT("InUse")))
        return false;
    Source = C;
    Enemy = Target;
    Token = T;
    Primary = IsPrimary;
    Active = true;
    Started = false;
    Flag(GetOwner(), TEXT("InUse"), true);
    return true;
}
void UEntertainerCaptureBridge::Start()
{
    if (!Active || Started)
        return;
    if (!Source.IsValid() || !ValidateProjection(Enemy.Get()))
    {
        Abort();
        return;
    }
    Started = true;
    if (auto *F = GetOwner()->FindFunction(TEXT("CaptureAtCameraTransform")))
    {
        FStructOnScope Args(F);
        Arg(F, Args.GetStructMemory(), TEXT("Enemy"), Enemy.Get());
        GetOwner()->ProcessEvent(F, Args.GetStructMemory());
    }
    else
        Abort();
}
void UEntertainerCaptureBridge::PrepareCapture(AActor *Target)
{
    RestoreCapture();
    if (!IsValid(Target))
        return;
    for (auto *M : TInlineComponentArray<USkeletalMeshComponent *>(Target))
    {
        DepthStates.Add({M, bool(M->bRenderCustomDepth), M->CustomDepthStencilValue});
        M->SetRenderCustomDepth(true);
        M->SetCustomDepthStencilValue(7);
    }
}
void UEntertainerCaptureBridge::RestoreCapture()
{
    for (auto &S : DepthStates)
        if (auto *M = S.Mesh.Get())
        {
            M->SetRenderCustomDepth(S.Enabled);
            M->SetCustomDepthStencilValue(S.Stencil);
        }
    DepthStates.Reset();
}
bool UEntertainerCaptureBridge::Complete(AActor *Target, AActor *Projection)
{
    RestoreCapture();
    bool Success = Active && Target == Enemy.Get() && Source.IsValid() && ValidateProjection(Target) &&
                   Source->CompleteCapture(this, Target, Projection, Token, Primary);
    Active = false;
    Source.Reset();
    Enemy.Reset();
    if (!Success && IsValid(Projection))
        Projection->Destroy();
    Flag(GetOwner(), TEXT("InUse"), false);
    return Success;
}
void UEntertainerCaptureBridge::Abort()
{
    if (!Active)
        return;
    Active = false;
    if (GetWorld())
        GetWorld()->GetLatentActionManager().RemoveActionsForObject(GetOwner());
    RestoreCapture();
    Flag(GetOwner(), TEXT("InUse"), false);
    Source.Reset();
    Enemy.Reset();
}
void UEntertainerCaptureBridge::EndPlay(EEndPlayReason::Type Reason)
{
    Abort();
    Super::EndPlay(Reason);
}
void UEntertainerCombatEvents::SetDamageSource(AActor *Enemy, AActor *Source, FName Category, bool Suppress)
{
    if (auto *T = Tracker(Enemy))
    {
        T->DamageSource = Source;
        T->Category = Category;
        T->Suppress = Suppress;
    }
}
void UEntertainerCombatEvents::BeforeDamage(UActorComponent *H)
{
    if (H)
        if (auto *T = Tracker(H->GetOwner()))
            T->PreviousHP = Number(H, TEXT("CurrentHealth"));
}
void UEntertainerCombatEvents::AfterDamage(UActorComponent *H)
{
    if (!H)
        return;
    auto *T = Tracker(H->GetOwner());
    float Now = Number(H, TEXT("CurrentHealth"));
    float Before = T->PreviousHP;
    T->PreviousHP = Now;
    if (Now >= Before || Before <= 0)
        return;
    if (auto *Hero = H->GetOwner()->FindComponentByClass<UEntertainerComponent>())
    {
        Hero->TookDamage();
        return;
    }
    if (!Health(H->GetOwner()))
        return;
    auto *Hero = T->DamageSource.IsValid() ? T->DamageSource->FindComponentByClass<UEntertainerComponent>() : Player(H);
    if (Hero && Hero->Settings && !T->Suppress)
    {
        bool Finish = T->Finisher || Flag(H, TEXT("FinisherActive"));
        if (!Finish)
            Hero->Score(TEXT("Hit"), T->Category, Hero->Settings->HitScore);
        if (Now <= 0 && !T->KillAwarded)
        {
            T->KillAwarded = true;
            Hero->Score(Finish ? TEXT("Finisher") : TEXT("Kill"), Finish ? FName(TEXT("Finisher")) : T->Category,
                        Finish ? Hero->Settings->FinisherScore : Hero->Settings->KillScore);
        }
    }
    T->DamageSource.Reset();
    T->Category = TEXT("Melee");
    T->Finisher = false;
}
void UEntertainerCombatEvents::SuccessfulBlock(UActorComponent *Combat, float Incoming, float Applied)
{
    if (Combat && Incoming > 0 && Applied + KINDA_SMALL_NUMBER < Incoming)
        if (auto *C = Combat->GetOwner()->FindComponentByClass<UEntertainerComponent>())
            if (C->Settings)
                C->Score(TEXT("Block"), TEXT("Defence"), C->Settings->BlockScore);
}
void UEntertainerCombatEvents::BeginFinisher(UActorComponent *H)
{
    if (H)
        Tracker(H->GetOwner())->Finisher = true;
}
void UEntertainerCombatEvents::AttackStarted(AActor *Attacker)
{
    if (!Attacker)
        return;
    for (auto It = Attacks.CreateIterator(); It; ++It)
        if (!It.Key().IsValid() || Attacker->GetWorld()->GetTimeSeconds() - It.Value().Time > 5)
            It.RemoveCurrent();
    auto *C = Player(Attacker);
    if (!C)
        return;
    auto *P = C->GetOwner();
    FVector Delta = P->GetActorLocation() - Attacker->GetActorLocation();
    if (Delta.Size() <= 250 && FVector::DotProduct(Attacker->GetActorForwardVector(), Delta.GetSafeNormal()) > .5)
        Attacks.Add(Attacker, {P, P->GetActorLocation(), Attacker->GetWorld()->GetTimeSeconds(),
                               float(Number(Object(P, TEXT("AC_PlayerHealth")), TEXT("CurrentHealth"))), false});
}
void UEntertainerCombatEvents::AttackResolved(AActor *Attacker)
{
    FAttackWatch Watch;
    if (!Attacks.RemoveAndCopyValue(Attacker, Watch) || !Watch.Player.IsValid() || !Watch.Dashed)
        return;
    auto *P = Watch.Player.Get();
    if (Attacker->GetWorld()->GetTimeSeconds() - Watch.Time > 2.5)
        return;
    auto *Combat = Object(Attacker, TEXT("AC_EnemyCombat"));
    if (Combat)
        if (auto *Hits = FindFProperty<FArrayProperty>(Combat->GetClass(), TEXT("HitActors")))
        {
            FScriptArrayHelper A(Hits, Hits->ContainerPtrToValuePtr<void>(Combat));
            if (auto *Inner = CastField<FObjectPropertyBase>(Hits->Inner))
                for (int I = 0; I < A.Num(); ++I)
                    if (Inner->GetObjectPropertyValue(A.GetRawPtr(I)) == P)
                        return;
        }
    if (Number(Object(P, TEXT("AC_PlayerHealth")), TEXT("CurrentHealth")) < Watch.HP ||
        FVector::Dist(P->GetActorLocation(), Watch.Start) < 100)
        return;
    if (auto *C = P->FindComponentByClass<UEntertainerComponent>())
        if (C->Settings)
            C->Score(TEXT("Dodge"), TEXT("Movement"), C->Settings->DodgeScore);
}
void UAnimNotify_EntertainerContact::Notify(USkeletalMeshComponent *Mesh, UAnimSequenceBase *Animation,
                                            const FAnimNotifyEventReference &Ref)
{
    if (Mesh)
        if (auto *C = Mesh->GetOwner()->FindComponentByClass<UEntertainerComponent>())
            C->Contact();
}
void UEntertainerComboWidget::NativeTick(const FGeometry &Geometry, float Dt)
{
    Super::NativeTick(Geometry, Dt);
    auto *P = GetOwningPlayerPawn();
    auto *C = P ? P->FindComponentByClass<UEntertainerComponent>() : nullptr;
    bool Show = C && C->Alive() && C->Stats.Unlocked && C->Stats.Equipped;
    SetRenderOpacity(Show ? 1.f : 0.f);
    SetVisibility(ESlateVisibility::HitTestInvisible);
    if (!Show)
        return;
    ComboText->SetText(FText::FromString(
        FString::Printf(TEXT("COMBO  %s   %.0f / %.0f"), *C->Rank(), C->Combo, C->Settings->Capacity)));
    ComboBar->SetPercent(C->Combo / FMath::Max(1.f, C->Settings->Capacity));
    FString State = C->Pending                 ? TEXT("Snapping...")
                    : C->CooldownRemaining > 0 ? FString::Printf(TEXT("H  Encore in %.1fs"), C->CooldownRemaining)
                    : C->Combo < C->Stats.Cost ? FString::Printf(TEXT("H  Need %.0f Combo"), C->Stats.Cost)
                                               : TEXT("H  Snap ready");
    ComboState->SetText(FText::FromString(State));
    ComboAction->SetText(FText::FromString(C->LastAction));
}
