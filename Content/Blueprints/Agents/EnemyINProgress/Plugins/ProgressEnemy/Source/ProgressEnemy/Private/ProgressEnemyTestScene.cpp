#include "ProgressEnemyEditorLibrary.h"
#if WITH_EDITOR
#include "ProgressEnemySpawner.h"
#include "ProgressEnemyCharacter.h"
#include "Editor.h"
#include "FileHelpers.h"
#include "Builders/CubeBuilder.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavMesh/RecastNavMesh.h"
#include "NavigationSystem.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Subsystems/UnrealEditorSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/BrushComponent.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/GameModeBase.h"
#include "Model.h"
#include "Materials/MaterialInterface.h"
#include "EngineUtils.h"
#include "StaticMeshCompiler.h"
#endif

bool UProgressEnemyEditorLibrary::CreateTestScene()
{
#if WITH_EDITOR
    const FString Path = TEXT("/Game/Blueprints/Agents/EnemyINProgress/L_EnemyTest");
    UWorld* World = UEditorLoadingAndSavingUtils::NewBlankMap(false);
    if (!World) return false;
    auto* Floor = World->SpawnActor<AStaticMeshActor>(FVector(0,0,-25), FRotator::ZeroRotator);
    Floor->SetActorLabel(TEXT("Test Floor - 60m x 60m"));
    Floor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
    Floor->SetActorScale3D(FVector(60,60,.5f));
    Floor->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("BlockAll"));
    Floor->GetStaticMeshComponent()->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/LevelPrototyping/Materials/MI_PrototypeGrid_Gray.MI_PrototypeGrid_Gray")));
    auto* Start = World->SpawnActor<APlayerStart>(FVector(-1700,0,110), FRotator::ZeroRotator);
    Start->SetActorLabel(TEXT("Player Start"));
    auto* Sun = World->SpawnActor<ADirectionalLight>(FVector(0,0,1200), FRotator(-55,-30,0));
    Sun->SetActorLabel(TEXT("Sun"));
    Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    Sun->GetLightComponent()->SetIntensity(3);
    auto* Sky = World->SpawnActor<ASkyLight>();
    Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    Sky->GetLightComponent()->SetIntensity(1);
    Sky->GetLightComponent()->bRealTimeCapture = true;
    World->SpawnActor<ASkyAtmosphere>();
    auto* Bounds = World->SpawnActor<ANavMeshBoundsVolume>(FVector(0,0,250), FRotator::ZeroRotator);
    Bounds->SetActorLabel(TEXT("Navigation Bounds"));
    Bounds->Brush = NewObject<UModel>(Bounds, NAME_None, RF_Transactional);
    Bounds->Brush->Initialize(nullptr, true);
    auto* Builder = NewObject<UCubeBuilder>(Bounds, NAME_None, RF_Transactional);
    Builder->X = 5900; Builder->Y = 5900; Builder->Z = 1000;
    Bounds->BrushBuilder = Builder;
    if (!Builder->Build(World, Bounds)) return false;
    Bounds->GetBrushComponent()->Brush = Bounds->Brush;
    Bounds->GetBrushComponent()->BuildSimpleBrushCollision();
    Bounds->PostEditChange();
    Bounds->ReregisterAllComponents();
    UE_LOG(LogTemp, Display, TEXT("TEST_SCENE_BOUNDS %s"), *Bounds->GetComponentsBoundingBox(true).ToString());
    for (const TCHAR* ManagerPath : { TEXT("/Game/ShaderMats/Sanity.Sanity_C"), TEXT("/Game/ShaderMats/ImpactParticleManager.ImpactParticleManager_C") })
        if (UClass* ManagerClass = LoadClass<AActor>(nullptr, ManagerPath))
            World->SpawnActor<AActor>(ManagerClass)->SetActorLabel(FString(TEXT("Support - "))+ManagerClass->GetName());
    UClass* SpawnerClass = LoadClass<AProgressEnemySpawner>(nullptr, TEXT("/Game/Blueprints/Agents/EnemyINProgress/BP_ProgressEnemySpawner.BP_ProgressEnemySpawner_C"));
    if (!SpawnerClass) return false;
    auto* Spawner = World->SpawnActor<AProgressEnemySpawner>(SpawnerClass, FVector(500,0,0), FRotator::ZeroRotator);
    Spawner->SetActorLabel(TEXT("Enemy Spawner - 3 enemies"));
    Spawner->bSpawnAroundSpawner = true;
    Spawner->bWaitForProceduralMap = false;
    Spawner->MinimumPerRoom = 3;
    Spawner->MaximumPerRoom = 3;
    Spawner->MaximumAlive = 3;
    Spawner->RoomSpawnRadius = 1200;
    Spawner->MinimumPlayerDistance = 1000;
    Spawner->Seed = 12345;
    UClass* GameMode = LoadClass<AGameModeBase>(nullptr, TEXT("/Game/Blueprints/Objects/BP_FirstPersonGameMode.BP_FirstPersonGameMode_C"));
    World->GetWorldSettings()->DefaultGameMode = GameMode;
    World->UpdateWorldComponents(true, false);
    FNavigationSystem::AddNavigationSystemToWorld(*World, FNavigationSystemRunMode::EditorMode);
    if (auto* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
    {
        FStaticMeshCompilingManager::Get().FinishAllCompilation();
        Nav->OnWorldInitDone(FNavigationSystemRunMode::EditorMode);
        Floor->ReregisterAllComponents();
        FNavigationSystem::UpdateComponentData(*Floor->GetStaticMeshComponent());
        Nav->OnNavigationBoundsUpdated(Bounds);
        Nav->Tick(.1f);
        Nav->Build();
        if (auto* Recast = Cast<ARecastNavMesh>(Nav->GetDefaultNavDataInstance()))
            UE_LOG(LogTemp, Display, TEXT("TEST_SCENE_NAV Bounds=%s Tiles=%d FloorAffects=%d"), *Recast->GetNavMeshBounds().ToString(), Recast->GetNavMeshTilesCount(), int32(Floor->GetStaticMeshComponent()->CanEverAffectNavigation()));
    }
    GEditor->GetEditorSubsystem<UUnrealEditorSubsystem>()->SetLevelViewportCameraInfo(FVector(-3500,-3500,3000), FRotator(-30,45,0));
    return UEditorLoadingAndSavingUtils::SaveMap(World, Path);
#else
    return false;
#endif
}
