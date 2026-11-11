#include "DungeonGuidanceComponent.h"
#include "DungeonCorridorNetworkComponent.h"
#include "ProceduralMeshComponent.h"
#include "Curve/PolygonIntersectionUtils.h"
#include "ConstrainedDelaunay2.h"
#include "PhysicsEngine/BodySetup.h"
#include "DrawDebugHelpers.h"
#include "Algo/Reverse.h"
#include "Components/ArrowComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Curves/CurveFloat.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Modules/ModuleManager.h"
#include "UObject/UnrealType.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, DungeonGuidance)
DEFINE_LOG_CATEGORY_STATIC(LogDungeonGuidance, Log, All);
namespace
{
UObject *ObjectField(UObject *O, FName Name)
{
    if (O)
        if (auto *P = FindFProperty<FObjectPropertyBase>(O->GetClass(), Name))
            return P->GetObjectPropertyValue_InContainer(O);
    return nullptr;
}
FProperty *Prefix(UStruct *S, const TCHAR *Name)
{
    for (TFieldIterator<FProperty> I(S); I; ++I)
        if (I->GetName().StartsWith(Name))
            return *I;
    return nullptr;
}
TArray<USceneComponent *> Exits(UObject *O, FName Name)
{
    TArray<USceneComponent *> R;
    if (O)
        if (auto *P = FindFProperty<FArrayProperty>(O->GetClass(), Name))
            if (auto *Inner = CastField<FObjectPropertyBase>(P->Inner))
            {
                FScriptArrayHelper H(P, P->ContainerPtrToValuePtr<void>(O));
                for (int I = 0; I < H.Num(); ++I)
                    if (auto *C = Cast<USceneComponent>(Inner->GetObjectPropertyValue(H.GetRawPtr(I))))
                        R.Add(C);
            }
    return R;
}
void RemoveExit(UObject *O, FName Name, USceneComponent *Exit)
{
    if (O)
        if (auto *P = FindFProperty<FArrayProperty>(O->GetClass(), Name))
            if (auto *Inner = CastField<FObjectPropertyBase>(P->Inner))
            {
                FScriptArrayHelper H(P, P->ContainerPtrToValuePtr<void>(O));
                for (int I = H.Num() - 1; I >= 0; --I)
                    if (Inner->GetObjectPropertyValue(H.GetRawPtr(I)) == Exit)
                        H.RemoveValues(I);
            }
}
void AddExit(UObject *O, USceneComponent *Exit)
{
    if (O)
        if (auto *P = FindFProperty<FArrayProperty>(O->GetClass(), TEXT("UsedExits")))
            if (auto *Inner = CastField<FObjectPropertyBase>(P->Inner))
            {
                FScriptArrayHelper H(P, P->ContainerPtrToValuePtr<void>(O));
                Inner->SetObjectPropertyValue(H.GetRawPtr(H.AddValue()), Exit);
            }
}
bool IsRoomBox(const UBoxComponent *C)
{
    for (auto *P = C; P; P = Cast<UBoxComponent>(P->GetAttachParent()))
        if (P->GetName().StartsWith(TEXT("OverlapFolder")))
            return true;
    for (auto *P = C->GetAttachParent(); P; P = P->GetAttachParent())
        if (P->GetName().StartsWith(TEXT("OverlapFolder")))
            return true;
    return false;
}
TArray<UBoxComponent *> RoomBoxes(AActor *R)
{
    TArray<UBoxComponent *> Out;
    if (R)
    {
        TInlineComponentArray<UBoxComponent *> All(R);
        for (auto *B : All)
            if (IsRoomBox(B))
                Out.Add(B);
    }
    return Out;
}
bool InsideRoom(AActor *R, FVector P, float Margin = 0, bool CheckHeight = true)
{
    for (auto *B : RoomBoxes(R))
    {
        FVector L = B->GetComponentTransform().InverseTransformPosition(P), E = B->GetUnscaledBoxExtent();
        FVector Scale=B->GetComponentScale().GetAbs();
        if (FMath::Abs(L.X) <= E.X + Margin/FMath::Max(.001,Scale.X) && FMath::Abs(L.Y) <= E.Y + Margin/FMath::Max(.001,Scale.Y) &&
            (!CheckHeight || FMath::Abs(L.Z) <= E.Z + 250/FMath::Max(.001,Scale.Z)))
            return true;
    }
    return false;
}
FVector FlatForward(USceneComponent *C)
{
    FVector V = C->GetForwardVector();
    V.Z = 0;
    return V.GetSafeNormal();
}
bool AxisDirection(FVector V, int &D)
{
    if (FMath::Abs(V.X) > .99)
    {
        D = V.X > 0 ? 0 : 2;
        return true;
    }
    if (FMath::Abs(V.Y) > .99)
    {
        D = V.Y > 0 ? 1 : 3;
        return true;
    }
    return false;
}
FVector Direction(int D)
{
    static const FVector Values[]{FVector(1, 0, 0), FVector(0, 1, 0), FVector(-1, 0, 0), FVector(0, -1, 0)};
    return Values[D & 3];
}
float Distance2D(FVector A, FVector B)
{
    return FVector::Dist2D(A, B);
}
} // namespace

