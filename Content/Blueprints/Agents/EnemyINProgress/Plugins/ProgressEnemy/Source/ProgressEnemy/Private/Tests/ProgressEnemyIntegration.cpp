#include "ProgressEnemyEditorLibrary.h"
#if WITH_EDITOR
#include "ProgressEnemyCharacter.h"
#include "ProgressEnemySpawner.h"
#include "ProgressEnemyReflection.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/WorldSettings.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/BoxComponent.h"
#endif

FString UProgressEnemyEditorLibrary::RunIntegrationChecks()
{
    FString Report;
#if WITH_EDITOR
    int32 Failures = 0;
    auto Check = [&](bool Passed, const TCHAR* Name)
    {
        Report += FString(Passed ? TEXT("PASS ") : TEXT("FAIL ")) + Name + TEXT("\n");
        if (!Passed) ++Failures;
    };
    UWorld::InitializationValues Values;
    Values.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(true).ShouldSimulatePhysics(false).SetTransactional(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("ProgressEnemyVerification"), nullptr, true, ERHIFeatureLevel::SM5, &Values);
    FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
    Context.SetCurrentWorld(World);
    UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
    World->SetGameInstance(GameInstance);
    Context.OwningGameInstance = GameInstance;
    World->GetWorldSettings()->DefaultGameMode = AGameModeBase::StaticClass();
    World->SetGameMode(FURL());
    World->InitializeActorsForPlay(FURL());
    World->BeginPlay();
    World->SetBegunPlay(true);
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto Spawn = [&](FVector Position)
    {
        return World->SpawnActor<AProgressEnemyCharacter>(AProgressEnemyCharacter::StaticClass(), Position, FRotator::ZeroRotator, Params);
    };
    auto* Player = Spawn(FVector(120,0,100));
    auto* PC = World->SpawnActor<APlayerController>();
    PC->SetPlayerState(World->SpawnActor<APlayerState>());
    PC->Possess(Player);
    Check(Player->IsPlayerControlled() && Player->CanBeDamaged(), TEXT("Test player is possessed and damageable"));
    Player->SetActorTickEnabled(false);
    auto* Enemy = Spawn(FVector(0,0,100));
    Enemy->BlockChance = 0;
    Check(Enemy->EnemyHealth && Enemy->EnemyCombat, TEXT("Shared health and combat components initialize"));
    Check(FMath::IsNearlyEqual(Enemy->CurrentHealth, 100), TEXT("Health initializes without old-enemy casts"));
    Enemy->NotifyPlayerDamage(Player);
    Check(Enemy->State == EProgressEnemyState::Chase && Enemy->Target == Player, TEXT("Player damage triggers chase"));
    Check(Enemy->TryAttack(EProgressEnemyAttack::Light), TEXT("Light melee starts in range"));
    Enemy->Tick(.5f);
    Check(FMath::IsNearlyEqual(Player->CurrentHealth, 88), TEXT("Light melee uses shared point damage once"));
    Enemy->Tick(1);
    Check(Enemy->TryAttack(EProgressEnemyAttack::Heavy), TEXT("Heavy melee uses same attack path"));
    Enemy->Tick(.7f);
    Check(FMath::IsNearlyEqual(Player->CurrentHealth, 63), TEXT("Repeated attack clears shared hit list and deals heavy damage"));
    Enemy->Tick(2);
    Enemy->SetAttackEnabled(EProgressEnemyAttack::Light, false);
    Check(!Enemy->TryAttack(EProgressEnemyAttack::Light), TEXT("Disabled light attack cannot start"));
    Enemy->TryAttack(EProgressEnemyAttack::Heavy);
    Enemy->SetAttackEnabled(EProgressEnemyAttack::Heavy, false);
    Enemy->SetAttackEnabled(EProgressEnemyAttack::Taser, false);
    const float BeforeCancel = Player->CurrentHealth;
    Enemy->Tick(.8f);
    Check(FMath::IsNearlyEqual(Player->CurrentHealth, BeforeCancel), TEXT("Disabling an attack cancels pending damage"));
    Enemy->NotifyPlayerDamage(Player);
    Check(Enemy->TryBlock(), TEXT("Block can start with charges"));
    FDamageEvent Damage;
    const float BeforeBlock = Enemy->CurrentHealth;
    for (int32 I=0; I<3; ++I) Enemy->TakeDamage(20, Damage, PC, Player);
    Check(FMath::IsNearlyEqual(Enemy->CurrentHealth, BeforeBlock-15), TEXT("Three block hits reduce damage by exactly 75 percent"));
    Check(Enemy->BlockHitsRemaining == 0 && !Enemy->TryBlock(), TEXT("Exhausted shield cannot block"));
    Enemy->Tick(6.1f);
    Check(Enemy->BlockHitsRemaining == 3, TEXT("Shield recharges after configured delay"));
    Enemy->bEnableBlock = false;
    Enemy->SetDifficultyLevels(2, 2, 3, 4);
    Check(FMath::IsNearlyEqual(Enemy->MaximumHealth, 160) && Enemy->GetMaximumBlockHits() == 5, TEXT("Independent health and block difficulty scaling"));
    Check(FMath::IsNearlyEqual(Enemy->CurrentHealth / Enemy->MaximumHealth, .85f), TEXT("Live difficulty changes preserve health percentage"));
    Check(FMath::IsNearlyEqual(Enemy->IntelligenceLevel,2.75f) && FMath::IsNearlyEqual(Enemy->DecisionInterval,.28125f),TEXT("Intelligence averages all four levels and scales decision delay"));
    Enemy->SetDifficultyLevels(10000,10000,10000,10000);
    Check(Enemy->IntelligenceAlpha==1 && FMath::IsNearlyEqual(Enemy->DecisionInterval,.1f) && FMath::IsNearlyEqual(Enemy->PathUpdateInterval,.15f),TEXT("Extreme intelligence respects decision and navigation limits"));
    Player->GetCharacterMovement()->Velocity=FVector(300,0,0);
    Check(FMath::IsNearlyZero(Enemy->GetHeavyAttackChance()),TEXT("Smart enemy avoids heavy attacks against moving targets"));
    Player->GetCharacterMovement()->Velocity=FVector::ZeroVector;
    Check(FMath::IsNearlyEqual(Enemy->GetHeavyAttackChance(),.8f),TEXT("Smart enemy favors heavy attacks on close stationary targets"));
    Check(Enemy->GetBlockDecisionChance()>Enemy->BlockChance,TEXT("Smart wounded enemy increases defensive priority"));
    Enemy->SetDifficultyLevels(-1,-1,-1,-1);
    Check(Enemy->IntelligenceLevel==0 && FMath::IsNearlyEqual(Enemy->DecisionInterval,.35f),TEXT("Negative levels restore base intelligence safely"));
    Check(Enemy->Difficulty.Damage == 0 && Enemy->Difficulty.Health == 0, TEXT("Negative difficulty values clamp to zero"));
    Player->SetActorLocation(FVector(500,0,100));
    Enemy->SetAttackEnabled(EProgressEnemyAttack::Taser, true);
    Enemy->NotifyPlayerDamage(Player);
    Check(Enemy->TryAttack(EProgressEnemyAttack::Taser), TEXT("First taser attack starts"));
    Enemy->Tick(.5f); Enemy->Tick(1.1f);
    Check(Enemy->TaserShotsRemaining == 1 && Enemy->TaserRechargeRemaining == 0, TEXT("First taser shot has no magazine cooldown"));
    Check(Enemy->TryAttack(EProgressEnemyAttack::Taser), TEXT("Second taser attack starts"));
    Enemy->Tick(.5f); Enemy->Tick(1.1f);
    Check(Enemy->TaserShotsRemaining == 0 && !Enemy->TryAttack(EProgressEnemyAttack::Taser), TEXT("Two taser shots exhaust magazine"));
    Enemy->Tick(6.1f);
    Check(Enemy->TaserShotsRemaining == 2, TEXT("Taser refills both shots after cooldown"));
    auto* Blind = Spawn(FVector(0,1000,100));
    Blind->SightRadius = 1;
    Blind->ProximityRadius = 250;
    Blind->ProximityNoticeSeconds = 2;
    Blind->bEnableTaser = false; Blind->bEnableLightMelee = false; Blind->bEnableHeavyMelee = false;
    Blind->bEnableBlock = false;
    Player->SetActorLocation(FVector(-100,1000,100));
    Blind->Tick(1);
    Check(Blind->State == EProgressEnemyState::Patrol, TEXT("Close player is not noticed immediately"));
    Player->SetActorLocation(FVector(-500,1000,100)); Blind->Tick(.1f);
    Player->SetActorLocation(FVector(-100,1000,100)); Blind->Tick(1.1f);
    Check(Blind->State == EProgressEnemyState::Patrol, TEXT("Leaving proximity resets notice time"));
    Blind->Tick(1);
    Check(Blind->State == EProgressEnemyState::Chase, TEXT("Continuous proximity triggers awareness outside vision cone"));
    Blind->bEnableTaser = false; Blind->bEnableLightMelee = false; Blind->bEnableHeavyMelee = false;
    Blind->ProximityRadius = 1;
    Blind->Tick(11);
    Check(Blind->State == EProgressEnemyState::Search, TEXT("Losing player transitions chase to search"));
    Blind->Tick(11);
    Check(Blind->State == EProgressEnemyState::Patrol, TEXT("Search timeout returns to patrol"));
    auto* Watcher = Spawn(FVector(0,2500,100));
    Watcher->bEnableTaser = false; Watcher->bEnableLightMelee = false; Watcher->bEnableHeavyMelee = false; Watcher->bEnableBlock = false;
    Player->SetActorLocation(FVector(500,2500,100));
    Watcher->Tick(.1f);
    Check(Watcher->Target == Player && Watcher->State == EProgressEnemyState::Chase, TEXT("Player in vision cone is detected immediately"));
    Watcher->SetDifficultyLevels(0,0,0,4);
    Watcher->Tick(.1f);
    Check(FMath::IsNearlyEqual(Watcher->GetCharacterMovement()->MaxWalkSpeed, 475.2f, .01f), TEXT("Movement difficulty scales chase speed independently"));
    Watcher->SetAttackEnabled(EProgressEnemyAttack::Taser, true);
    Player->SetActorLocation(FVector(120,2500,100));
    Check(Watcher->TryAttack(EProgressEnemyAttack::Taser), TEXT("Taser-only configuration can fight at close range"));
    Watcher->SetAttackEnabled(EProgressEnemyAttack::Taser, false);
    Watcher->SetAttackEnabled(EProgressEnemyAttack::Light, true);
    Watcher->SetDifficultyLevels(2,0,0,0);
    Player->ApplyDifficulty(FProgressEnemyDifficulty(), true);
    Check(Watcher->TryAttack(EProgressEnemyAttack::Light), TEXT("Scaled melee can start"));
    Watcher->Tick(.5f);
    Check(FMath::IsNearlyEqual(Player->CurrentHealth, 84.4f, .01f), TEXT("Damage difficulty scales point damage independently"));
    Watcher->Tick(1);
    Check(Watcher->TryAttack(EProgressEnemyAttack::Light), TEXT("Attack starts before target reaches cover"));
    auto* Wall = World->SpawnActor<AActor>();
    auto* Box = NewObject<UBoxComponent>(Wall);
    Wall->SetRootComponent(Box);
    Box->SetBoxExtent(FVector(10,100,120));
    Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Box->SetCollisionResponseToAllChannels(ECR_Block);
    Box->RegisterComponent();
    Wall->SetActorLocation(FVector(60,2500,100));
    const float BeforeCover = Player->CurrentHealth;
    Watcher->Tick(.5f);
    Check(FMath::IsNearlyEqual(Player->CurrentHealth, BeforeCover), TEXT("New cover during windup prevents damage through walls"));
    Wall->Destroy();
    Player->SetActorLocation(FVector(120,0,100));
    Enemy->TakeDamage(80, Damage, PC, Player);
    Check(Enemy->CanBeFinished(), TEXT("Low health enables finishers"));
    Enemy->BeginFinisher(Player);
    Check(Enemy->State == EProgressEnemyState::Finishing && Player->GetCharacterMovement()->MovementMode == MOVE_None, TEXT("Finisher locks combat and player movement"));
    Enemy->Destroy();
    Check(Player->GetCharacterMovement()->MovementMode != MOVE_None, TEXT("Interrupted finisher restores player movement"));
    auto* Spawner = World->SpawnActor<AProgressEnemySpawner>();
    Spawner->EnemyClass = AProgressEnemyCharacter::StaticClass();
    Spawner->bAutoSpawn = false;
    Spawner->SpawnPendingEnemies();
    Check(Spawner->AliveCount == 0, TEXT("Spawner waits when map/navigation is not ready"));
    for (int32 Level : {0,10})
    {
        Player->SetActorLocation(FVector(120,5000+Level*100,100));
        auto* DecisionEnemy=Spawn(FVector(0,5000+Level*100,100));
        DecisionEnemy->bEnableBlock=false; DecisionEnemy->bEnableHeavyMelee=false; DecisionEnemy->bEnableTaser=false;
        DecisionEnemy->SetDifficultyLevels(Level,Level,Level,Level);
        DecisionEnemy->NotifyPlayerDamage(Player);
        DecisionEnemy->Tick(.05f);
        Check(DecisionEnemy->State==EProgressEnemyState::Chase,TEXT("Newly alerted enemy waits for its decision delay"));
        DecisionEnemy->Tick(.06f);
        Check(DecisionEnemy->State==(Level==10 ? EProgressEnemyState::Attacking : EProgressEnemyState::Chase),TEXT("High intelligence attacks sooner than base intelligence"));
        if (Level==0) { DecisionEnemy->Tick(.25f); Check(DecisionEnemy->State==EProgressEnemyState::Attacking,TEXT("Base intelligence attacks after the longer delay")); }
        DecisionEnemy->Destroy();
    }
    World->EndPlay(EEndPlayReason::Quit);
    World->DestroyWorld(false);
    GEngine->DestroyWorldContext(World);
    Report += FString::Printf(TEXT("RESULT failures=%d\n"), Failures);
#endif
    return Report;
}
