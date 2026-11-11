#pragma once
#include "CollisionQueryParams.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "DungeonGuidanceComponent.generated.h"

class UCurveFloat;
class UMaterialInterface;
class UStaticMesh;
class USplineMeshComponent;
class USceneComponent;

USTRUCT(BlueprintType)
struct DUNGEONGUIDANCE_API FDungeonGuideLink
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category = "Dungeon Guidance") TObjectPtr<AActor> RoomA = nullptr;
    UPROPERTY(BlueprintReadOnly, Category = "Dungeon Guidance") TObjectPtr<AActor> RoomB = nullptr;
    UPROPERTY(BlueprintReadOnly, Category = "Dungeon Guidance") TObjectPtr<USceneComponent> ExitA = nullptr;
    UPROPERTY(BlueprintReadOnly, Category = "Dungeon Guidance") TObjectPtr<USceneComponent> ExitB = nullptr;
    UPROPERTY(BlueprintReadOnly, Category = "Dungeon Guidance") TArray<FVector> ConnectorPoints;
    UPROPERTY(BlueprintReadOnly, Category = "Dungeon Guidance") TObjectPtr<AActor> TunnelActor = nullptr;
    FVector PositionA() const;
    FVector PositionB() const;
};

UCLASS(ClassGroup = (Dungeon), meta = (BlueprintSpawnableComponent))
class DUNGEONGUIDANCE_API UDungeonGuidanceComponent : public UActorComponent
{
    GENERATED_BODY()
  public:
    UDungeonGuidanceComponent();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guidance|Timing", meta = (ClampMin = "0"))
    float DoorHintDelaySeconds = 10;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guidance|Timing", meta = (ClampMin = "0"))
    float SmellHintDelaySeconds = 20;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guidance|Timing", meta = (ClampMin = "0.1"))
    float RouteRefreshCooldownSeconds = 30;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guidance|Door Pulse",
              meta = (ClampMin = "1", ClampMax = "32"))
    int32 MaxSimultaneousDoors = 5;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guidance|Door Pulse", meta = (ClampMin = "0.05"))
    float PulseStepSeconds = .3f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guidance|Door Pulse", meta = (ClampMin = "0.1"))
    float PulseGrowthSeconds = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guidance|Door Pulse")
    TObjectPtr<UCurveFloat> PulseWidthCurve;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guidance|Door Pulse")
    TObjectPtr<UCurveFloat> PulseSpeedCurve;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guidance|Smell", meta = (ClampMin = "1"))
    float SmellHeightAboveFloor = 220;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guidance|Smell", meta = (ClampMin = "0.1"))
    float SmellRadius = 2.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guidance|Smell", meta = (ClampMin = "0"))
    float SmellWaveAmplitude = 4;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guidance|Smell", meta = (ClampMin = "20"))
    float SmellSegmentLength = 150;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guidance|Smell", meta = (ClampMin = "0"))
    float SmellViewLeadDistance = 300;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guidance|Smell", meta = (ClampMin = "0"))
    float SmellViewLeadHeight = 35;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Guidance|Smell", meta=(ClampMin="0.05")) float SmellLocalJoinRefreshSeconds=.25;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Guidance|Debug") TArray<FVector> VisibleSmellPath;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Guidance|Smell", meta=(ClampMin="0")) float SmellMaxHeightVariation=100;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guidance|Smell")
    TObjectPtr<UMaterialInterface> SmellMaterial;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guidance|Routing", meta = (ClampMin = "40"))
    float RouteGridSize = 100;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guidance|Routing", meta = (ClampMin = "10"))
    float AgentRadius = 48;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guidance|Routing", meta = (ClampMin = "30"))
    float AgentHalfHeight = 96;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guidance|Routing", meta = (ClampMin = "0"))
    float AgentMaxStepHeight = 45;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guidance|Routing", meta = (ClampMin = "100"))
    int32 LocalRouteNodeBudget = 5000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guidance|Assets") TSubclassOf<AActor> RoomBaseClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guidance|Assets") TSubclassOf<AActor> KeyActorClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guidance|Assets") TSubclassOf<AActor> ExitActorClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guidance|Assets") TSubclassOf<AActor> DoorFrameClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guidance|Assets") TObjectPtr<UStaticMesh> ScentCylinder;
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Guidance|Overrides") TObjectPtr<AActor> PlayerOverride;
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Guidance|Overrides") TObjectPtr<AActor> KeyOverride;
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Guidance|Overrides") TObjectPtr<AActor> ExitOverride;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunnels") bool bConnectUnusedExits = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunnels", meta = (ClampMin = "0")) int32 MaxTunnels = 4;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunnels", meta = (ClampMin = "0"))
    float MaxExitPairDistance = 3500;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunnels", meta = (ClampMin = "0"))
    float MaxExitHeightDifference = 10;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunnels", meta = (ClampMin = "0", ClampMax = "3"))
    int32 MaxTunnelTurns = 3;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunnels", meta = (ClampMin = "120"))
    float TunnelWidth = 240;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunnels", meta = (ClampMin = "200"))
    float TunnelHeight = 280;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunnels", meta = (ClampMin = "5"))
    float TunnelThickness = 20;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunnels", meta = (ClampMin = "50"))
    float TunnelGridSize = 200;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunnels", meta = (ClampMin = "0"))
    float ExitMarkerHeightAboveFloor = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunnels", meta = (ClampMin = "1"))
    int32 TunnelCandidatePairBudget = 40;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunnels", meta = (ClampMin = "100"))
    int32 TunnelSearchNodeBudget = 5000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunnels") TObjectPtr<UStaticMesh> TunnelCube;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tunnels") TObjectPtr<UMaterialInterface> TunnelMaterial;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Guidance|Debug") bool bGuidanceStarted = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Guidance|Debug") bool bExitObjective = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Guidance|Debug") float ObjectiveElapsed = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Guidance|Debug") int32 RouteBuildCount = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Guidance|Debug") int32 ActiveDoorCount = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Guidance|Debug")
    int32 VisibleSmellSegments = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Guidance|Debug")
    FString SmellStatus = TEXT("Waiting for guidance start");
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Guidance|Debug")
    bool bScentRouteComplete = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Guidance|Debug")
    FString LastScentRouteFailure;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Guidance|Debug")
    int32 TunnelConnectionsMade = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Guidance|Debug")
    TArray<TObjectPtr<AActor>> Rooms;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Guidance|Debug")
    TArray<FDungeonGuideLink> Links;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Guidance|Debug")
    TArray<TObjectPtr<AActor>> RouteRooms;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Guidance|Debug") TArray<FVector> FloorRoute;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Guidance|Debug")
    TArray<TObjectPtr<AActor>> DoorFrames;

    UFUNCTION(BlueprintCallable, Category = "Dungeon Guidance") void BeforeCloseExits();
    UFUNCTION(BlueprintCallable, Category = "Dungeon Guidance") void StartGuidance();
    UFUNCTION(BlueprintCallable, Category = "Dungeon Guidance") void NotifyKeyCollected();
    UFUNCTION(BlueprintCallable, Category = "Dungeon Guidance") void RefreshGraph();
    UFUNCTION(BlueprintCallable, Category = "Dungeon Guidance") void HideRoomExitMarkers();
    UFUNCTION(BlueprintCallable, Category = "Dungeon Guidance") bool RecalculateRoute();
    UFUNCTION(BlueprintCallable, Category = "Dungeon Guidance") void ConnectUnusedExits();
    UFUNCTION(BlueprintCallable, Category = "Dungeon Guidance")
    bool FindTunnelRoute(USceneComponent *A, USceneComponent *B, TArray<FVector> &Points) const;
    UFUNCTION(BlueprintPure, Category = "Dungeon Guidance") AActor *RoomAtLocation(FVector Location) const;
    UFUNCTION(BlueprintPure, Category = "Dungeon Guidance") AActor *ActiveObjective() const;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                               FActorComponentTickFunction *TickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

  private:
    friend class UDungeonCorridorNetworkComponent;
    mutable FCollisionQueryParams CachedQuery;
    mutable bool bQueryCacheValid = false;
    mutable TWeakObjectPtr<AActor> CachedPlayer;
    mutable TWeakObjectPtr<AActor> CachedObjective;
    UPROPERTY(Transient) TArray<FDungeonGuideLink> ExtraLinks;
    UPROPERTY(Transient) TArray<TObjectPtr<USplineMeshComponent>> ScentMeshes;
    UPROPERTY(Transient) TArray<int32> RouteLinkIndices;
    UPROPERTY(Transient) TArray<FVector> RouteDoors;
    UPROPERTY(Transient) TArray<FRotator> RouteDoorRotations;
    UPROPERTY(Transient) TArray<int32> RouteDoorRooms;
    TArray<FVector> CachedSmellJoin;
    int32 CachedSmellJoinSegment=-1, CachedSmellJoinVersion=-1;
    float SmellJoinElapsed=1000;
    float RepathElapsed = 0;
    float PulseAge = 0;
    float PulseAccumulator = 0;
    int32 PulseHead = 0;
    bool bConnectionsProcessed = false;
    bool bHasCalculatedRoute = false;
    int32 ScentPrefixLastRoom = -1;
    void SetSmellStatus(const FString &Status);
    void ClearVisuals();
    void SetObjective(bool Exit);
    AActor *Player() const;
    FCollisionQueryParams QueryParams() const;
    bool WalkSegment(FVector A, FVector B) const;
    bool GroundAt(FVector Approximate, FVector &Ground) const;
    bool LocalWalkPath(FVector Start, FVector End, AActor *Room, TArray<FVector> &Result,
                       bool AllowNearGoal = false) const;
    bool LocalScentPath(FVector Start, FVector End, AActor *Room, TArray<FVector> &Result,
                        bool FinalGoal = false) const;
    bool ScentSegmentClear(FVector A, FVector B) const;
    bool TunnelSegmentClear(FVector A, FVector B, AActor *IgnoreRoomA = nullptr, AActor *IgnoreRoomB = nullptr) const;
    AActor *BuildTunnel(const TArray<FVector> &Points, USceneComponent *A, USceneComponent *B);
    void UpdateDoors(float Dt, int32 FirstRoom);
    void UpdateSmell(float Dt);
    void SpawnDoorFrames();
};