FVector FDungeonGuideLink::PositionA() const
{
    return ConnectorPoints.Num() ? ConnectorPoints[0] : ExitA ? ExitA->GetComponentLocation() : FVector::ZeroVector;
}
FVector FDungeonGuideLink::PositionB() const
{
    return ConnectorPoints.Num() ? ConnectorPoints.Last() : ExitB ? ExitB->GetComponentLocation() : PositionA();
}
UDungeonGuidanceComponent::UDungeonGuidanceComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = .05f;
}
AActor *UDungeonGuidanceComponent::Player() const
{
    return PlayerOverride ? PlayerOverride.Get() : UGameplayStatics::GetPlayerPawn(this, 0);
}
AActor *UDungeonGuidanceComponent::ActiveObjective() const
{
    if (bExitObjective && ExitOverride)
        return ExitOverride;
    if (!bExitObjective && KeyOverride)
        return KeyOverride;
    UClass *C = bExitObjective ? ExitActorClass.Get() : KeyActorClass.Get();
    if (C)
        for (TActorIterator<AActor> I(GetWorld(), C); I; ++I)
            if (IsValid(*I))
                return *I;
    return nullptr;
}
FCollisionQueryParams UDungeonGuidanceComponent::QueryParams() const
{
    AActor *CurrentPlayer = Player();
    AActor *CurrentObjective = ActiveObjective();
    if (bQueryCacheValid && CachedPlayer.Get() == CurrentPlayer && CachedObjective.Get() == CurrentObjective)
        return CachedQuery;
    FCollisionQueryParams P(SCENE_QUERY_STAT(DungeonGuide), false);
    P.AddIgnoredActor(GetOwner());
    if (Player())
        P.AddIgnoredActor(Player());
    if (ActiveObjective())
        P.AddIgnoredActor(ActiveObjective());
    for (AActor *R : Rooms)
        if (R)
        {
            TInlineComponentArray<UPrimitiveComponent *> C(R);
            for (auto *X : C)
                if (X->GetName().StartsWith(TEXT("Overlap")) ||
                    (Cast<UBoxComponent>(X) && IsRoomBox(CastChecked<UBoxComponent>(X))))
                    P.AddIgnoredComponent(X);
        }
    for (AActor *A : DoorFrames)
        if (A)
            P.AddIgnoredActor(A);
    CachedQuery = P;
    CachedPlayer = CurrentPlayer;
    CachedObjective = CurrentObjective;
    bQueryCacheValid = true;
    return P;
}
void UDungeonGuidanceComponent::RefreshGraph()
{
    bQueryCacheValid = false;
    Rooms.Reset();
    Links.Reset();
    if (RoomBaseClass)
        for (TActorIterator<AActor> I(GetWorld(), RoomBaseClass); I; ++I)
            if (IsValid(*I))
                Rooms.Add(*I);
    for (AActor *R : Rooms)
        if (auto *P = FindFProperty<FArrayProperty>(R->GetClass(), TEXT("RoomCon")))
        {
            auto *S = CastField<FStructProperty>(P->Inner);
            if (!S)
                continue;
            auto *Prev = CastField<FObjectPropertyBase>(Prefix(S->Struct, TEXT("PrvsRoom_")));
            auto *Exit = CastField<FObjectPropertyBase>(Prefix(S->Struct, TEXT("ExitFromPrvs_")));
            if (!Prev || !Exit)
                continue;
            FScriptArrayHelper H(P, P->ContainerPtrToValuePtr<void>(R));
            for (int I = 0; I < H.Num(); ++I)
            {
                auto *A = Cast<AActor>(Prev->GetObjectPropertyValue_InContainer(H.GetRawPtr(I)));
                auto *E = Cast<USceneComponent>(Exit->GetObjectPropertyValue_InContainer(H.GetRawPtr(I)));
                if (A && E && A != R)
                {
                    FDungeonGuideLink L;
                    L.RoomA = A;
                    L.RoomB = R;
                    L.ExitA = E;
                    Links.Add(L);
                }
            }
        }
    Links.Append(ExtraLinks);
    for(const auto& L:ExtraLinks){if(L.RoomA)Rooms.AddUnique(L.RoomA);if(L.RoomB)Rooms.AddUnique(L.RoomB);}
}
void UDungeonGuidanceComponent::HideRoomExitMarkers()
{
    for (AActor *R : Rooms)
        if (R)
        {
            TInlineComponentArray<UArrowComponent *> C(R);
            for (auto *A : C)
                A->SetHiddenInGame(true);
        }
}
void UDungeonGuidanceComponent::BeforeCloseExits()
{
    if (bConnectionsProcessed)
        return;
    bConnectionsProcessed = true;
    RefreshGraph();
    HideRoomExitMarkers();
    if (auto* Network=GetOwner()->FindComponentByClass<UDungeonCorridorNetworkComponent>())
        Network->BuildNetwork(this);
    else if (bConnectUnusedExits)
        ConnectUnusedExits();
    RefreshGraph();
    UE_LOG(LogDungeonGuidance, Display, TEXT("Before sealing: rooms=%d links=%d tunnels=%d"), Rooms.Num(), Links.Num(),
           TunnelConnectionsMade);
}
void UDungeonGuidanceComponent::ClearVisuals()
{
    VisibleSmellSegments = 0;
    for (AActor *A : DoorFrames)
        if (IsValid(A))
            A->Destroy();
    DoorFrames.Reset();
    for (USplineMeshComponent *C : ScentMeshes)
        if (C)
            C->DestroyComponent();
    ScentMeshes.Reset();
    ActiveDoorCount = 0;
}
void UDungeonGuidanceComponent::SetObjective(bool Exit)
{
    ClearVisuals();
    bExitObjective = Exit;
    ObjectiveElapsed = 0;
    RepathElapsed = 0;
    bHasCalculatedRoute = false;
    bScentRouteComplete = false;
    ScentPrefixLastRoom = -1;
    LastScentRouteFailure.Reset();
    FloorRoute.Reset();
    RouteRooms.Reset();
    RouteDoors.Reset();
    RouteLinkIndices.Reset();
    PulseAge = 0;
    PulseAccumulator = 0;
    PulseHead = 0;
}
void UDungeonGuidanceComponent::StartGuidance()
{
    if (auto *GI = GetWorld()->GetGameInstance())
        if (auto *Flag = FindFProperty<FBoolProperty>(GI->GetClass(), TEXT("HasKey")))
            Flag->SetPropertyValue_InContainer(GI, false);
    RefreshGraph();
    HideRoomExitMarkers();
    SetObjective(false);
    bGuidanceStarted = true;
    SetComponentTickEnabled(true);
}
void UDungeonGuidanceComponent::NotifyKeyCollected()
{
    if (!bExitObjective)
        SetObjective(true);
}
void UDungeonGuidanceComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    ClearVisuals();
    Super::EndPlay(Reason);
}
AActor *UDungeonGuidanceComponent::RoomAtLocation(FVector L) const
{
    AActor *Best = nullptr;
    float Score = MAX_flt;
    for (AActor *R : Rooms)
        if (R)
        {
            if (InsideRoom(R, L))
                return R;
            float D = FVector::DistSquared2D(L, R->GetActorLocation());
            if (D < Score)
            {
                Score = D;
                Best = R;
            }
        } // A player in a connecting tunnel belongs to its closest endpoint.
    for (const auto &Link : ExtraLinks)
        for (int I = 1; I < Link.ConnectorPoints.Num(); ++I)
        {
            FVector Q = FMath::ClosestPointOnSegment(L, Link.ConnectorPoints[I - 1], Link.ConnectorPoints[I]);
            float D = FVector::DistSquared2D(L, Q);
            if (D < Score)
            {
                Score = D;
                Best = FVector::DistSquared2D(L, Link.PositionA()) < FVector::DistSquared2D(L, Link.PositionB())
                           ? Link.RoomA.Get()
                           : Link.RoomB.Get();
            }
        }
    return Best;
}
bool UDungeonGuidanceComponent::GroundAt(FVector P, FVector &G) const
{
    FHitResult H;
    auto Q = QueryParams();
    const FVector Up(0, 0, AgentMaxStepHeight + 25);
    if (GetWorld()->LineTraceSingleByChannel(H, P + Up, P - FVector(0, 0, AgentHalfHeight * 3 + 300), ECC_Visibility,
                                             Q) &&
        H.ImpactNormal.Z > .65)
    {
        G = H.ImpactPoint;
        return true;
    }
    return false;
}
bool UDungeonGuidanceComponent::WalkSegment(FVector A, FVector B) const
{
    auto Q = QueryParams();
    FVector Lift(0, 0, AgentHalfHeight + 3);
    FHitResult H;
    if (GetWorld()->SweepSingleByChannel(H, A + Lift, B + Lift, FQuat::Identity, ECC_Visibility,
                                         FCollisionShape::MakeCapsule(AgentRadius, AgentHalfHeight), Q))
        return false;
    int N = FMath::Max(1, FMath::CeilToInt(Distance2D(A, B) / FMath::Max(30.f, AgentRadius)));
    FVector Previous = A;
    for (int I = 1; I <= N; ++I)
    {
        FVector G;
        if (!GroundAt(FMath::Lerp(A, B, float(I) / N), G) || FMath::Abs(G.Z - Previous.Z) > AgentMaxStepHeight + 5)
            return false;
        Previous = G;
    }
    return true;
}
bool UDungeonGuidanceComponent::LocalWalkPath(FVector Start, FVector End, AActor *Room, TArray<FVector> &Out,
                                              bool AllowNearGoal) const
{
    FVector S, E;
    if (!GroundAt(Start, S) || !GroundAt(End, E))
    {
        UE_LOG(LogDungeonGuidance, Verbose, TEXT("No ground for local route %s to %s"), *Start.ToString(),
               *End.ToString());
        return false;
    }
    if (WalkSegment(S, E))
    {
        Out = {S, E};
        return true;
    }
    struct FCell
    {
        FIntPoint XY;
        FVector P;
        int Parent = -1;
        float Cost = 0;
        float F = 0;
        bool Closed = false;
    };
    TArray<FCell> Nodes;
    TMap<FIntPoint, int> Index;
    const float Grid = FMath::Max(40.f, RouteGridSize);
    auto Key = [&](FVector P) {
        return FIntPoint(FMath::RoundToInt((P.X - S.X) / Grid), FMath::RoundToInt((P.Y - S.Y) / Grid));
    };
    Nodes.Add({FIntPoint(0, 0), S, -1, 0, Distance2D(S, E), false});
    Index.Add(FIntPoint(0, 0), 0);
    int Found = -1;
    for (int Iter = 0; Iter < LocalRouteNodeBudget; ++Iter)
    {
        int Best = -1;
        float Score = MAX_flt;
        for (int I = 0; I < Nodes.Num(); ++I)
            if (!Nodes[I].Closed && Nodes[I].F < Score)
            {
                Score = Nodes[I].F;
                Best = I;
            }
        if (Best < 0)
            break;
        FCell C = Nodes[Best];
        Nodes[Best].Closed = true;
        if (Distance2D(C.P, E) < Grid * 1.5 && WalkSegment(C.P, E))
        {
            Found = Best;
            break;
        }
        if (AllowNearGoal && Distance2D(C.P, End) < 450)
        {
            FHitResult Hit;
            if (!GetWorld()->LineTraceSingleByChannel(Hit, C.P + FVector(0, 0, SmellHeightAboveFloor), End,
                                                      ECC_Visibility, QueryParams()))
            {
                Found = Best;
                E = C.P;
                break;
            }
        }
        for (int D = 0; D < 4; ++D)
        {
            FVector Candidate = C.P + Direction(D) * Grid;
            FVector G;
            if (Room && RoomBoxes(Room).Num() && !InsideRoom(Room, Candidate, Grid))
                continue;
            if (!GroundAt(Candidate, G) || FMath::Abs(G.Z - C.P.Z) > AgentMaxStepHeight || !WalkSegment(C.P, G))
                continue;
            auto K = Key(G);
            int *Existing = Index.Find(K);
            float Cost = C.Cost + FVector::Distance(C.P, G);
            if (Existing)
            {
                auto &Old = Nodes[*Existing];
                if (Cost < Old.Cost)
                {
                    Old.Cost = Cost;
                    Old.F = Cost + Distance2D(G, E);
                    Old.Parent = Best;
                    Old.Closed = false;
                }
            }
            else
            {
                int I = Nodes.Add({K, G, Best, Cost, Cost + Distance2D(G, E), false});
                Index.Add(K, I);
            }
            if (Nodes.Num() >= LocalRouteNodeBudget)
                break;
        }
        if (Nodes.Num() >= LocalRouteNodeBudget)
            break;
    }
    if (Found < 0)
    {
        float Nearest = MAX_flt;
        FVector Near = FVector::ZeroVector;
        for (const auto &N : Nodes)
            if (Distance2D(N.P, E) < Nearest)
            {
                Nearest = Distance2D(N.P, E);
                Near = N.P;
            }
        UE_LOG(LogDungeonGuidance, Verbose, TEXT("Local A* failed room=%s start=%s end=%s nodes=%d nearest=%.1f at %s"),
               *GetNameSafe(Room), *S.ToString(), *E.ToString(), Nodes.Num(), Nearest, *Near.ToString());
        return false;
    }
    TArray<FVector> Reverse;
    for (int I = Found; I >= 0; I = Nodes[I].Parent)
        Reverse.Add(Nodes[I].P);
    Algo::Reverse(Reverse);
    Reverse.Add(E);
    Out.Reset();
    for (int I = 0; I < Reverse.Num();)
    {
        Out.Add(Reverse[I]);
        int J = Reverse.Num() - 1;
        for (; J > I + 1; --J)
            if (WalkSegment(Reverse[I], Reverse[J]))
                break;
        I = J > I ? J : I + 1;
    }
    return true;
}
bool UDungeonGuidanceComponent::ScentSegmentClear(FVector A, FVector B) const
{
    FHitResult Hit;
    return !GetWorld()->SweepSingleByChannel(
        Hit, A, B, FQuat::Identity, ECC_Visibility,
        FCollisionShape::MakeSphere(FMath::Max(3.f, SmellRadius + SmellWaveAmplitude + 3)), QueryParams());
}
bool UDungeonGuidanceComponent::LocalScentPath(FVector Start, FVector End, AActor *Room, TArray<FVector> &Out,
                                               bool FinalGoal) const
{
    // A floating scent can travel above furniture that the capsule cannot step onto.
    // Every edge is swept, including vertical edges: it cannot escape through walls/ceilings.
    FVector GroundEnd = End;
    if (FinalGoal && !GroundAt(End, GroundEnd))
        return false;
    const FVector Height(0, 0, SmellHeightAboveFloor);
    FVector S = Start + Height, E = GroundEnd + Height;
    FBox2D ScentBounds(ForceInit);ScentBounds+=FVector2D(S);ScentBounds+=FVector2D(E);
    if(Room)for(auto* Box:RoomBoxes(Room)){auto Bounds=Box->Bounds.GetBox();ScentBounds+=FVector2D(Bounds.Min);ScentBounds+=FVector2D(Bounds.Max);}
    ScentBounds=ScentBounds.ExpandBy(FMath::Max(100.f,RouteGridSize));
    if (ScentSegmentClear(S, E))
    {
        Out = {S - Height, E - Height};
        return true;
    }
    struct FCell
    {
        FIntVector Key;
        FVector P;
        int Parent;
        float Cost, F;
        bool Closed;
    };
    TArray<FCell> N;
    TMap<FIntVector, int> Seen;
    float Grid = FMath::Max(50.f, RouteGridSize);
    N.Add({FIntVector::ZeroValue, S, -1, 0, float(FVector::Distance(S, E)), false});
    Seen.Add(FIntVector::ZeroValue, 0);
    int Found = -1;
    for (int Iter = 0; Iter < LocalRouteNodeBudget; ++Iter)
    {
        int Best = -1;
        float F = MAX_flt;
        for (int I = 0; I < N.Num(); ++I)
            if (!N[I].Closed && N[I].F < F)
            {
                Best = I;
                F = N[I].F;
            }
        if (Best < 0)
            break;
        auto C = N[Best];
        N[Best].Closed = true;
        if (FVector::Distance(C.P, E) < Grid * 1.8 && ScentSegmentClear(C.P, E))
        {
            Found = Best;
            break;
        }
        for (int D = 0; D < 27; ++D)
        {
            FVector Offset((D%3-1)*Grid,((D/3)%3-1)*Grid,(D/9-1)*Grid);
            if(Offset.IsNearlyZero()) continue;
            FVector P = C.P + Offset;
            if (P.Z < FMath::Min(S.Z, E.Z) - 30 || P.Z > FMath::Max(S.Z, E.Z) + SmellMaxHeightVariation)
                continue;
            // Placement boxes are often only a few centimetres tall. Their footprint
            // bounds the search; actual collision sweeps bound the floating trail vertically.
            if (!ScentBounds.IsInside(FVector2D(P)))
                continue;
            if (!ScentSegmentClear(C.P, P))
                continue;
            FIntVector K(FMath::RoundToInt((P.X - S.X) / Grid), FMath::RoundToInt((P.Y - S.Y) / Grid),
                         FMath::RoundToInt((P.Z - S.Z) / Grid));
            float Cost = C.Cost + Offset.Size() + FMath::Abs(Offset.Z)*.65f + FMath::Abs(P.Z-E.Z)*.06f;
            int *Old = Seen.Find(K);
            if (Old)
            {
                auto &V = N[*Old];
                if (Cost < V.Cost)
                {
                    V.Cost = Cost;
                    V.F = Cost + FVector::Distance(P, E);
                    V.Parent = Best;
                    V.Closed = false;
                }
            }
            else
            {
                int I = N.Add({K, P, Best, Cost, float(Cost + FVector::Distance(P, E)), false});
                Seen.Add(K, I);
            }
            if (N.Num() >= LocalRouteNodeBudget)
                break;
        }
        if (N.Num() >= LocalRouteNodeBudget)
            break;
    }
    if (Found < 0)
        return false;
    TArray<FVector> Reverse;
    for (int I = Found; I >= 0; I = N[I].Parent)
        Reverse.Add(N[I].P);
    Algo::Reverse(Reverse);
    Reverse.Add(E);
    Out.Reset();
    for (int I = 0; I < Reverse.Num();)
    {
        Out.Add(Reverse[I] - Height);
        int J = Reverse.Num() - 1;
        for (; J > I + 1; --J)
            if (ScentSegmentClear(Reverse[I], Reverse[J]))
                break;
        I = J > I ? J : I + 1;
    }
    return true;
}
bool UDungeonGuidanceComponent::RecalculateRoute()
{
    ClearVisuals();
    RefreshGraph();
    ++RouteBuildCount;
    bHasCalculatedRoute = true;
    RepathElapsed = 0;
    bScentRouteComplete = false;
    LastScentRouteFailure.Reset();
    ScentPrefixLastRoom = -1;
    FloorRoute.Reset();
    RouteRooms.Reset();
    RouteDoors.Reset();
    RouteDoorRotations.Reset();
    RouteDoorRooms.Reset();
    RouteLinkIndices.Reset();
    auto *P = Player();
    auto *Goal = ActiveObjective();
    if (!P || !Goal)
        return false;
    auto *Start = RoomAtLocation(P->GetActorLocation());
    auto *End = Cast<AActor>(ObjectField(GetOwner(), bExitObjective ? TEXT("ExitDoorRoom") : TEXT("EndRoom")));
    if (!End || (bExitObjective ? ExitOverride != nullptr : KeyOverride != nullptr))
        End = RoomAtLocation(Goal->GetActorLocation());
    if (!Start || !End)
        return false;
    // A weighted graph includes real rooms and corridor turns/junctions.
    // Calculate toward the objective, allowing a virtual start on a corridor edge.
    TMap<AActor *, int> ParentLink;
    TMap<AActor*,double> Distance;TSet<AActor*> Done;Distance.Add(End,0);ParentLink.Add(End,-1);
    for(int Pass=0;Pass<Rooms.Num();++Pass){AActor* Best=nullptr;double Cost=MAX_dbl;for(auto Pair:Distance)if(!Done.Contains(Pair.Key)&&Pair.Value<Cost){Best=Pair.Key;Cost=Pair.Value;}if(!Best)break;Done.Add(Best);for(int J=0;J<Links.Num();++J){auto& L=Links[J];AActor* Next=L.RoomA==Best?L.RoomB.Get():L.RoomB==Best?L.RoomA.Get():nullptr;if(!Next)continue;double Length=0;for(int K=1;K<L.ConnectorPoints.Num();++K)Length+=FVector::Distance(L.ConnectorPoints[K-1],L.ConnectorPoints[K]);if(Length<1)Length=FVector::Dist2D(L.RoomA->GetActorLocation(),L.RoomB->GetActorLocation());double NewCost=Cost+FMath::Max(1.,Length);if(!Distance.Contains(Next)||NewCost<Distance[Next]){Distance.Add(Next,NewCost);ParentLink.Add(Next,J);}}}
    FVector Current=P->GetActorLocation(),PlayerFloor;if(GroundAt(Current,PlayerFloor))Current=PlayerFloor;
    auto* Network=GetOwner()->FindComponentByClass<UDungeonCorridorNetworkComponent>();
    if(Network&&Network->ContainsPoint(P->GetActorLocation())){const FDungeonGuideLink* Chosen=nullptr;FVector Projection;double Near=MAX_dbl;for(const auto& L:Network->GraphLinks)if(!L.ExitA&&!L.ExitB&&L.ConnectorPoints.Num()==2){FVector Q=FMath::ClosestPointOnSegment(Current,L.PositionA(),L.PositionB());double D=FVector::DistSquared2D(Current,Q);if(D<Near){Chosen=&L;Projection=Q;Near=D;}}if(Chosen){double A=Distance.Contains(Chosen->RoomA)?Distance[Chosen->RoomA]+FVector::Dist(Current,Chosen->PositionA()):MAX_dbl;double B=Distance.Contains(Chosen->RoomB)?Distance[Chosen->RoomB]+FVector::Dist(Current,Chosen->PositionB()):MAX_dbl;Start=A<=B?Chosen->RoomA.Get():Chosen->RoomB.Get();FloorRoute.Add(Current);FloorRoute.Add(Projection);Current=A<=B?Chosen->PositionA():Chosen->PositionB();FloorRoute.Add(Current);}}
    if (!ParentLink.Contains(Start))
    {
        UE_LOG(LogDungeonGuidance, Warning, TEXT("No connected room route to objective"));
        return false;
    }
    TArray<int> Reverse;
    for (AActor *R = Start; R != End;)
    {
        int I = ParentLink[R];
        Reverse.Add(I);
        R = Links[I].RoomA == R ? Links[I].RoomB.Get() : Links[I].RoomA.Get();
    }
    RouteLinkIndices = Reverse;
    RouteRooms.Add(Start);
    AActor *Room = Start;
    bool ValidFloor = true;
    auto AppendLocal = [&](FVector Target, bool Final = false) {
        if (!ValidFloor)
        {
            Current = Target;
            return;
        }
        TArray<FVector> Segment;
        if ((LocalScentPath(Current, Target, Room, Segment, Final) ||
             LocalWalkPath(Current, Target, Room, Segment, Final)))
        {
            for (auto V : Segment)
                if (FloorRoute.IsEmpty() || !V.Equals(FloorRoute.Last(), 1))
                    FloorRoute.Add(V);
            ScentPrefixLastRoom = RouteRooms.Num() - 1;
        }
        else
        {
            ValidFloor = false;
            LastScentRouteFailure = FString::Printf(TEXT("Local route blocked in %s"), *GetNameSafe(Room));
            FHitResult StartHit,EndHit;auto Query=QueryParams();auto Shape=FCollisionShape::MakeSphere(SmellRadius+SmellWaveAmplitude+3);FVector Lift(0,0,SmellHeightAboveFloor);GetWorld()->SweepSingleByChannel(StartHit,Current+Lift,Current+Lift,FQuat::Identity,ECC_Visibility,Shape,Query);GetWorld()->SweepSingleByChannel(EndHit,Target+Lift,Target+Lift,FQuat::Identity,ECC_Visibility,Shape,Query);UE_LOG(LogDungeonGuidance,Verbose,TEXT("Scent endpoints %s -> %s; blocked start=%s end=%s"),*Current.ToString(),*Target.ToString(),*GetNameSafe(StartHit.GetComponent()),*GetNameSafe(EndHit.GetComponent()));
            UE_LOG(LogDungeonGuidance, Verbose, TEXT("Local path unavailable in %s; smell hidden"), *GetNameSafe(Room));
        }
        Current = Target;
    };
    TOptional<FVector> EntryApproach;
    for (int I : Reverse)
    {
        auto &L = Links[I];
        bool Forward = L.RoomA == Room;
        FVector Portal = Forward ? L.PositionA() : L.PositionB();
        float Offset=L.TunnelActor&&L.TunnelActor->ActorHasTag(TEXT("GeneratedCorridorNetwork"))?0:ExitMarkerHeightAboveFloor;
        auto *Exit = Forward ? L.ExitA.Get() : L.ExitB ? L.ExitB.Get() : L.ExitA.Get();
        if(EntryApproach.IsSet()){AppendLocal(EntryApproach.GetValue());EntryApproach.Reset();}
        FVector FloorPortal=Portal-FVector(0,0,Offset);
        if(Exit&&!Room->ActorHasTag(TEXT("CorridorNode"))&&FVector::Dist2D(Current,FloorPortal)>10)
            AppendLocal(FloorPortal+FlatForward(Exit)*(Exit->GetOwner()==Room?-150:150));
        AppendLocal(FloorPortal);
        if(Exit){RouteDoors.Add(Exit->GetComponentLocation());RouteDoorRotations.Add(Exit->GetComponentRotation());RouteDoorRooms.Add(RouteRooms.Num()-1);}
        if (L.ConnectorPoints.Num())
        {
            TArray<FVector> Line = L.ConnectorPoints;
            if (!Forward)
                Algo::Reverse(Line);
            if (ValidFloor)
                for (FVector V : Line)
                {
                V.Z -= Offset;
                    FloorRoute.Add(V);
                }
            if (ValidFloor)
                ScentPrefixLastRoom = RouteRooms.Num();
            Current = (Forward ? L.PositionB() : L.PositionA()) - FVector(0, 0, Offset);
            if (!L.PositionA().Equals(L.PositionB(), 10) && (Forward?L.ExitB!=nullptr:L.ExitA!=nullptr))
            {
                RouteDoors.Add(Forward ? L.PositionB() : L.PositionA());
                auto *FarExit = Forward ? L.ExitB.Get() : L.ExitA.Get();
                RouteDoorRotations.Add(FarExit ? FarExit->GetComponentRotation() : FRotator::ZeroRotator);
                RouteDoorRooms.Add(RouteRooms.Num() - 1);
            }
        }
        Room = Forward ? L.RoomB.Get() : L.RoomA.Get();
        RouteRooms.Add(Room);
        if(Exit&&!Room->ActorHasTag(TEXT("CorridorNode")))EntryApproach=Current+FlatForward(Exit)*(Exit->GetOwner()==Room?-150:150);
    }
    FVector GoalPoint = Goal->GetActorLocation();
    if (bExitObjective)
        GoalPoint -= Goal->GetActorForwardVector() * 100;
    if(EntryApproach.IsSet())AppendLocal(EntryApproach.GetValue());
    AppendLocal(GoalPoint, true);
    bScentRouteComplete = ValidFloor;
    if (FloorRoute.Num() < 2)
        FloorRoute.Reset();
    PulseHead = 0;
    PulseAge = 0;
    PulseAccumulator = 0;
    SpawnDoorFrames();
    return true;
}
void UDungeonGuidanceComponent::SpawnDoorFrames()
{
    if (!DoorFrameClass)
        return;
    for (int I = 0; I < RouteDoors.Num(); ++I)
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Params.Owner = GetOwner();
        auto *A = GetWorld()->SpawnActor<AActor>(DoorFrameClass, RouteDoors[I], RouteDoorRotations[I], Params);
        if (A)
        {
            A->SetActorEnableCollision(false);
            A->SetActorHiddenInGame(true);
        }
        DoorFrames.Add(A);
    }
}
void UDungeonGuidanceComponent::UpdateDoors(float Dt, int FirstRoom)
{
    PulseAge += Dt;
    PulseAccumulator += Dt;
    float Progress = FMath::Clamp(PulseAge / FMath::Max(.1f, PulseGrowthSeconds), 0.f, 1.f);
    float Growth = PulseWidthCurve ? PulseWidthCurve->GetFloatValue(Progress) : Progress;
    int Width = FMath::Clamp(1 + FMath::FloorToInt(FMath::Clamp(Growth, 0.f, 1.f) * (MaxSimultaneousDoors - 1)), 1,
                             FMath::Max(1, MaxSimultaneousDoors));
    float Step = FMath::Max(.05f, PulseStepSeconds * (PulseSpeedCurve ? PulseSpeedCurve->GetFloatValue(Progress) : 1));
    if (PulseAccumulator >= Step)
    {
        PulseAccumulator = FMath::Fmod(PulseAccumulator, Step);
        ++PulseHead;
    }
    int FirstDoor = 0;
    while (FirstDoor < RouteDoorRooms.Num() && RouteDoorRooms[FirstDoor] < FirstRoom)
        ++FirstDoor;
    // Trim by actual progress through a tunnel, including both of its doorway frames.
    if (FloorRoute.Num() > 1 && Player())
    {
        auto PathProgress = [&](FVector P, bool CheckVisibility) {
            double Best = MAX_dbl, Along = -1, Length = 0;
            for (int I = 1; I < FloorRoute.Num(); ++I)
            {
                FVector A = FloorRoute[I - 1], B = FloorRoute[I], Delta = B - A;
                double T =
                    FMath::Clamp(FVector::DotProduct(FVector(P.X - A.X, P.Y - A.Y, 0), FVector(Delta.X, Delta.Y, 0)) /
                                     FMath::Max(1., Delta.SizeSquared2D()),
                                 0., 1.);
                FVector Q = FMath::Lerp(A, B, T);
                double Distance = FVector::DistSquared2D(P, Q);
                if (Distance <= Best + .1 &&
                    (!CheckVisibility || ScentSegmentClear(P + FVector(0, 0, SmellHeightAboveFloor),
                                                           Q + FVector(0, 0, SmellHeightAboveFloor))))
                {
                    Best = Distance;
                    Along = Length + Delta.Size() * T;
                }
                Length += Delta.Size();
            }
            return Along;
        };
        FVector Ground;
        if (GroundAt(Player()->GetActorLocation(), Ground))
        {
            double PlayerProgress = PathProgress(Ground, true);
            if (PlayerProgress >= 0)
            {
                FirstDoor = 0;
                while (FirstDoor < RouteDoors.Num() &&
                       PathProgress(RouteDoors[FirstDoor], false) + 100 < PlayerProgress)
                    ++FirstDoor;
            }
        }
    }
    int Count = DoorFrames.Num() - FirstDoor;
    if (Count <= 0)
    {
        for (AActor *A : DoorFrames)
            if (A)
                A->SetActorHiddenInGame(true);
        ActiveDoorCount = 0;
        return;
    }
    int Head = PulseHead % (Count + Width);
    ActiveDoorCount = 0;
    for (int I = 0; I < DoorFrames.Num(); ++I)
        if (AActor *A = DoorFrames[I])
        {
            bool Lit = I >= FirstDoor && I - FirstDoor <= Head && I - FirstDoor > Head - Width;
            A->SetActorHiddenInGame(!Lit);
            ActiveDoorCount += Lit;
        }
}
void UDungeonGuidanceComponent::SetSmellStatus(const FString &Status)
{
    if (SmellStatus != Status)
    {
        SmellStatus = Status;
        UE_LOG(LogDungeonGuidance, Display, TEXT("Smell trail: %s"), *Status);
    }
}
void UDungeonGuidanceComponent::UpdateSmell(float Dt)
{
    VisibleSmellPath.Reset();
    SmellJoinElapsed+=Dt;
    VisibleSmellSegments = 0;
    for (USplineMeshComponent *C : ScentMeshes)
        if (C)
            C->SetHiddenInGame(true);
    if (ObjectiveElapsed < SmellHintDelaySeconds)
    {
        SetSmellStatus(TEXT("Waiting for smell delay"));
        return;
    }
    if (!Player())
    {
        SetSmellStatus(TEXT("No player"));
        return;
    }
    if (!ScentCylinder || !SmellMaterial)
    {
        SetSmellStatus(TEXT("Missing scent mesh or material"));
        return;
    }
    if (FloorRoute.Num() < 2)
    {
        SetSmellStatus(LastScentRouteFailure.IsEmpty() ? TEXT("No scent route available") : LastScentRouteFailure);
        return;
    }
    if (!bScentRouteComplete &&
        RouteRooms.IndexOfByKey(RoomAtLocation(Player()->GetActorLocation())) > ScentPrefixLastRoom)
    {
        SetSmellStatus(LastScentRouteFailure);
        return;
    }
    FVector Ground;
    if (!GroundAt(Player()->GetActorLocation(), Ground))
    {
        SetSmellStatus(TEXT("No floor detected below player"));
        return;
    }
    if(!bScentRouteComplete&&FloorRoute.Num()>1)
    {
        FVector Direction=(FloorRoute.Last()-FloorRoute[FloorRoute.Num()-2]).GetSafeNormal2D();
        if(FVector::DotProduct(Ground-FloorRoute.Last(),Direction)>AgentRadius)
        {SetSmellStatus(TEXT("Player passed verified trail prefix; waiting for route refresh"));return;}
    }
    FVector Start=Ground;
    if(SmellViewLeadDistance>0)
        if(auto* Pawn=Cast<APawn>(Player()))
            if(auto* PC=Cast<APlayerController>(Pawn->GetController()))
            {
                FVector Eye;FRotator View;PC->GetPlayerViewPoint(Eye,View);
                FVector Lead=Eye+View.Vector()*SmellViewLeadDistance+FRotationMatrix(View).GetUnitAxis(EAxis::Z)*SmellViewLeadHeight;
                FHitResult Hit;
                if(GetWorld()->SweepSingleByChannel(Hit,Eye,Lead,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(SmellRadius+SmellWaveAmplitude+5),QueryParams()))
                    Lead=FMath::Lerp(Eye,Lead,FMath::Max(0.f,Hit.Time-.06f));
                Start=Lead-FVector(0,0,SmellHeightAboveFloor);
            }

    const FVector Lift(0,0,SmellHeightAboveFloor);
    int Best=-1;double Distance=MAX_dbl;FVector Join;
    // Project the visible start, rather than the player, onto the route. This avoids a backwards loop.
    for(int I=0;I<FloorRoute.Num()-1;++I)
    {
        FVector Q=FMath::ClosestPointOnSegment(Start,FloorRoute[I],FloorRoute[I+1]);double D=FVector::DistSquared(Start,Q);
        if(D<Distance&&ScentSegmentClear(Start+Lift,Q+Lift)){Distance=D;Join=Q;Best=I;}
    }
    TArray<FVector> Path;
    if(Best>=0){Path={Start,Join};}
    else
    {
        // An obstacle separates the camera lead from the route: search a local join at a bounded rate.
        Distance=MAX_dbl;
        for(int I=0;I<FloorRoute.Num()-1;++I)
        {
            FVector Q=FMath::ClosestPointOnSegment(Ground,FloorRoute[I],FloorRoute[I+1]);double D=FVector::DistSquared(Ground,Q);
            if(D<Distance&&ScentSegmentClear(Ground+Lift,Q+Lift)){Distance=D;Join=Q;Best=I;}
        }
        if(Best<0){SetSmellStatus(TEXT("Cannot join cached trail without crossing geometry; waiting for route refresh"));return;}
        if(CachedSmellJoinVersion!=RouteBuildCount||SmellJoinElapsed>=FMath::Max(.05f,SmellLocalJoinRefreshSeconds))
        {
            SmellJoinElapsed=0;CachedSmellJoin.Reset();CachedSmellJoinVersion=RouteBuildCount;CachedSmellJoinSegment=Best;
            if(!LocalScentPath(Start,Join,RoomAtLocation(Player()->GetActorLocation()),CachedSmellJoin,false))
            {SetSmellStatus(TEXT("Cannot safely connect camera lead to route"));return;}
        }
        Path=CachedSmellJoin;Best=CachedSmellJoinSegment;
        if(Path.Num()<2||!ScentSegmentClear(Start+Lift,Path[1]+Lift)){SetSmellStatus(TEXT("Updating local scent join"));return;}
        Path[0]=Start;
    }
    for(int I=Best+1;I<FloorRoute.Num();++I)
        if(Path.IsEmpty()||!FloorRoute[I].Equals(Path.Last(),1))Path.Add(FloorRoute[I]);
    for(FVector Point:Path)VisibleSmellPath.Add(Point+Lift);
    float TotalLength = 0;
    for (int I = 1; I < Path.Num(); ++I)
        TotalLength += FVector::Distance(Path[I - 1], Path[I]);
    const int MaxSegments = 256;
    const float SegmentLength = FMath::Max(SmellSegmentLength, TotalLength / FMath::Max(1, MaxSegments - Path.Num()));
    int MeshIndex = 0;
    float Travel = 0;
    for (int I = 1; I < Path.Num() && MeshIndex < MaxSegments; ++I)
    {
        float L = FVector::Distance(Path[I - 1], Path[I]);
        if (L < 1)
            continue;
        int Count = FMath::Max(1, FMath::CeilToInt(L / FMath::Max(20.f, SegmentLength)));
        for (int J = 0; J < Count && MeshIndex < MaxSegments; ++J)
        {
            FVector A = FMath::Lerp(Path[I - 1], Path[I], float(J) / Count),
                    B = FMath::Lerp(Path[I - 1], Path[I], float(J + 1) / Count);
            A.Z += SmellHeightAboveFloor +
                   SmellWaveAmplitude * FMath::Sin(Travel / 100 + GetWorld()->GetTimeSeconds() * 2);
            Travel += L / Count;
            B.Z += SmellHeightAboveFloor +
                   SmellWaveAmplitude * FMath::Sin(Travel / 100 + GetWorld()->GetTimeSeconds() * 2);
            if (MeshIndex >= ScentMeshes.Num())
            {
                auto *C = NewObject<USplineMeshComponent>(GetOwner());
                C->SetMobility(EComponentMobility::Movable);
                C->SetStaticMesh(ScentCylinder);
                C->SetMaterial(0, SmellMaterial);
                C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                C->SetCastShadow(false);
                C->SetForwardAxis(ESplineMeshAxis::Z, false);
                C->SetStartScale(FVector2D(SmellRadius / 50), false);
                C->SetEndScale(FVector2D(SmellRadius / 50), false);
                C->RegisterComponent();
                ScentMeshes.Add(C);
            }
            USplineMeshComponent *C = ScentMeshes[MeshIndex++];
            C->SetStartAndEnd(A, B - A, B, B - A, true);
            C->SetStartScale(FVector2D(SmellRadius / 50));
            C->SetEndScale(FVector2D(SmellRadius / 50));
            C->SetHiddenInGame(false);
        }
    }
    VisibleSmellSegments = MeshIndex;
    SetSmellStatus(bScentRouteComplete ? TEXT("Visible") : TEXT("Visible: verified part of route"));
}
void UDungeonGuidanceComponent::TickComponent(float Dt, ELevelTick TickType, FActorComponentTickFunction *Function)
{
    Super::TickComponent(Dt, TickType, Function);
    if (!bGuidanceStarted || !Player())
        return;
    ObjectiveElapsed += Dt;
    RepathElapsed += Dt;
    float First = FMath::Min(DoorHintDelaySeconds, SmellHintDelaySeconds);
    if (ObjectiveElapsed >= First &&
        (!bHasCalculatedRoute || RepathElapsed >= FMath::Max(.1f, RouteRefreshCooldownSeconds)))
        RecalculateRoute();
    int RoomIndex = RouteRooms.IndexOfByKey(RoomAtLocation(Player()->GetActorLocation()));
    if(RoomIndex<0)if(auto* Network=GetOwner()->FindComponentByClass<UDungeonCorridorNetworkComponent>())if(Network->ContainsPoint(Player()->GetActorLocation())&&!FloorRoute.IsEmpty())RoomIndex=0;
    if (RoomIndex < 0)
    {
        for (AActor *A : DoorFrames)
            if (A)
                A->SetActorHiddenInGame(true);
        for (USplineMeshComponent *C : ScentMeshes)
            if (C)
                C->SetHiddenInGame(true);
        ActiveDoorCount = 0;
        VisibleSmellSegments = 0;
        SetSmellStatus(bHasCalculatedRoute ? TEXT("Player left cached route; waiting for route refresh cooldown")
                                           : TEXT("Waiting for first objective route"));
        return;
    }
    if (ObjectiveElapsed >= DoorHintDelaySeconds)
        UpdateDoors(Dt, RoomIndex);
    UpdateSmell(Dt);
}

