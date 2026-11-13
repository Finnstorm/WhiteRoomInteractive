#include "ProgressEnemySpawner.h"
#include "ProgressEnemyCharacter.h"
#include "ProgressEnemyReflection.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformMisc.h"

AProgressEnemySpawner::AProgressEnemySpawner()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = .5f;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}
void AProgressEnemySpawner::BeginPlay()
{
    Super::BeginPlay();
    Random.Initialize(Seed);
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("ProgressEnemyVerifyTestScene")))
    {
        FTimerHandle Handle;
        GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this]()
        {
            int32 Controllers = 0, Moving = 0;
            for (const auto& Enemy : Spawned) if (Enemy.IsValid())
            {
                if (Enemy->GetController()) ++Controllers;
                if (Enemy->GetVelocity().Size2D() > 5) ++Moving;
            }
            auto* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
            FNavLocation Point;
            const bool Projected = Nav && Nav->ProjectPointToNavigation(GetActorLocation()+FVector(0,0,100), Point, FVector(150,150,250));
            auto* Player = UGameplayStatics::GetPlayerPawn(this, 0);
            const FString Report = FString::Printf(TEXT("Standalone=%d Enemies=%d Controllers=%d Moving=%d Ready=%d NavProjection=%d Player=%s\nRESULT %s\n"), bSpawnAroundSpawner, Spawned.Num(), Controllers, Moving, IsReady(), Projected, Player ? *Player->GetActorLocation().ToString() : TEXT("None"), bSpawnAroundSpawner && Spawned.Num()==3 && Controllers==3 ? TEXT("PASS") : TEXT("FAIL"));
            UE_LOG(LogTemp, Display, TEXT("PROGRESS_ENEMY_TEST_SCENE %s"), *Report);
            FFileHelper::SaveStringToFile(Report, *(FPaths::ProjectContentDir()/TEXT("Blueprints/Agents/EnemyINProgress/TestSceneVerification.txt")));
            FPlatformMisc::RequestExit(false);
        }), 12.f, false);
    }
