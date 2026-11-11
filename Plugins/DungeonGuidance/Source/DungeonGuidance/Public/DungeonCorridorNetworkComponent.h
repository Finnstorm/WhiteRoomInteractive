#pragma once
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "DungeonGuidanceComponent.h"
#include "DungeonCorridorNetworkComponent.generated.h"
USTRUCT(BlueprintType)
struct DUNGEONGUIDANCE_API FCorridorBranch
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category = "Corridors") TArray<FVector> Points;
    UPROPERTY(BlueprintReadOnly, Category = "Corridors") TObjectPtr<USceneComponent> PortA;
    UPROPERTY(BlueprintReadOnly, Category = "Corridors") TObjectPtr<USceneComponent> PortB;
};
UCLASS(ClassGroup = (Dungeon), meta = (BlueprintSpawnableComponent))
class DUNGEONGUIDANCE_API UDungeonCorridorNetworkComponent : public UActorComponent
{
    GENERATED_BODY()
  public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Corridors") bool bEnabled = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Corridors", meta = (ClampMin = "120")) float Width = 240;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Corridors", meta = (ClampMin = "200")) float Height = 280;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Corridors", meta = (ClampMin = "5")) float Thickness = 20;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Corridors", meta = (ClampMin = "0"))
    float CollisionMargin = 5;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Corridors", meta = (ClampMin = "50")) float DoorStub = 200;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Corridors", meta = (ClampMin = "0")) int32 MaxBranches = 8;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Corridors", meta = (ClampMin = "0")) int32 MaxBends = 3;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Corridors", meta = (ClampMin = "100"))
    float CandidateRange = 6000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Corridors", meta = (ClampMin = "100"))
    float MaxBranchLength = 10000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Corridors", meta = (ClampMin = "50")) float GridSize = 150;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Corridors", meta = (ClampMin = "100"))
    int32 SearchNodeBudget = 12000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Corridors", meta = (ClampMin = "1"))
    int32 CandidateBudget = 80;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Corridors", meta = (ClampMin = "0.05"))
    float SearchTimeBudgetSeconds = 2;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Corridors") bool bAllowSharedJunctions = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Corridors") bool bDebugDraw = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Corridors|Lighting") bool bAddLights=true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Corridors|Lighting", meta=(ClampMin="100")) float LightSpacing=800;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Corridors|Lighting", meta=(ClampMin="0")) float LightIntensity=4500;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Corridors|Lighting", meta=(ClampMin="100")) float LightRadius=1100;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Corridors|Lighting") FLinearColor LightColor=FLinearColor(1,.88,.72);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Corridors|Lighting", meta=(ClampMin="1")) int32 MaxLights=64;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Corridors|Doorway Seals") bool bSealDoorwayGaps=true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Corridors|Doorway Seals", meta=(ClampMin="100")) float DoorwaySealReach=800;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Corridors|Debug") int32 LightsPlaced=0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Corridors|Debug") int32 SealedDoorways=0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Corridors") TObjectPtr<UMaterialInterface> FloorMaterial;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Corridors") TObjectPtr<UMaterialInterface> WallMaterial;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Corridors") TObjectPtr<UMaterialInterface> CeilingMaterial;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Corridors", meta = (ClampMin = "10"))
    float UVRepeatDistance = 200;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Corridors|Debug")
    TArray<FCorridorBranch> Branches;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Corridors|Debug")
    TArray<TObjectPtr<AActor>> Nodes;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Corridors|Debug")
    TArray<FDungeonGuideLink> GraphLinks;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Corridors|Debug") int32 TJunctions = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Corridors|Debug") int32 XJunctions = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Corridors|Debug") FString BuildReport;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Corridors|Debug")
    TObjectPtr<AActor> GeometryActor;
    UFUNCTION(BlueprintCallable, Category = "Corridors") void BuildNetwork(UDungeonGuidanceComponent *Guide);
    UFUNCTION(BlueprintPure, Category = "Corridors") bool ContainsPoint(FVector Point) const;
    UFUNCTION(BlueprintCallable, Category = "Corridors")
    bool FindBranch(USceneComponent *A, USceneComponent *B, FVector Target, TArray<FVector> &Points) const;
    UFUNCTION(BlueprintCallable, Category = "Corridors") bool RebuildGeometryAndGraph(UDungeonGuidanceComponent *Guide);

  private:
    UPROPERTY(Transient) TObjectPtr<UDungeonGuidanceComponent> Guidance;
    TArray<TArray<FVector2D>> FootprintOuters;
    TArray<TArray<FVector2D>> FootprintHoles;
    double SearchDeadline = 0;
    bool SegmentClear(FVector A, FVector B, AActor *SourceRoom, AActor *TargetRoom) const;
};