bool UDungeonGuidanceComponent::TunnelSegmentClear(FVector A, FVector B, AActor *IgnoreA, AActor *IgnoreB) const
{
    const float Half = TunnelWidth * .5f + TunnelThickness;
    auto Q = QueryParams();
    FVector Center(0, 0, -ExitMarkerHeightAboveFloor + TunnelHeight * .5f);
    FHitResult Hit;
    FVector Extent(Half, Half, FMath::Max(30.f, TunnelHeight * .5f - 8));
    if (GetWorld()->SweepSingleByChannel(Hit, A + Center, B + Center, (B - A).Rotation().Quaternion(), ECC_Visibility,
                                         FCollisionShape::MakeBox(Extent), Q))
    {
        UE_LOG(LogDungeonGuidance, Verbose, TEXT("Tunnel blocked at %s by %s/%s impact=%s normal=%s"), *A.ToString(),
               *GetNameSafe(Hit.GetActor()), *GetNameSafe(Hit.GetComponent()), *Hit.ImpactPoint.ToString(),
               *Hit.ImpactNormal.ToString());
        return false;
    }
    int N = FMath::Max(1, FMath::CeilToInt(Distance2D(A, B) / FMath::Max(40.f, Half * .5f)));
    for (int I = 0; I <= N; ++I)
    {
        FVector P = FMath::Lerp(A, B, float(I) / N);
        for (AActor *R : Rooms)
            if (R && R != IgnoreA && R != IgnoreB)
                if (InsideRoom(R, P, Half))
                {
                    UE_LOG(LogDungeonGuidance, Verbose, TEXT("Tunnel room exclusion %s at %s"), *R->GetName(),
                           *P.ToString());
                    return false;
                }
    }
    return true;
}
bool UDungeonGuidanceComponent::FindTunnelRoute(USceneComponent *A, USceneComponent *B, TArray<FVector> &Out) const
{
    Out.Reset();
    if (!A || !B || A == B || A->GetOwner() == B->GetOwner())
        return false;
    const FVector Origin = A->GetComponentLocation();
    const FQuat Basis = FlatForward(A).Rotation().Quaternion();
    auto ToWorld = [&](FVector V) { return Basis.RotateVector(V) + Origin; };
    FVector Start = FVector::ZeroVector, End = Basis.UnrotateVector(B->GetComponentLocation() - Origin);
    if (FMath::Abs(End.Z) > MaxExitHeightDifference || Distance2D(Start, End) > MaxExitPairDistance)
        return false;
    int StartD = 0, EndD;
    if (!AxisDirection(Basis.UnrotateVector(-FlatForward(B)), EndD))
        return false;
    End.Z = 0;
    auto Clear = [&](FVector P, FVector Q, AActor *RA = nullptr, AActor *RB = nullptr) {
        return TunnelSegmentClear(ToWorld(P), ToWorld(Q), RA, RB);
    };
    float Stub = TunnelWidth * .5f + TunnelThickness + 35;
    FVector S = Start + Direction(StartD) * Stub, E = End - Direction(EndD) * Stub;
    // Nearby facing doorways need a bridge, without overshooting them with the normal port stubs.
    if (StartD == EndD && End.X >= 0 && FMath::Abs(End.Y) < 15 && Clear(Start, End, A->GetOwner(), B->GetOwner()))
    {
        Out = {ToWorld(Start), ToWorld(End)};
        return true;
    }
    if (!Clear(Start, S, A->GetOwner(), B->GetOwner()) || !Clear(E, End, A->GetOwner(), B->GetOwner()))
        return false;
    TArray<float> X{float(S.X), float(E.X)}, Y{float(S.Y), float(E.Y)};
    float Grid = FMath::Max(50.f, TunnelGridSize), Margin = FMath::Min(MaxExitPairDistance, Grid * 4);
    for (float V = FMath::Min(S.X, E.X) - Margin; V <= FMath::Max(S.X, E.X) + Margin; V += Grid)
        X.AddUnique(V);
    for (float V = FMath::Min(S.Y, E.Y) - Margin; V <= FMath::Max(S.Y, E.Y) + Margin; V += Grid)
        Y.AddUnique(V);
    X.Sort();
    Y.Sort();
    int SX = X.IndexOfByPredicate([&](float V) { return FMath::IsNearlyEqual(V, float(S.X), .1f); }),
        SY = Y.IndexOfByPredicate([&](float V) { return FMath::IsNearlyEqual(V, float(S.Y), .1f); });
    int EX = X.IndexOfByPredicate([&](float V) { return FMath::IsNearlyEqual(V, float(E.X), .1f); }),
        EY = Y.IndexOfByPredicate([&](float V) { return FMath::IsNearlyEqual(V, float(E.Y), .1f); });
    struct FNode
    {
        int X, Y, D, T, Parent;
        float Cost, F;
        bool Closed;
    };
    TArray<FNode> Nodes;
    TMap<int64, int> Seen;
    auto Key = [](int I, int J, int D, int T) { return (int64(I) << 32) | (int64(J) << 16) | (D << 8) | T; };
    auto Pos = [&](int I, int J) { return FVector(X[I], Y[J], Start.Z); };
    Nodes.Add({SX, SY, StartD, 0, -1, 0, Distance2D(S, E), false});
    Seen.Add(Key(SX, SY, StartD, 0), 0);
    int Found = -1;
    const int Turns = FMath::Clamp(MaxTunnelTurns, 0, 3);
    for (int Iter = 0; Iter < TunnelSearchNodeBudget; ++Iter)
    {
        int Best = -1;
        float Score = MAX_flt;
        for (int I = 0; I < Nodes.Num(); ++I)
            if (!Nodes[I].Closed && Nodes[I].F < Score)
            {
                Best = I;
                Score = Nodes[I].F;
            }
        if (Best < 0)
            break;
        FNode C = Nodes[Best];
        Nodes[Best].Closed = true;
        if (C.X == EX && C.Y == EY && C.T + (C.D != EndD) <= Turns && C.D != ((EndD + 2) & 3))
        {
            Found = Best;
            break;
        }
        for (int D = 0; D < 4; ++D)
        {
            if (D == ((C.D + 2) & 3))
                continue;
            int T = C.T + (D != C.D);
            if (T > Turns)
                continue;
            int I = C.X + (D == 0) - (D == 2), J = C.Y + (D == 1) - (D == 3);
            if (!X.IsValidIndex(I) || !Y.IsValidIndex(J))
                continue;
            FVector P = Pos(C.X, C.Y), N = Pos(I, J);
            if (!Clear(P, N, A->GetOwner(), B->GetOwner()))
                continue;
            float Cost = C.Cost + Distance2D(P, N) + (D != C.D ? Grid * .2f : 0);
            int64 K = Key(I, J, D, T);
            int *Existing = Seen.Find(K);
            if (Existing)
            {
                auto &Old = Nodes[*Existing];
                if (Cost < Old.Cost)
                {
                    Old.Cost = Cost;
                    Old.F = Cost + Distance2D(N, E);
                    Old.Parent = Best;
                    Old.Closed = false;
                }
            }
            else
            {
                int New = Nodes.Add({I, J, D, T, Best, Cost, Cost + Distance2D(N, E), false});
                Seen.Add(K, New);
            }
            if (Nodes.Num() >= TunnelSearchNodeBudget)
                break;
        }
        if (Nodes.Num() >= TunnelSearchNodeBudget)
            break;
    }
    if (Found < 0)
    {
        UE_LOG(LogDungeonGuidance, Verbose, TEXT("Tunnel search failed S=%s E=%s nodes=%d"), *S.ToString(),
               *E.ToString(), Nodes.Num());
        return false;
    }
    TArray<FVector> R;
    for (int I = Found; I >= 0; I = Nodes[I].Parent)
        R.Add(Pos(Nodes[I].X, Nodes[I].Y));
    Algo::Reverse(R);
    R.Insert(Start, 0);
    R.Add(End);
    for (int I = 0; I < R.Num(); ++I)
    {
        if (Out.Num() > 1)
        {
            FVector ADir = (Out.Last() - Out[Out.Num() - 2]).GetSafeNormal(),
                    BDir = (R[I] - Out.Last()).GetSafeNormal();
            if (FVector::DotProduct(ADir, BDir) > .999)
                Out.Pop();
        }
        if (Out.IsEmpty() || !Out.Last().Equals(R[I], 1))
            Out.Add(R[I]);
    }
    float Length = 0;
    for (int I = 1; I < Out.Num(); ++I)
        Length += Distance2D(Out[I - 1], Out[I]);
    for (auto &P : Out)
        P = ToWorld(P);
    return Length <= MaxExitPairDistance * 2;
}
AActor *UDungeonGuidanceComponent::BuildTunnel(const TArray<FVector> &WorldPoints, USceneComponent *A,
                                               USceneComponent *B)
{
    if (WorldPoints.Num() < 2)
        return nullptr;
    const FVector Origin = WorldPoints[0];
    const FQuat Rotation = (WorldPoints[1] - Origin).Rotation().Quaternion();
    TArray<FVector> Points;
    for (auto P : WorldPoints)
        Points.Add(Rotation.UnrotateVector(P - Origin));
    if (!TunnelCube || Points.Num() < 2)
        return nullptr;
    FActorSpawnParameters Params;
    Params.Owner = GetOwner();
    auto *Actor = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params);
    auto *Root = NewObject<USceneComponent>(Actor);
    Root->SetMobility(EComponentMobility::Static);
    Actor->SetRootComponent(Root);
    Root->RegisterComponent();
    Actor->Tags.Add(TEXT("GeneratedConnectionTunnel"));
    const float H = TunnelWidth * .5f, Z = Points[0].Z - ExitMarkerHeightAboveFloor, T = TunnelThickness;
    struct FRect
    {
        float X0, X1, Y0, Y1;
    };
    TArray<FRect> Rects;
    TArray<float> X, Y;
    for (int I = 1; I < Points.Num(); ++I)
    {
        auto P = Points[I - 1], Q = Points[I];
        if (Distance2D(P, Q) < 1)
            continue;
        FRect R;
        if (FMath::Abs(P.X - Q.X) < 1)
            R = {float(P.X - H), float(P.X + H), float(FMath::Min(P.Y, Q.Y)), float(FMath::Max(P.Y, Q.Y))};
        else
            R = {float(FMath::Min(P.X, Q.X)), float(FMath::Max(P.X, Q.X)), float(P.Y - H), float(P.Y + H)};
        Rects.Add(R);
    }
    for (int I = 1; I < Points.Num() - 1; ++I)
    {
        auto P = Points[I];
        Rects.Add({float(P.X - H), float(P.X + H), float(P.Y - H), float(P.Y + H)});
    }
    for (auto R : Rects)
    {
        X.AddUnique(R.X0);
        X.AddUnique(R.X1);
        Y.AddUnique(R.Y0);
        Y.AddUnique(R.Y1);
    }
    X.Sort();
    Y.Sort();
    auto Inside = [&](float PX, float PY) {
        for (auto R : Rects)
            if (PX > R.X0 - .1 && PX < R.X1 + .1 && PY > R.Y0 - .1 && PY < R.Y1 + .1)
                return true;
        return false;
    };
    auto Cube = [&](FVector P, FVector Size) {
        if (Size.X < .1 || Size.Y < .1)
            return;
        auto *C = NewObject<UStaticMeshComponent>(Actor);
        C->SetupAttachment(Root);
        C->SetStaticMesh(TunnelCube);
        if (TunnelMaterial)
            C->SetMaterial(0, TunnelMaterial);
        C->SetWorldLocationAndRotation(Rotation.RotateVector(P) + Origin, Rotation);
        C->SetWorldScale3D(Size / 100);
        C->SetCollisionProfileName(TEXT("BlockAll"));
        C->SetMobility(EComponentMobility::Static);
        C->RegisterComponent();
    };
    auto Port = [&](FVector P) {
        for (auto *Exit : {A, B})
        {
            FVector Delta = Rotation.RotateVector(P) + Origin - Exit->GetComponentLocation();
            Delta.Z = 0;
            if (FMath::Abs(FVector::DotProduct(Delta, FlatForward(Exit))) < T + 1 &&
                FMath::Abs(FVector::CrossProduct(Delta, FlatForward(Exit)).Z) < H + 1)
                return true;
        }
        return false;
    };
    for (int I = 0; I < X.Num() - 1; ++I)
        for (int J = 0; J < Y.Num() - 1; ++J)
        {
            float MX = (X[I] + X[I + 1]) * .5f, MY = (Y[J] + Y[J + 1]) * .5f, DX = X[I + 1] - X[I],
                  DY = Y[J + 1] - Y[J];
            if (!Inside(MX, MY))
                continue;
            Cube({MX, MY, Z - T * .5f}, {DX, DY, T});
            Cube({MX, MY, Z + TunnelHeight + T * .5f}, {DX, DY, T});
            for (int D = 0; D < 4; ++D)
            {
                FVector P(MX, MY, Z + TunnelHeight * .5f);
                if (D == 0)
                    P.X = X[I + 1];
                if (D == 2)
                    P.X = X[I];
                if (D == 1)
                    P.Y = Y[J + 1];
                if (D == 3)
                    P.Y = Y[J];
                FVector Sample = P + Direction(D) * .5f;
                if (!Inside(Sample.X, Sample.Y) && !Port(P))
                {
                    Cube(P + Direction(D) * (T * .5f),
                         D % 2 == 0 ? FVector(T, DY, TunnelHeight) : FVector(DX, T, TunnelHeight));
                }
            }
        }
    return Actor;
}
void UDungeonGuidanceComponent::ConnectUnusedExits()
{
    auto Available = Exits(GetOwner(), TEXT("ExitList"));
    auto *Reserved = Cast<USceneComponent>(ObjectField(GetOwner(), TEXT("ExitDoorExit")));
    struct FPair
    {
        USceneComponent *A;
        USceneComponent *B;
        float D;
    };
    TArray<FPair> Candidates;
    for (int I = 0; I < Available.Num(); ++I)
        for (int J = I + 1; J < Available.Num(); ++J)
        {
            auto *A = Available[I];
            auto *B = Available[J];
            if (!IsValid(A) || !IsValid(B) || A == Reserved || B == Reserved || A->GetOwner() == B->GetOwner())
                continue;
            float D = Distance2D(A->GetComponentLocation(), B->GetComponentLocation());
            if (D <= MaxExitPairDistance)
                Candidates.Add({A, B, D});
        }
    Candidates.Sort([](const FPair &A, const FPair &B) { return A.D < B.D; });
    TSet<USceneComponent *> Used;
    int Attempts = 0;
    for (auto Pair : Candidates)
    {
        if (TunnelConnectionsMade >= MaxTunnels || Attempts >= TunnelCandidatePairBudget)
            break;
        if (Used.Contains(Pair.A) || Used.Contains(Pair.B))
            continue;
        ++Attempts;
        TArray<FVector> Path;
        if (!FindTunnelRoute(Pair.A, Pair.B, Path))
            continue;
        auto *Tunnel = BuildTunnel(Path, Pair.A, Pair.B);
        if (!Tunnel)
            continue;
        FDungeonGuideLink L;
        L.RoomA = Pair.A->GetOwner();
        L.RoomB = Pair.B->GetOwner();
        L.ExitA = Pair.A;
        L.ExitB = Pair.B;
        L.ConnectorPoints = Path;
        L.TunnelActor = Tunnel;
        ExtraLinks.Add(L);
        for (auto *E : {Pair.A, Pair.B})
        {
            Used.Add(E);
            RemoveExit(GetOwner(), TEXT("ExitList"), E);
            AddExit(E->GetOwner(), E);
        }
        ++TunnelConnectionsMade;
        UE_LOG(LogDungeonGuidance, Display, TEXT("Connected unused exits: %s -> %s; corners=%d"),
               *Pair.A->GetPathName(), *Pair.B->GetPathName(), FMath::Max(0, Path.Num() - 2));
    }
    Links.RemoveAll([](const FDungeonGuideLink &L) { return L.TunnelActor != nullptr; });
    Links.Append(ExtraLinks);
}
#include "DungeonCorridorNetwork.inl"