#endif
}
void AProgressEnemySpawner::Tick(float Delta)
{
    Super::Tick(Delta);
    Spawned.RemoveAll([](const TWeakObjectPtr<AProgressEnemyCharacter>& E) { return !E.IsValid() || E->State == EProgressEnemyState::Dead; });
    AliveCount = Spawned.Num();
    if (bAutoSpawn) SpawnPendingEnemies();
}
bool AProgressEnemySpawner::IsReady()
{
    if (!HasAuthority() || !EnemyClass) return false;
    if (bWaitForProceduralMap && !bSpawnAroundSpawner)
    {
        if (!IsValid(MapGenerator))
        {
            UClass* Class = LoadClass<AActor>(nullptr, TEXT("/Game/Blueprints/ProceduralMap/Map_Generator.Map_Generator_C"));
            MapGenerator = Class ? UGameplayStatics::GetActorOfClass(this, Class) : nullptr;
        }
        if (!MapGenerator || !ProgressEnemyReflection::Bool(MapGenerator, TEXT("MapComplete"))) return false;
    }
    auto* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    return Nav && Nav->GetDefaultNavDataInstance(FNavigationSystem::DontCreate) && !UNavigationSystemV1::IsNavigationBeingBuiltOrLocked(this);
}
bool AProgressEnemySpawner::FindSpawnLocation(AActor* Room, FVector& Location)
{
    auto* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    const auto* Defaults = EnemyClass->GetDefaultObject<AProgressEnemyCharacter>();
    const float Radius = Defaults->GetCapsuleComponent()->GetScaledCapsuleRadius();
    const float HalfHeight = Defaults->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
    const FBox Bounds = Room->GetComponentsBoundingBox(true);
    for (int32 Attempt = 0; Attempt < FMath::Max(1, AttemptsPerEnemy); ++Attempt)
    {
        const float Angle = Random.FRandRange(0, 2 * PI);
        const float Distance = FMath::Sqrt(Random.FRand()) * RoomSpawnRadius;
        const FVector Candidate = Room->GetActorLocation() + FVector(FMath::Cos(Angle)*Distance, FMath::Sin(Angle)*Distance, 100);
        FNavLocation Projected;
        if (!Nav->ProjectPointToNavigation(Candidate, Projected, FVector(120,120,250))) continue;
        if (Room != this && Bounds.IsValid && !Bounds.ExpandBy(25).IsInsideOrOn(Projected.Location)) continue;
        Location = Projected.Location + FVector(0,0,HalfHeight+5);
        if (Player && FVector::DistSquared(Location, Player->GetActorLocation()) < FMath::Square(MinimumPlayerDistance)) continue;
        bool TooClose = false;
        for (TActorIterator<AProgressEnemyCharacter> It(GetWorld()); It; ++It)
            if (It->State != EProgressEnemyState::Dead && FVector::DistSquared(Location, It->GetActorLocation()) < FMath::Square(MinimumEnemySeparation)) { TooClose = true; break; }
        if (TooClose) continue;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(ProgressEnemySpawn), false, this);
        if (GetWorld()->OverlapBlockingTestByChannel(Location, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(Radius, HalfHeight), Params)) continue;
        // Require the generated floor to connect to the player's navigable island.
        if (Player)
        {
            FNavLocation PlayerNav;
            if (!Nav->ProjectPointToNavigation(Player->GetActorLocation(), PlayerNav, FVector(150,150,250))) continue;
            if (!Nav->TestPathSync(FPathFindingQuery(nullptr, *Nav->GetDefaultNavDataInstance(), Projected.Location, PlayerNav.Location))) continue;
        }
        return true;
    }
    return false;
}
void AProgressEnemySpawner::SpawnPendingEnemies()
{
    if (!IsReady()) return;
    TArray<AActor*> Rooms;
    if (bSpawnAroundSpawner) Rooms.Add(this);
    else
    {
        UClass* RoomClass = LoadClass<AActor>(nullptr, TEXT("/Game/Blueprints/ProceduralMap/Rooms/Master_Room.Master_Room_C"));
        if (!RoomClass) return;
        UGameplayStatics::GetAllActorsOfClass(this, RoomClass, Rooms);
    }
    Rooms.Sort([](const AActor& A, const AActor& B) { return A.GetName() < B.GetName(); });
    AActor* StartRoom = Cast<AActor>(ProgressEnemyReflection::Object(MapGenerator, TEXT("SpawnRoom")));
    int32 SpawnBudget = FMath::Max(1, SpawnBudgetPerTick);
    AliveCount = 0;
    for (auto& Enemy : Spawned) if (Enemy.IsValid() && Enemy->State != EProgressEnemyState::Dead) ++AliveCount;
    for (AActor* Room : Rooms)
    {
        if (bSkipPlayerSpawnRoom && (Room == StartRoom || Room->GetClass()->GetName().Contains(TEXT("Spawn_Room")))) continue;
        const TWeakObjectPtr<AActor> Key(Room);
        if (!RoomQuota.Contains(Key)) RoomQuota.Add(Key, Random.RandRange(FMath::Max(0, MinimumPerRoom), FMath::Max(MinimumPerRoom, MaximumPerRoom)));
        int32& Count = RoomSpawnCount.FindOrAdd(Key);
        while (Count < RoomQuota[Key] && AliveCount < FMath::Max(0, MaximumAlive) && SpawnBudget > 0)
        {
            --SpawnBudget;
            FVector Location;
            if (!FindSpawnLocation(Room, Location)) break; // Retry on a later tick; no quota consumed.
            const FTransform Transform(FRotator(0, Random.FRandRange(-180,180), 0), Location);
            auto* Enemy = GetWorld()->SpawnActorDeferred<AProgressEnemyCharacter>(EnemyClass, Transform, this, nullptr, ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding);
            if (!Enemy) break;
            Enemy->Difficulty = Difficulty; // Available to construction scripts and BeginPlay.
            UGameplayStatics::FinishSpawningActor(Enemy, Transform);
            if (!IsValid(Enemy)) break;
            Spawned.Add(Enemy);
            ++Count; ++AliveCount;
            OnEnemySpawned.Broadcast(Enemy);
            if (bDrawDebug) DrawDebugSphere(GetWorld(), Location, 50, 12, FColor::Green, false, 5);
        }
        if (SpawnBudget <= 0 || AliveCount >= MaximumAlive) break;
    }
}
void AProgressEnemySpawner::SetDifficultyLevels(int32 Damage, int32 Block, int32 Health, int32 MovementSpeed, bool bUpdateExisting)
{
    Difficulty.Damage = FMath::Max(0, Damage); Difficulty.Block = FMath::Max(0, Block);
    Difficulty.Health = FMath::Max(0, Health); Difficulty.MovementSpeed = FMath::Max(0, MovementSpeed);
    if (bUpdateExisting) for (auto& Enemy : Spawned) if (Enemy.IsValid()) Enemy->ApplyDifficulty(Difficulty);
}
void AProgressEnemySpawner::ResetSpawner(bool bDestroySpawnedEnemies)
{
    if (!HasAuthority()) return;
    if (bDestroySpawnedEnemies) for (auto& Enemy : Spawned) if (Enemy.IsValid()) Enemy->Destroy();
    if (bDestroySpawnedEnemies) Spawned.Empty();
    RoomQuota.Empty(); RoomSpawnCount.Empty();
    Random.Initialize(Seed);
}
