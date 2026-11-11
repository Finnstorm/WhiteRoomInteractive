#include "BountyHunter.h"
#include "Entertainer.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UnrealType.h"
#include "UObject/StructOnScope.h"
namespace
{
UObject *ReadObject(UObject *O, FName Name)
{
    if (O)
        if (auto *P = FindFProperty<FObjectPropertyBase>(O->GetClass(), Name))
            return P->GetObjectPropertyValue_InContainer(O);
    return nullptr;
}
double ReadNumber(UObject *O, FName Name, double Fallback = 0)
{
    if (O)
        if (auto *P = FindFProperty<FNumericProperty>(O->GetClass(), Name))
        {
            const void *V = P->ContainerPtrToValuePtr<void>(O);
            return P->IsFloatingPoint() ? P->GetFloatingPointPropertyValue(V) : double(P->GetSignedIntPropertyValue(V));
        }
    return Fallback;
}
bool ReadBool(UObject *O, FName Name)
{
    if (O)
        if (auto *P = FindFProperty<FBoolProperty>(O->GetClass(), Name))
            return P->GetPropertyValue_InContainer(O);
    return false;
}
UActorComponent *EnemyHealth(AActor *Actor)
{
    if (!Actor || Actor->IsActorBeingDestroyed() || Cast<APawn>(Actor) && Cast<APawn>(Actor)->IsPlayerControlled())
        return nullptr;
    for (auto *C : TInlineComponentArray<UActorComponent *>(Actor))
        if (([](UClass *Class) {
                for (; Class; Class = Class->GetSuperClass())
                    if (Class->GetName().Contains(TEXT("EnemyHealth")))
                        return true;
                return false;
            })(C->GetClass()) &&
            FindFProperty<FNumericProperty>(C->GetClass(), TEXT("CurrentHealth")))
            return C;
    return nullptr;
}
bool Armed(AActor *Target)
{
    if (Target->ActorHasTag(TEXT("StunGunCarrier")))
        return true;
    if (auto *C = Target->FindComponentByClass<UBountyHunterTargetComponent>())
        if (C->bCarriesStunGun)
            return true;
    return ReadBool(Target, TEXT("bCarriesStunGun"));
}
void Feedback(UObject *Context, const FString &Text)
{
    UKismetSystemLibrary::PrintString(Context, Text, true, true, FLinearColor(.2, .8, 1), 2);
}
} // namespace
UBountyHunterTargetComponent::UBountyHunterTargetComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}
void UBountyHunterTargetComponent::UpdateSlow()
{
    auto *C = Cast<ACharacter>(GetOwner());
    if (!C)
        return;
    auto *M = C->GetCharacterMovement();
    if (!Movement.IsValid())
    {
        Movement = M;
        OriginalSpeed = M->MaxWalkSpeed;
    }
    else if (!FMath::IsNearlyEqual(M->MaxWalkSpeed, AppliedSpeed, 1.f))
        OriginalSpeed = M->MaxWalkSpeed;
    float Factor = 1;
    if (StunRemaining > 0)
        Factor = FMath::Min(Factor, Multiplier);
    if (FrightRemaining > 0)
        Factor = FMath::Min(Factor, FrightMultiplier);
    AppliedSpeed = OriginalSpeed * Factor;
    M->MaxWalkSpeed = AppliedSpeed;
}
void UBountyHunterTargetComponent::ApplyStun(float Duration, float SpeedMultiplier)
{
    if (Duration <= 0)
        return;
    Multiplier = StunRemaining > 0 ? FMath::Min(Multiplier, FMath::Clamp(SpeedMultiplier, 0.f, 1.f))
                                   : FMath::Clamp(SpeedMultiplier, 0.f, 1.f);
    StunRemaining = FMath::Max(StunRemaining, Duration);
    UpdateSlow();
}
void UBountyHunterTargetComponent::ApplyFright(float Duration, float SpeedMultiplier)
{
    if (Duration <= 0)
        return;
    FrightMultiplier = FrightRemaining > 0 ? FMath::Min(FrightMultiplier, FMath::Clamp(SpeedMultiplier, 0.f, 1.f))
                                           : FMath::Clamp(SpeedMultiplier, 0.f, 1.f);
    FrightRemaining = FMath::Max(FrightRemaining, Duration);
    UpdateSlow();
}
void UBountyHunterTargetComponent::Restore()
{
    if (auto *M = Movement.Get())
        if (FMath::IsNearlyEqual(M->MaxWalkSpeed, AppliedSpeed, 1.f))
            M->MaxWalkSpeed = OriginalSpeed;
    StunRemaining = 0;
    FrightRemaining = 0;
    Movement.Reset();
}
void UBountyHunterTargetComponent::TickComponent(float Dt, ELevelTick Tick, FActorComponentTickFunction *Fn)
{
    Super::TickComponent(Dt, Tick, Fn);
    if (StunRemaining <= 0 && FrightRemaining <= 0)
        return;
    StunRemaining = FMath::Max(0.f, StunRemaining - Dt);
    FrightRemaining = FMath::Max(0.f, FrightRemaining - Dt);
    if (StunRemaining <= 0 && FrightRemaining <= 0)
        Restore();
    else
        UpdateSlow();
}
void UBountyHunterTargetComponent::EndPlay(EEndPlayReason::Type Reason)
{
    Restore();
    Super::EndPlay(Reason);
}
ABountyHunterGun::ABountyHunterGun()
{
    PrimaryActorTick.bCanEverTick = false;
    auto *Root = CreateDefaultSubobject<USceneComponent>(TEXT("GunRoot"));
    SetRootComponent(Root);
    Barrel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Barrel"));
    Grip = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Grip"));
    Barrel->SetupAttachment(Root);
    Grip->SetupAttachment(Root);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    Barrel->SetStaticMesh(Cube.Object);
    Grip->SetStaticMesh(Cube.Object);
    Barrel->SetRelativeLocation(FVector(9, 0, 5));
    Barrel->SetRelativeScale3D(FVector(.3, .065, .08));
    Grip->SetRelativeLocation(FVector(0, 0, -3));
    Grip->SetRelativeScale3D(FVector(.065, .065, .16));
    Grip->SetRelativeRotation(FRotator(12, 0, 0));
    Barrel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Grip->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Barrel->SetCastShadow(false);
    Grip->SetCastShadow(false);
    SetActorEnableCollision(false);
}
ABountyHunterProjectile::ABountyHunterProjectile()
{
    PrimaryActorTick.bCanEverTick = true;
    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    Collision->InitSphereRadius(3);
    SetRootComponent(Collision);
    Collision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    Collision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
    Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Collision->SetNotifyRigidBodyCollision(true);
    Collision->OnComponentHit.AddDynamic(this, &ABountyHunterProjectile::Hit);
    Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Bullet"));
    Visual->SetupAttachment(Collision);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    Visual->SetStaticMesh(Sphere.Object);
    Visual->SetRelativeScale3D(FVector(.06));
    Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Visual->SetCastShadow(false);
    Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    Movement->UpdatedComponent = Collision;
    Movement->InitialSpeed = 2000;
    Movement->MaxSpeed = 0;
    Movement->ProjectileGravityScale = 0;
    Movement->bRotationFollowsVelocity = true;
    Movement->bForceSubStepping = true;
    InitialLifeSpan = 3;
}
void ABountyHunterProjectile::Launch(UBountyHunterComponent *Source, const FBountyHunterStats &Stats,
                                     UBountyHunterSettings *Tuning, FVector Direction)
{
    SourceComponent = Source;
    ShotStats = Stats;
    Settings = Tuning;
    PreviousPosition = GetActorLocation();
    if (Tuning->ProjectileMaterial)
        Visual->SetMaterial(0, Tuning->ProjectileMaterial);
    Collision->IgnoreActorWhenMoving(Source->GetOwner(), true);
    Movement->Velocity = Direction.GetSafeNormal() * Stats.Speed;
    Movement->ProjectileGravityScale = 0;
    SetLifeSpan(Tuning->ProjectileLifeSeconds);
}
void ABountyHunterProjectile::Tick(float Dt)
{
    Super::Tick(Dt);
    DistanceTravelled += FVector::Dist(PreviousPosition, GetActorLocation());
    PreviousPosition = GetActorLocation();
    if (Settings && DistanceTravelled >= ShotStats.Range)
        Movement->ProjectileGravityScale = Settings->GravityAfterRange;
}
void ABountyHunterProjectile::Hit(UPrimitiveComponent *Component, AActor *Other, UPrimitiveComponent *OtherComponent,
                                  FVector Impulse, const FHitResult &Impact)
{
    if (bImpacted || Other == GetOwner())
        return;
    bImpacted = true;
    Movement->StopMovementImmediately();
    if (auto *Source = SourceComponent.Get())
        Source->ResolveImpact(Impact, ShotStats);
    Destroy();
}
FBountyHunterStats UBountyHunterComponent::CalculateStats(const UBountyHunterSettings *S, const UDataTable *Table,
                                                          const TArray<FName> &IDs, bool Equipped)
{
    FBountyHunterStats R;
    if (!S)
        return R;
    R.Equipped = Equipped;
    R.MaxBullets = S->BaseBullets;
    float DamageBonus = 0, CooldownReduction = 0, WindupReduction = 0, SpeedBonus = 0, RangeBonus = 0,
          DurationBonus = 0, BlastBonus = 0, KnockBonus = 0;
    for (FName ID : IDs)
    {
        const auto *U = Table ? Table->FindRow<FBountyHunterUpgrade>(ID, TEXT("Bounty Hunter"), false) : nullptr;
        if (!U)
            continue;
        R.Unlocked |= U->UnlockGun;
        R.MaxBullets += U->ExtraBullets;
        DamageBonus += U->BulletDamageBonus;
        CooldownReduction += U->ShotCooldownReduction;
        WindupReduction += U->WindupReduction;
        SpeedBonus += U->BulletSpeedBonus;
        RangeBonus += U->BulletRangeBonus;
        DurationBonus += U->StunDurationBonus;
        BlastBonus += U->ExplosionDamageBonus;
        KnockBonus += U->ExplosionKnockbackBonus;
        R.Stun |= U->UnlockStun;
        R.KillRefund |= U->UnlockKillRefund;
        R.Explosion |= U->UnlockExplosion;
    }
    R.MaxBullets = FMath::Max(1, R.MaxBullets);
    R.Damage = FMath::Max(0.f, S->BulletDamage * (1 + DamageBonus));
    R.Cooldown = FMath::Max(.05f, S->ShotCooldown * (1 - CooldownReduction));
    R.Windup = FMath::Max(.05f, S->WindupTime * (1 - WindupReduction));
    R.Speed = FMath::Max(1.f, S->BulletSpeed * (1 + SpeedBonus));
    R.Range = FMath::Max(1.f, S->BulletRange * (1 + RangeBonus));
    R.StunDuration = FMath::Max(0.f, S->StunDuration * (1 + DurationBonus));
    R.ExplosionDamage = FMath::Max(0.f, S->ExplosionDamage * (1 + BlastBonus));
    R.Knockback = FMath::Max(0.f, S->ExplosionKnockback * (1 + KnockBonus));
    return R;
}
UBountyHunterComponent::UBountyHunterComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}
void UBountyHunterComponent::BeginPlay()
{
    Super::BeginPlay();
    RefreshUpgrades();
}
USkeletalMeshComponent *UBountyHunterComponent::PlayerMesh() const
{
    auto *C = Cast<ACharacter>(GetOwner());
    return C ? C->GetMesh() : nullptr;
}
bool UBountyHunterComponent::Alive() const
{
    if (!GetOwner() || GetOwner()->IsActorBeingDestroyed())
        return false;
    auto *H = ReadObject(GetOwner(), TEXT("AC_PlayerHealth"));
    return H && ReadNumber(H, TEXT("CurrentHealth")) > 0;
}
bool UBountyHunterComponent::IsBlocking() const
{
    return ReadBool(ReadObject(GetOwner(), TEXT("AC_CombatSystem")), TEXT("BlockActive"));
}
void UBountyHunterComponent::RefreshUpgrades()
{
    if (!Settings)
        return;
    auto *GI = UGameplayStatics::GetGameInstance(this);
    auto *Save = ReadObject(GI, TEXT("SaveGameRef"));
    if (!Save)
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
        Equipped = P->GetPropertyValue_InContainer(Save) == TEXT("BountyHunter");
    int OldMax = Stats.MaxBullets;
    bool OldUnlocked = Stats.Unlocked;
    Stats = CalculateStats(Settings, Upgrades, IDs, Equipped);
    if (!bAmmoInitialized || (!OldUnlocked && Stats.Unlocked))
    {
        Bullets = Stats.MaxBullets;
        bAmmoInitialized = true;
    }
    else if (Stats.MaxBullets > OldMax)
        Bullets += Stats.MaxBullets - OldMax;
    Bullets = FMath::Clamp(Bullets, 0, Stats.MaxBullets);
    if ((!Stats.Unlocked || !Stats.Equipped) && bWindingUp)
        CancelGunAction();
}
bool UBountyHunterComponent::TryFire()
{
    RefreshUpgrades();
    auto *Mesh = PlayerMesh();
    auto *Anim = Mesh ? Mesh->GetAnimInstance() : nullptr;
    auto *C = Cast<ACharacter>(GetOwner());
    if (!Settings || !Alive() || !Stats.Unlocked || !Stats.Equipped || Bullets <= 0 || bWindingUp || IsBlocking() ||
        !Anim || !FireMontage || !C || C->GetCharacterMovement()->MovementMode == MOVE_None)
        return false;
    double Now = GetWorld()->GetTimeSeconds();
    if (Now + Stats.Windup + KINDA_SMALL_NUMBER < LastShot + Stats.Cooldown)
        return false;
    bWindingUp = true;
    bShotIssued = false;
    float FireTime = Settings->AnimationFireTime;
    for (const FAnimNotifyEvent &Notify : FireMontage->Notifies)
        if (Notify.Notify && Notify.Notify->IsA<UAnimNotify_BountyHunterFire>())
        {
            FireTime = Notify.GetTime();
            break;
        }
    float Rate = FMath::Max(.01f, FireTime) / Stats.Windup;
    float Result = Anim->Montage_PlayWithBlendIn(
        FireMontage, FAlphaBlendArgs(FMath::Min(Settings->BlendInTime, Stats.Windup * .75f)), Rate,
        EMontagePlayReturnType::MontageLength, 0, false);
    if (Result <= 0)
    {
        bWindingUp = false;
        return false;
    }
    if (!Gun)
    {
        FActorSpawnParameters P;
        P.Owner = GetOwner();
        P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Gun = GetWorld()->SpawnActor<ABountyHunterGun>(GunClass ? GunClass.Get() : ABountyHunterGun::StaticClass(),
                                                       FTransform::Identity, P);
        if (Gun)
        {
            if (Settings->GunMaterial)
            {
                Gun->Barrel->SetMaterial(0, Settings->GunMaterial);
                Gun->Grip->SetMaterial(0, Settings->GunMaterial);
            }
            Gun->AttachToComponent(Mesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("RightHand"));
            Gun->SetActorRelativeTransform(Settings->GunGripOffset);
        }
    }
    if (Gun)
        Gun->SetActorHiddenInGame(false);
    return true;
}
void UBountyHunterComponent::FireAtNotify()
{
    if (!PlayerMesh() || !PlayerMesh()->GetAnimInstance() ||
        PlayerMesh()->GetAnimInstance()->Montage_GetIsStopped(FireMontage) || !bWindingUp || bShotIssued || !Alive() ||
        !Stats.Unlocked || !Stats.Equipped || Bullets <= 0)
        return;
    bShotIssued = true;
    auto *Pawn = Cast<APawn>(GetOwner());
    FVector Eye;
    FRotator View;
    if (auto *PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr)
        PC->GetPlayerViewPoint(Eye, View);
    else if (Pawn)
        Pawn->GetActorEyesViewPoint(Eye, View);
    else
        return;
    FVector Muzzle = Eye + View.Vector() * 45 + FRotationMatrix(View).GetUnitAxis(EAxis::Y) * 12 - FVector(0, 0, 12);
    if (Gun && Gun->Barrel)
        Muzzle = Gun->GetActorTransform().TransformPosition(FVector(26, 0, 5));
    FCollisionQueryParams Q;
    Q.AddIgnoredActor(GetOwner());
    if (Gun)
        Q.AddIgnoredActor(Gun);
    FHitResult Aim;
    FVector Target = Eye + View.Vector() * 5000;
    if (GetWorld()->LineTraceSingleByChannel(Aim, Eye, Target, ECC_Visibility, Q))
        Target = Aim.ImpactPoint;
    FHitResult Obstruction;
    if (GetWorld()->SweepSingleByChannel(Obstruction, Eye, Muzzle, FQuat::Identity, ECC_Visibility,
                                         FCollisionShape::MakeSphere(3), Q))
    {
        --Bullets;
        LastShot = GetWorld()->GetTimeSeconds();
        ResolveImpact(Obstruction, Stats);
        return;
    }
    FActorSpawnParameters P;
    P.Owner = GetOwner();
    P.Instigator = Pawn;
    P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto *Bullet = GetWorld()->SpawnActor<ABountyHunterProjectile>(
        ProjectileClass ? ProjectileClass.Get() : ABountyHunterProjectile::StaticClass(), Muzzle,
        (Target - Muzzle).Rotation(), P);
    if (Bullet)
    {
        Bullet->Launch(this, Stats, Settings, Target - Muzzle);
        --Bullets;
        LastShot = GetWorld()->GetTimeSeconds();
        Feedback(this, FString::Printf(TEXT("Bounty Hunter: %d / %d bullets"), Bullets, Stats.MaxBullets));
    }
}
void UBountyHunterComponent::RefillBullets()
{
    RefreshUpgrades();
    if (!Alive() || !Stats.Unlocked)
        return;
    Bullets = Stats.MaxBullets;
    Feedback(this, FString::Printf(TEXT("Stun gun refilled: %d / %d [Y test key]"), Bullets, Stats.MaxBullets));
}
void UBountyHunterComponent::CancelGunAction()
{
    if (auto *M = PlayerMesh())
        if (auto *A = M->GetAnimInstance())
            if (FireMontage && A->Montage_IsPlaying(FireMontage))
                A->Montage_Stop(Settings ? Settings->BlendOutTime : .16f, FireMontage);
    bWindingUp = false;
    bShotIssued = false;
    if (Gun)
        Gun->SetActorHiddenInGame(true);
}
void UBountyHunterComponent::UpdateBlockAnimation(bool Active)
{
    if (Active == bBlocking)
    {
        if (Active)
        {
            auto *Mesh = PlayerMesh();
            auto *Anim = Mesh ? Mesh->GetAnimInstance() : nullptr;
            if (Anim && BlockMontage && !Anim->Montage_IsPlaying(BlockMontage) && !Anim->IsAnyMontagePlaying())
            {
                Anim->Montage_PlayWithBlendIn(BlockMontage, FAlphaBlendArgs(Settings ? Settings->BlendInTime : .12f), 1,
                                              EMontagePlayReturnType::MontageLength, 0, false);
                Anim->Montage_JumpToSection(TEXT("Hold"), BlockMontage);
            }
        }
        return;
    }
    bBlocking = Active;
    auto *Mesh = PlayerMesh();
    auto *Anim = Mesh ? Mesh->GetAnimInstance() : nullptr;
    if (!Anim || !BlockMontage)
        return;
    if (Active)
    {
        CancelGunAction();
        Anim->Montage_PlayWithBlendIn(BlockMontage, FAlphaBlendArgs(Settings ? Settings->BlendInTime : .12f), 1,
                                      EMontagePlayReturnType::MontageLength, 0, false);
    }
    else if (Anim->Montage_IsPlaying(BlockMontage))
        Anim->Montage_Stop(Settings ? Settings->BlendOutTime : .16f, BlockMontage);
}
void UBountyHunterComponent::TickComponent(float Dt, ELevelTick Tick, FActorComponentTickFunction *Fn)
{
    Super::TickComponent(Dt, Tick, Fn);
    if (auto *Pawn = Cast<APawn>(GetOwner()))
        if (auto *Input = Cast<UEnhancedInputComponent>(Pawn->InputComponent))
            if (Input != BoundInput.Get())
            {
                if (FireAction)
                    Input->BindAction(FireAction, ETriggerEvent::Started, this, &UBountyHunterComponent::InputFire);
                if (DebugReloadAction)
                    Input->BindAction(DebugReloadAction, ETriggerEvent::Started, this,
                                      &UBountyHunterComponent::RefillBullets);
                BoundInput = Input;
            }
    RefreshAccumulator += Dt;
    if (RefreshAccumulator >= .2f)
    {
        RefreshAccumulator = 0;
        RefreshUpgrades();
    }
    CooldownRemaining = FMath::Max(0., LastShot + Stats.Cooldown - GetWorld()->GetTimeSeconds());
    if (!Alive())
    {
        CancelGunAction();
        UpdateBlockAnimation(false);
        ArmAimWeight = 0;
        return;
    }
    UpdateBlockAnimation(IsBlocking());
    ArmAimWeight = FMath::FInterpTo(ArmAimWeight,
                                    (bWindingUp || bBlocking ||
                                     (GetOwner()->FindComponentByClass<UEntertainerComponent>() &&
                                      GetOwner()->FindComponentByClass<UEntertainerComponent>()->Pending))
                                        ? 1.f
                                        : 0.f,
                                    Dt, 12.f);
    if (bWindingUp)
    {
        auto *M = PlayerMesh();
        auto *Anim = M ? M->GetAnimInstance() : nullptr;
        if (!Anim || !Anim->Montage_IsPlaying(FireMontage))
            CancelGunAction();
    }
}
void UBountyHunterComponent::EndPlay(EEndPlayReason::Type Reason)
{
    CancelGunAction();
    UpdateBlockAnimation(false);
    if (Gun)
        Gun->Destroy();
    if (auto *Input = BoundInput.Get())
        Input->ClearBindingsForObject(this);
    Super::EndPlay(Reason);
}
bool UBountyHunterComponent::DamageEnemy(AActor *Target, float Damage, const FHitResult &Hit, bool AllowRefund)
{
    auto *H = EnemyHealth(Target);
    if (!H || ReadNumber(H, TEXT("CurrentHealth")) <= 0)
        return false;
    const bool WasArmed = Armed(Target);
    auto *Fn = H->FindFunction(TEXT("ProcessDamage"));
    if (!Fn)
        return false;
    FStructOnScope Args(Fn);
    for (TFieldIterator<FProperty> I(Fn); I; ++I)
        if (I->HasAnyPropertyFlags(CPF_Parm))
        {
            if (auto *N = CastField<FNumericProperty>(*I))
            {
                auto *V = N->ContainerPtrToValuePtr<void>(Args.GetStructMemory());
                double Value = I->GetFName() == TEXT("DamageAmount") ? Damage
                               : I->GetFName() == TEXT("DeathDelay") ? 2.
                                                                     : 0.;
                if (N->IsFloatingPoint())
                    N->SetFloatingPointPropertyValue(V, Value);
            }
        }
    UEntertainerCombatEvents::SetDamageSource(Target, GetOwner(), TEXT("Ability"));
    H->ProcessEvent(Fn, Args.GetStructMemory());
    bool Killed =
        !IsValid(Target) || Target->IsActorBeingDestroyed() || !IsValid(H) || ReadNumber(H, TEXT("CurrentHealth")) <= 0;
    if (Killed && WasArmed && AllowRefund)
        Bullets = FMath::Min(Bullets + 1, Stats.MaxBullets);
    return true;
}
void UBountyHunterComponent::ResolveImpact(const FHitResult &Hit, const FBountyHunterStats &Fired)
{
    if (!Settings)
        return;
    AActor *Direct = Hit.GetActor();
    if (DamageEnemy(Direct, Fired.Damage, Hit, Fired.KillRefund) && Fired.Stun && IsValid(Direct) &&
        EnemyHealth(Direct) && ReadNumber(EnemyHealth(Direct), TEXT("CurrentHealth")) > 0)
    {
        auto *Status = Direct->FindComponentByClass<UBountyHunterTargetComponent>();
        if (!Status)
        {
            Status = NewObject<UBountyHunterTargetComponent>(Direct);
            Direct->AddInstanceComponent(Status);
            Status->RegisterComponent();
        }
        Status->ApplyStun(Fired.StunDuration, Settings->StunnedSpeedMultiplier);
    }
    if (Fired.Explosion)
    {
        TArray<FOverlapResult> Hits;
        FCollisionObjectQueryParams Objects;
        Objects.AddObjectTypesToQuery(ECC_Pawn);
        Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
        FCollisionQueryParams Q;
        Q.AddIgnoredActor(GetOwner());
        GetWorld()->OverlapMultiByObjectType(Hits, Hit.ImpactPoint, FQuat::Identity, Objects,
                                             FCollisionShape::MakeSphere(Settings->ExplosionRadius), Q);
        TSet<AActor *> Done;
        for (const auto &Item : Hits)
        {
            AActor *Target = Item.GetActor();
            if (!Target || Done.Contains(Target) || !EnemyHealth(Target))
                continue;
            Done.Add(Target);
            FVector Origin = Hit.ImpactPoint + Hit.ImpactNormal * 5;
            FVector Aim = Target->GetActorLocation();
            FHitResult Block;
            FCollisionQueryParams Visibility;
            Visibility.AddIgnoredActor(GetOwner());
            if (Direct)
                Visibility.AddIgnoredActor(Direct);
            if (GetWorld()->LineTraceSingleByChannel(Block, Origin, Aim, ECC_Visibility, Visibility) &&
                Block.GetActor() != Target)
                continue;
            if (!DamageEnemy(Target, Fired.ExplosionDamage, Hit, Fired.KillRefund))
                continue;
            if (auto *C = Cast<ACharacter>(Target))
            {
                FVector Direction = (C->GetActorLocation() - Hit.ImpactPoint).GetSafeNormal2D();
                if (Direction.IsNearlyZero())
                    Direction = Hit.ImpactNormal.GetSafeNormal2D();
                C->LaunchCharacter(Direction * Fired.Knockback + FVector(0, 0, Settings->ExplosionUpward), false,
                                   false);
            }
        }
    }
    FActorSpawnParameters P;
    P.Owner = GetOwner();
    auto *Flash =
        GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), FTransform(Hit.ImpactPoint + Hit.ImpactNormal * 10), P);
    if (Flash)
    {
        auto *Light = NewObject<UPointLightComponent>(Flash);
        Flash->SetRootComponent(Light);
        Light->SetWorldLocation(Hit.ImpactPoint + Hit.ImpactNormal * 10);
        Light->SetLightColor(FLinearColor(.2, .65, 1));
        Light->SetIntensity(3000);
        Light->SetAttenuationRadius(Fired.Explosion ? Settings->ExplosionRadius : 100);
        Light->SetCastShadows(false);
        Light->RegisterComponent();
        Flash->SetLifeSpan(.15);
    }
}
void UAnimNotify_BountyHunterFire::Notify(USkeletalMeshComponent *Mesh, UAnimSequenceBase *Animation,
                                          const FAnimNotifyEventReference &Ref)
{
    Super::Notify(Mesh, Animation, Ref);
    if (Mesh && Mesh->GetOwner())
        if (auto *C = Mesh->GetOwner()->FindComponentByClass<UBountyHunterComponent>())
            C->FireAtNotify();
}

void UBountyHunterAnimationLibrary::UpdateArmAim(UAnimInstance *Anim)
{
    if (!Anim)
        return;
    FRotator Rotation = FRotator::ZeroRotator;
    if (auto *Pawn = Cast<APawn>(Anim->GetOwningActor()))
        if (auto *C = Pawn->FindComponentByClass<UBountyHunterComponent>())
            Rotation.Roll =
                -FMath::Clamp(FRotator::NormalizeAxis(Pawn->GetBaseAimRotation().Pitch), -75.f, 75.f) * C->ArmAimWeight;
    if (auto *P = FindFProperty<FStructProperty>(Anim->GetClass(), TEXT("AbilityArmAimRotation")))
        *P->ContainerPtrToValuePtr<FRotator>(Anim) = Rotation;
}
