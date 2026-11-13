#include "ProgressEnemyWorldSubsystem.h"
#include "ProgressEnemySpawner.h"
#include "EngineUtils.h"
#include "ProgressEnemyCharacter.h"
#include "ProgressEnemyReflection.h"
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/PlatformMisc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
bool UProgressEnemyWorldSubsystem::DoesSupportWorldType(EWorldType::Type Type) const
{
    return Type == EWorldType::Game || Type == EWorldType::PIE;
}
void UProgressEnemyWorldSubsystem::OnWorldBeginPlay(UWorld& World)
{
    Super::OnWorldBeginPlay(World);
    if (World.GetNetMode() == NM_Client || !GetDefault<UProgressEnemySettings>()->bAutoCreateDungeonSpawner || !World.GetMapName().Contains(TEXT("ProceduralDungeon"))) return;
    for (TActorIterator<AProgressEnemySpawner> It(&World); It; ++It) return;
    UClass* Class = LoadClass<AProgressEnemySpawner>(nullptr, TEXT("/Game/Blueprints/Agents/EnemyINProgress/BP_ProgressEnemySpawner.BP_ProgressEnemySpawner_C"));
    if (Class) World.SpawnActor<AProgressEnemySpawner>(Class);
#if !UE_BUILD_SHIPPING
    // Explicit opt-in smoke-test mode. Normal gameplay never exits or writes this report.
    if (FParse::Param(FCommandLine::Get(), TEXT("ProgressEnemyVerifyDungeon")))
    {
        FTimerHandle Handle;
        const TWeakObjectPtr<UWorld> WeakWorld(&World);
        World.GetTimerManager().SetTimer(Handle, [WeakWorld]()
        {
            UWorld* W = WeakWorld.Get();
            if (!W) return;
            int32 Enemies = 0, Spawners = 0, Controllers = 0, Moving = 0;
            bool Complete = false;
            for (TActorIterator<AProgressEnemySpawner> It(W); It; ++It)
            {
                ++Spawners;
                Complete |= ProgressEnemyReflection::Bool(It->MapGenerator, TEXT("MapComplete"));
            }
            for (TActorIterator<AProgressEnemyCharacter> It(W); It; ++It)
            {
                ++Enemies;
                if (It->GetController()) ++Controllers;
                if (It->GetVelocity().Size2D() > 5) ++Moving;
            }
            const FString Report = FString::Printf(TEXT("MapComplete=%d Spawners=%d Enemies=%d Controllers=%d Moving=%d NavigationBuilding=%d\nRESULT %s\n"), Complete, Spawners, Enemies, Controllers, Moving, UNavigationSystemV1::IsNavigationBeingBuiltOrLocked(W), Complete && Enemies>0 && Controllers==Enemies ? TEXT("PASS") : TEXT("FAIL"));
            UE_LOG(LogTemp, Display, TEXT("PROGRESS_ENEMY_DUNGEON %s"), *Report);
            FFileHelper::SaveStringToFile(Report, *(FPaths::ProjectContentDir()/TEXT("Blueprints/Agents/EnemyINProgress/DungeonVerification.txt")));
            FPlatformMisc::RequestExit(false);
        }, 20.f, false);
    }
#endif
}
