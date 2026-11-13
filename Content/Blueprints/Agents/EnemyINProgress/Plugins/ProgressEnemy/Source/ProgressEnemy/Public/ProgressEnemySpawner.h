#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProgressEnemyTypes.h"
#include "ProgressEnemySpawner.generated.h"
class AProgressEnemyCharacter;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FProgressEnemySpawned, AProgressEnemyCharacter*, Enemy);

UCLASS(Blueprintable)
class PROGRESSENEMY_API AProgressEnemySpawner : public AActor
{
    GENERATED_BODY()
public:
    AProgressEnemySpawner();
    virtual void BeginPlay() override;
    virtual void Tick(float Delta) override;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner") TSubclassOf<AProgressEnemyCharacter> EnemyClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner") FProgressEnemyDifficulty Difficulty;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner") bool bAutoSpawn = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner") bool bWaitForProceduralMap = true;
    /** Standalone arena mode: use this actor as the spawn center without procedural rooms. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner") bool bSpawnAroundSpawner = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner") TObjectPtr<AActor> MapGenerator;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner", meta=(ClampMin="0")) int32 MinimumPerRoom = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner", meta=(ClampMin="0")) int32 MaximumPerRoom = 2;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner", meta=(ClampMin="0")) int32 MaximumAlive = 20;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner", meta=(ClampMin="0")) float MinimumPlayerDistance = 900;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner", meta=(ClampMin="0")) float MinimumEnemySeparation = 180;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner", meta=(ClampMin="1")) float RoomSpawnRadius = 650;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner", meta=(ClampMin="1")) int32 AttemptsPerEnemy = 12;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner", meta=(ClampMin="1")) int32 SpawnBudgetPerTick = 3;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner") int32 Seed = 12345;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner") bool bSkipPlayerSpawnRoom = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner") bool bDrawDebug = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spawner") int32 AliveCount = 0;
    UPROPERTY(BlueprintAssignable, Category="Spawner") FProgressEnemySpawned OnEnemySpawned;
    UFUNCTION(BlueprintCallable, Category="Spawner") void SetDifficultyLevels(int32 Damage, int32 Block, int32 Health, int32 MovementSpeed, bool bUpdateExisting = true);
    UFUNCTION(BlueprintCallable, Category="Spawner") void SpawnPendingEnemies();
    /** Destroy only enemies owned by this spawner, then allow a fresh wave. */
    UFUNCTION(BlueprintCallable, Category="Spawner") void ResetSpawner(bool bDestroySpawnedEnemies = true);
private:
    FRandomStream Random;
    TMap<TWeakObjectPtr<AActor>, int32> RoomQuota;
    TMap<TWeakObjectPtr<AActor>, int32> RoomSpawnCount;
    TArray<TWeakObjectPtr<AProgressEnemyCharacter>> Spawned;
    bool IsReady();
    bool FindSpawnLocation(AActor* Room, FVector& Location);
};
