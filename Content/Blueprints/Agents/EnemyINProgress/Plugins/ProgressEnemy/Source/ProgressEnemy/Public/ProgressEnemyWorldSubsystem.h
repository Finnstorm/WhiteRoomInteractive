#pragma once
#include "Subsystems/WorldSubsystem.h"
#include "Engine/DeveloperSettings.h"
#include "ProgressEnemyWorldSubsystem.generated.h"

UCLASS(Config=ProgressEnemy, DefaultConfig, meta=(DisplayName="Progress Enemy"))
class PROGRESSENEMY_API UProgressEnemySettings : public UDeveloperSettings
{
    GENERATED_BODY()
public:
    /** Automatically adds one spawner to the existing ProceduralDungeon map. A placed spawner takes priority. */
    UPROPERTY(Config, EditAnywhere, Category="Procedural Map") bool bAutoCreateDungeonSpawner = true;
};

UCLASS()
class PROGRESSENEMY_API UProgressEnemyWorldSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void OnWorldBeginPlay(UWorld& World) override;
protected:
    virtual bool DoesSupportWorldType(EWorldType::Type Type) const override;
};
