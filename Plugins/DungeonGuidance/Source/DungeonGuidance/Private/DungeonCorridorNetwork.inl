namespace
{
double Cross2(FVector2D A, FVector2D B)
{
    return A.X * B.Y - A.Y * B.X;
}
bool PolyContains(const TArray<FVector2D> &V, FVector2D P)
{
    bool In = false;
    for (int I = 0, J = V.Num() - 1; I < V.Num(); J = I++)
    {
        auto A = V[I], B = V[J];
        if ((A.Y > P.Y) != (B.Y > P.Y) && P.X < (B.X - A.X) * (P.Y - A.Y) / (B.Y - A.Y) + A.X)
            In = !In;
    }
    return In;
}
double PolylineLength(const TArray<FVector> &P)
{
    double L = 0;
    for (int I = 1; I < P.Num(); ++I)
        L += FVector::Dist(P[I - 1], P[I]);
    return L;
}
bool LineIntersection(FVector A, FVector B, FVector C, FVector D, double &T, double &U)
{
    FVector2D R(B - A), S(D - C), Delta(C - A);
    double Den = Cross2(R, S);
    if (FMath::Abs(Den) < .0001)
        return false;
    T = Cross2(Delta, S) / Den;
    U = Cross2(Delta, R) / Den;
    return T >= -.0001 && T <= 1.0001 && U >= -.0001 && U <= 1.0001;
}
} // namespace
bool UDungeonCorridorNetworkComponent::SegmentClear(FVector A, FVector B, AActor *SourceRoom, AActor *TargetRoom) const
{
    if (!Guidance || A.ContainsNaN() || B.ContainsNaN())
        return false;
    auto Q = Guidance->QueryParams();
    if (GeometryActor)
        Q.AddIgnoredActor(GeometryActor);
    FHitResult Hit;
    FVector Lift(0, 0, Height * .5f);
    const float Half = Width * .5f + Thickness + CollisionMargin;
    if (GetWorld()->SweepSingleByChannel(Hit, A + Lift, B + Lift, (B - A).Rotation().Quaternion(), ECC_Visibility,
                                         FCollisionShape::MakeBox(FVector(1, Half, Height * .5f - 5)), Q))
        return false;
    int Samples = FMath::Max(1, FMath::CeilToInt(FVector::Dist2D(A, B) / 75));
    for (int I = 0; I <= Samples; ++I)
    {
        FVector P = FMath::Lerp(A, B, double(I) / Samples);
        for (AActor *Room : Guidance->Rooms)
            if (Room && Room != SourceRoom && Room != TargetRoom && InsideRoom(Room, P, Half, false))
                return false;
    }
    return true;
}
bool UDungeonCorridorNetworkComponent::FindBranch(USceneComponent *A, USceneComponent *B, FVector Target,
                                                  TArray<FVector> &Out) const
{
    Out.Reset();
    if (!A || !Guidance)
        return false;
    FVector P = A->GetComponentLocation() - FVector(0, 0, Guidance->ExitMarkerHeightAboveFloor),
            End = B ? B->GetComponentLocation() - FVector(0, 0, Guidance->ExitMarkerHeightAboveFloor) : Target;
    if (FMath::Abs(P.Z - End.Z) > 1 || FVector::Dist2D(P, End) > CandidateRange)
        return false;
    End.Z = P.Z;
    auto *RA = A->GetOwner();
    auto *RB = B ? B->GetOwner() : nullptr;
    FVector FA = FlatForward(A), FB = B ? FlatForward(B) : FVector::ZeroVector;
    auto Clear = [&](FVector X, FVector Y) {
        return SegmentClear(X, Y, (X.Equals(P, 1) || Y.Equals(P, 1)) ? RA : nullptr,
                            (B && (X.Equals(End, 1) || Y.Equals(End, 1))) ? RB : nullptr);
    };
    auto Accessible = [&](USceneComponent *Port) {
        FVector Floor = Port->GetComponentLocation() - FVector(0, 0, Guidance->ExitMarkerHeightAboveFloor),
                F = FlatForward(Port), Lift(0, 0, Guidance->AgentHalfHeight + 3);
        FHitResult Hit;
        return !GetWorld()->SweepSingleByChannel(
            Hit, Floor - F * 150 + Lift, Floor + F * 50 + Lift, FQuat::Identity, ECC_Visibility,
            FCollisionShape::MakeCapsule(Guidance->AgentRadius, Guidance->AgentHalfHeight), Guidance->QueryParams());
    };
    if (!Accessible(A) || (B && !Accessible(B)))
        return false;
    auto Accept = [&](TArray<FVector> Points) {
        Out.Reset();
        for (auto V : Points)
        {
            if (Out.Num() > 1 &&
                (Out.Last() - Out[Out.Num() - 2]).GetSafeNormal().Equals((V - Out.Last()).GetSafeNormal(), .001))
                Out.Pop();
            if (Out.IsEmpty() || !V.Equals(Out.Last(), .1))
                Out.Add(V);
        }
        if (Out.Num() < 2 || Out.Num() - 2 > MaxBends || PolylineLength(Out) > MaxBranchLength)
            return false;
        for (int I = 1; I < Out.Num(); ++I)
            if (!Clear(Out[I - 1], Out[I]))
                return false;
        if (FVector::DotProduct((Out[1] - Out[0]).GetSafeNormal(), FA) < .9)
            return false;
        if (B && FVector::DotProduct((Out[Out.Num() - 2] - Out.Last()).GetSafeNormal(), FB) < .9)
            return false;
        // Footprints may join only where the logical centrelines also join.
        // In particular, close parallel corridors must not silently merge into an unregistered shortcut.
        for (const auto& Existing : Branches)
        {
            bool Touches=false, Crosses=false;
            for(int I=1;I<Out.Num();++I) for(int J=1;J<Existing.Points.Num();++J)
            {
                double T=0,U=0;
                Crosses |= LineIntersection(Out[I-1],Out[I],Existing.Points[J-1],Existing.Points[J],T,U);
                double D=FMath::Min(FMath::Min(FVector::Dist2D(Out[I-1],FMath::ClosestPointOnSegment(Out[I-1],Existing.Points[J-1],Existing.Points[J])),FVector::Dist2D(Out[I],FMath::ClosestPointOnSegment(Out[I],Existing.Points[J-1],Existing.Points[J]))),FMath::Min(FVector::Dist2D(Existing.Points[J-1],FMath::ClosestPointOnSegment(Existing.Points[J-1],Out[I-1],Out[I])),FVector::Dist2D(Existing.Points[J],FMath::ClosestPointOnSegment(Existing.Points[J],Out[I-1],Out[I]))));
                Touches |= D < Width+Thickness*2;
            }
            if((Touches&&!Crosses)||(!bAllowSharedJunctions&&(Touches||Crosses))) return false;
        }
        auto Q = Guidance->QueryParams();
        if (GeometryActor)
            Q.AddIgnoredActor(GeometryActor);
        for (int I = 1; I < Out.Num() - 1; ++I)
        {
            FVector Lift(0, 0, Height * .5f);
            float W = Width * .5f + Thickness + CollisionMargin;
            if (GetWorld()->OverlapBlockingTestByChannel(Out[I] + Lift, FQuat::Identity, ECC_Visibility,
                                                         FCollisionShape::MakeBox(FVector(W, W, Height * .5f - 5)), Q))
                return false;
        }
        return true;
    };
    // A short bridge must not overshoot its two doors with the normal outward stubs.
    if (FVector::DotProduct((End - P).GetSafeNormal(), FA) > .999 &&
        (!B || FVector::DotProduct((P - End).GetSafeNormal(), FB) > .999) && Clear(P, End))
        if (Accept({P, End}))
            return true;
    float StubA = DoorStub, StubB = DoorStub;
    const float Clearance = Width * .5f + Thickness + CollisionMargin;
    while (StubA < 1200 && InsideRoom(RA, P + FA * StubA, Clearance, false))
        StubA += 50;
    if (B)
        while (StubB < 1200 && InsideRoom(RB, End + FB * StubB, Clearance, false))
            StubB += 50;
    FVector S = P + FA * StubA, E = B ? End + FB * StubB : End;
    if (!Clear(P, S) || (B && !Clear(E, End)))
        return false;
    if (Clear(S, E) && Accept(B ? TArray<FVector>{P, S, E, End} : TArray<FVector>{P, S, End}))
        return true;
    struct FCell
    {
        FIntPoint K;
        FVector P;
        int Parent;
        double Cost, F;
        bool Closed;
    };
    TArray<FCell> N;
    TMap<FIntPoint, int> Seen;
    float Grid = FMath::Max(50.f, GridSize);
    N.Add({FIntPoint::ZeroValue, S, -1, 0, FVector::Dist2D(S, E), false});
    Seen.Add(FIntPoint::ZeroValue, 0);
    FBox2D Limit(ForceInit);
    Limit += FVector2D(S);
    Limit += FVector2D(E);
    Limit = Limit.ExpandBy(1600);
    int Found = -1;
    for (int Iter = 0; Iter < SearchNodeBudget; ++Iter)
    {
        if (SearchDeadline > 0 && FPlatformTime::Seconds() > SearchDeadline)
            break;
        int Best = -1;
        double Score = MAX_dbl;
        for (int I = 0; I < N.Num(); ++I)
            if (!N[I].Closed && N[I].F < Score)
            {
                Best = I;
                Score = N[I].F;
            }
        if (Best < 0)
            break;
        auto C = N[Best];
        N[Best].Closed = true;
        if (FVector::Dist2D(C.P, E) < Grid * 2 && Clear(C.P, E))
        {
            Found = Best;
            break;
        }
        for (int DX = -1; DX <= 1; ++DX)
            for (int DY = -1; DY <= 1; ++DY)
            {
                if (DX == 0 && DY == 0)
                    continue;
                FIntPoint K = C.K + FIntPoint(DX, DY);
                FVector V = S + FVector(K.X * Grid, K.Y * Grid, 0);
                if (!Limit.IsInside(FVector2D(V)) || !Clear(C.P, V))
                    continue;
                double Cost = C.Cost + FVector::Dist(C.P, V);
                if (C.Parent >= 0 &&
                    FVector::DotProduct((C.P - N[C.Parent].P).GetSafeNormal(), (V - C.P).GetSafeNormal()) < .99)
                    Cost += Grid * .15;
                if (auto *Old = Seen.Find(K))
                {
                    auto &Cell = N[*Old];
                    if (Cost < Cell.Cost)
                    {
                        Cell.Cost = Cost;
                        Cell.F = Cost + FVector::Dist2D(V, E);
                        Cell.Parent = Best;
                        Cell.Closed = false;
                    }
                }
                else
                    Seen.Add(K, N.Add({K, V, Best, Cost, Cost + FVector::Dist2D(V, E), false}));
                if (N.Num() >= SearchNodeBudget)
                    break;
            }
        if (N.Num() >= SearchNodeBudget)
            break;
    }
    if (Found < 0)
        return false;
    TArray<FVector> Raw;
    for (int I = Found; I >= 0; I = N[I].Parent)
        Raw.Add(N[I].P);
    Algo::Reverse(Raw);
    Raw.Add(E);
    TArray<FVector> Smooth{P};
    for (int I = 0; I < Raw.Num();)
    {
        Smooth.Add(Raw[I]);
        int J = Raw.Num() - 1;
        for (; J > I + 1; --J)
            if (Clear(Raw[I], Raw[J]))
                break;
        I = J > I ? J : I + 1;
    }
    if (B)
        Smooth.Add(End);
    return Accept(Smooth);
}
bool UDungeonCorridorNetworkComponent::ContainsPoint(FVector P) const
{
    if (Branches.IsEmpty() || FMath::Abs(P.Z - Branches[0].Points[0].Z) > Height + 200)
        return false;
    int Winding = 0;
    for (const auto &Poly : FootprintOuters)
        if (PolyContains(Poly, FVector2D(P)))
            ++Winding;
    for (const auto &Poly : FootprintHoles)
        if (PolyContains(Poly, FVector2D(P)))
            --Winding;
    return Winding > 0;
}
bool UDungeonCorridorNetworkComponent::RebuildGeometryAndGraph(UDungeonGuidanceComponent *Guide)
{
    Guidance = Guide;
    if (!Guide || Branches.IsEmpty())
        return false;
    using namespace UE::Geometry;
    TArray<FGeneralPolygon2d> Inputs, Union;
    float Half = Width * .5f;
    double FloorZ = Branches[0].Points[0].Z;
    auto Polygon = [&](TArray<FVector2D> Vertices) {
        FPolygon2d P(Vertices);
        if (P.IsClockwise())
            P.Reverse();
        Inputs.Add(FGeneralPolygon2d(P));
    };
    for (const auto &Branch : Branches)
    {
        for (int I = 1; I < Branch.Points.Num(); ++I)
        {
            FVector2D A(Branch.Points[I - 1]), B(Branch.Points[I]), D = (B - A).GetSafeNormal(), Side(-D.Y, D.X);
            if ((B - A).Size() < .1)
                continue;
            Polygon({A - Side * Half, B - Side * Half, B + Side * Half, A + Side * Half});
        }
        for (int I = 1; I < Branch.Points.Num() - 1; ++I)
        {
            TArray<FVector2D> V;
            for (int K = 0; K < 12; ++K)
            {
                double Angle = K * 2 * PI / 12;
                V.Add(FVector2D(Branch.Points[I]) + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Half);
            }
            Polygon(V);
        }
    }
    if (!PolygonsUnion(Inputs, Union, false) || Union.IsEmpty())
        return false;
    // Boolean unions can leave almost coincident vertices where a straight
    // section meets a bend. Use the same cleaned boundary for the slabs and
    // wall offsets, rather than creating zero-length wall strips at that seam.
    for (auto &P : Union)
        P.Simplify(.001, .001);
    struct FSection
    {
        TArray<FVector> V, N;
        TArray<int32> T;
        TArray<FVector2D> UV;
    };
    FSection Floor, Ceiling, Wall;
    float UV = FMath::Max(10.f, UVRepeatDistance);
    auto Tri = [&](FSection &S, FVector A, FVector B, FVector C) {
        int I = S.V.Num();
        FVector Cross = FVector::CrossProduct(B - A, C - A);
        if (Cross.SizeSquared() < 1.e-12)
            return;
        FVector N = Cross.GetSafeNormal();
        S.V.Append({A, B, C});
        S.N.Append({N, N, N});
        // Unreal renders clockwise fronts. Keep the intended surface normal,
        // but reverse the mathematical CCW winding used to construct each face.
        S.T.Append({I, I + 2, I + 1});
        for (auto P : {A, B, C})
            S.UV.Add(FVector2D(P.X + P.Z, P.Y + P.Z) / UV);
    };
    auto Quad = [&](FSection &S, FVector A, FVector B, FVector C, FVector D) {
        Tri(S, A, B, C);
        Tri(S, A, C, D);
    };
    TArray<USceneComponent *> Ports;
    for (const auto &Branch : Branches)
    {
        if (Branch.PortA)
            Ports.AddUnique(Branch.PortA);
        if (Branch.PortB)
            Ports.AddUnique(Branch.PortB);
    }
    auto AtPort = [&](FVector2D XY) {
        FVector P(XY, FloorZ);
        for (auto *Exit : Ports)
        {
            FVector Delta = P - Exit->GetComponentLocation();
            Delta.Z = 0;
            auto Forward = FlatForward(Exit);
            if (FMath::Abs(FVector::DotProduct(Delta, Forward)) < 2 &&
                FMath::Abs(FVector::CrossProduct(Delta, Forward).Z) < Half + 2)
                return true;
        }
        return false;
    };
    auto Boundary = [&](const FPolygon2d &P) {
        const auto &V = P.GetVertices();
        TArray<FVector2D> Outer;
        for (int I = 0; I < V.Num(); ++I)
        {
            FVector2D Before = (V[I] - V[(I + V.Num() - 1) % V.Num()]).GetSafeNormal();
            FVector2D After = (V[(I + 1) % V.Num()] - V[I]).GetSafeNormal();
            FVector2D N0(Before.Y, -Before.X), N1(After.Y, -After.X);
            FVector2D Miter = (N0 + N1) * (Thickness / FMath::Max(.001, 1. + FVector2D::DotProduct(N0, N1)));
            // A shared, bounded vertex closes both sides of every bend, including
            // the rounded bend facets and concave T/X junction boundaries.
            if (Miter.Size() > Thickness * 4)
                Miter = Miter.GetSafeNormal() * Thickness * 4;
            Outer.Add(V[I] + Miter);
        }
        struct FWallPiece
        {
            FVector2D A, B, OA, OB;
            bool Open;
        };
        TArray<FWallPiece> Pieces;
        for (int I = 0; I < V.Num(); ++I)
        {
            auto A = V[I], B = V[(I + 1) % V.Num()];
            Quad(Floor, FVector(A, FloorZ), FVector(A, FloorZ - Thickness), FVector(B, FloorZ - Thickness),
                 FVector(B, FloorZ));
            Quad(Ceiling, FVector(A, FloorZ + Height), FVector(B, FloorZ + Height),
                 FVector(B, FloorZ + Height + Thickness), FVector(A, FloorZ + Height + Thickness));
            TArray<double> Cuts{0, 1};
            for (auto *Port : Ports)
            {
                FVector F = FlatForward(Port), Side(-F.Y, F.X, 0);
                for (float Sign : {-1.f, 1.f})
                {
                    FVector2D Point(Port->GetComponentLocation() + Side * Half * Sign);
                    double T = FVector2D::DotProduct(Point - A, B - A) / FMath::Max(.01, (B - A).SizeSquared());
                    if (T > 0 && T < 1 && (Point - FMath::Lerp(A, B, T)).Size() < 2)
                        Cuts.Add(T);
                }
            }
            Cuts.Sort();
            for (int K = 1; K < Cuts.Num(); ++K)
            {
                auto X = FMath::Lerp(A, B, Cuts[K - 1]), Y = FMath::Lerp(A, B, Cuts[K]);
                if ((Y - X).SizeSquared() < 1.e-10)
                    continue;
                Pieces.Add({X, Y, FMath::Lerp(Outer[I], Outer[(I + 1) % V.Num()], Cuts[K - 1]),
                            FMath::Lerp(Outer[I], Outer[(I + 1) % V.Num()], Cuts[K]), AtPort((X + Y) * .5)});
            }
        }
        for (int I = 0; I < Pieces.Num(); ++I)
        {
            const auto &Piece = Pieces[I];
            if (Piece.Open)
                continue;
            FVector L(Piece.A, FloorZ), R(Piece.B, FloorZ), OL(Piece.OA, FloorZ), OR(Piece.OB, FloorZ);
            FVector Up(0, 0, Height);
            Quad(Wall, L, L + Up, R + Up, R);
            Quad(Wall, OL, OR, OR + Up, OL + Up);
            Quad(Wall, L + Up, OL + Up, OR + Up, R + Up);
            Quad(Wall, L, R, OR, OL);
            if (Pieces[(I + Pieces.Num() - 1) % Pieces.Num()].Open)
                Quad(Wall, L, OL, OL + Up, L + Up);
            if (Pieces[(I + 1) % Pieces.Num()].Open)
                Quad(Wall, R, R + Up, OR + Up, OR);
        }
    };
    for (const auto &P : Union)
    {
        TConstrainedDelaunay2<double> CDT;
        CDT.bOutputCCW = true;
        CDT.Add(P);
        if (!CDT.Triangulate() || CDT.Triangles.IsEmpty())
            return false;
        for (auto T : CDT.Triangles)
        {
            FVector A(CDT.Vertices[T.A], FloorZ), B(CDT.Vertices[T.B], FloorZ), C(CDT.Vertices[T.C], FloorZ);
            Tri(Floor, A, B, C);
            Tri(Floor, C - FVector(0, 0, Thickness), B - FVector(0, 0, Thickness), A - FVector(0, 0, Thickness));
            Tri(Ceiling, C + FVector(0, 0, Height), B + FVector(0, 0, Height), A + FVector(0, 0, Height));
            Tri(Ceiling, A + FVector(0, 0, Height + Thickness), B + FVector(0, 0, Height + Thickness),
                C + FVector(0, 0, Height + Thickness));
        }
        Boundary(P.GetOuter());
        for (const auto &Hole : P.GetHoles())
            Boundary(Hole);
    }
    SealedDoorways=0;
    if(bSealDoorwayGaps)for(auto* Port:Ports)
    {
        const FVector P=Port->GetComponentLocation()-FVector(0,0,Guide->ExitMarkerHeightAboveFloor),F=FlatForward(Port),Side(-F.Y,F.X,0);
        auto Q=Guide->QueryParams(); if(GeometryActor)Q.AddIgnoredActor(GeometryActor);
        auto Box=[&](FVector Center,FVector Extent)
        {
            FVector X=F*Extent.X,Y=Side*Extent.Y,Z(0,0,Extent.Z);
            // These box corners were originally ordered inward; preserve their
            // visible exterior while giving them proper outward normals too.
            auto Face=[&](FVector A,FVector B,FVector C,FVector D){Quad(Wall,D,C,B,A);};
            Face(Center-X-Y-Z,Center-X+Y-Z,Center-X+Y+Z,Center-X-Y+Z);
            Face(Center+X+Y-Z,Center+X-Y-Z,Center+X-Y+Z,Center+X+Y+Z);
            Face(Center+X-Y-Z,Center-X-Y-Z,Center-X-Y+Z,Center+X-Y+Z);
            Face(Center-X+Y-Z,Center+X+Y-Z,Center+X+Y+Z,Center-X+Y+Z);
            Face(Center-X-Y+Z,Center-X+Y+Z,Center+X+Y+Z,Center+X-Y+Z);
            Face(Center-X+Y-Z,Center-X-Y-Z,Center+X-Y-Z,Center+X+Y-Z);
        };
        float Left=Half+80,Right=Half+80,Top=Height+40;
        for(int Sign:{-1,1})
        {
            float Best=Half;bool Found=false;
            for(float Depth:{-30.f,0.f,20.f})for(float Z:{80.f,150.f,220.f})
            {
                FHitResult H;FVector From=P+F*Depth+FVector(0,0,Z);
                if(GetWorld()->LineTraceSingleByChannel(H,From,From+Side*Sign*DoorwaySealReach,ECC_Visibility,Q)&&H.GetActor()==Port->GetOwner()&&FMath::Abs(FVector::DotProduct(H.ImpactNormal,Side))>.4f&&H.Distance>=Half-5)
                {Best=FMath::Max(Best,H.Distance);Found=true;}
            }
            if(Found){if(Sign<0)Left=Best+Thickness;else Right=Best+Thickness;}
        }
        FHitResult H;FVector From=P-F*10+FVector(0,0,Height-10);
        if(GetWorld()->LineTraceSingleByChannel(H,From,P-F*10+FVector(0,0,900),ECC_Visibility,Q)&&H.GetActor()==Port->GetOwner()&&H.ImpactNormal.Z<-.5f)Top=FMath::Max(Height,float(H.ImpactPoint.Z-P.Z))+Thickness;
        for(int Sign:{-1,1}){float Reach=Sign<0?Left:Right,W=FMath::Max(0.f,Reach-Half);if(W>1)Box(P-F*10+Side*Sign*(Half+W*.5f)+FVector(0,0,Top*.5f),FVector(Thickness*2,W*.5f,Top*.5f));}
        if(Top>Height+1)Box(P-F*10+FVector(0,0,(Height+Top)*.5f),FVector(Thickness*2,Half,(Top-Height)*.5f));
        ++SealedDoorways;
    }
    FActorSpawnParameters Params;
    Params.Owner = GetOwner();
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto *NewActor = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params);
    NewActor->Tags.Add(TEXT("GeneratedCorridorNetwork"));
    auto *Root = NewObject<USceneComponent>(NewActor);
    Root->SetMobility(EComponentMobility::Static);
    NewActor->SetRootComponent(Root);
    Root->RegisterComponent();
    auto *Mesh = NewObject<UProceduralMeshComponent>(NewActor);
    Mesh->SetupAttachment(Root);
    Mesh->SetMobility(EComponentMobility::Static);
    Mesh->bUseAsyncCooking = false;
    Mesh->bUseComplexAsSimpleCollision = true;
    Mesh->SetCollisionProfileName(TEXT("BlockAll"));
    Mesh->RegisterComponent();
    int Section = 0;
    for (auto *S : {&Floor, &Wall, &Ceiling})
    {
        Mesh->CreateMeshSection_LinearColor(Section, S->V, S->T, S->N, S->UV, TArray<FLinearColor>(),
                                            TArray<FProcMeshTangent>(), true);
        UMaterialInterface *Material = Section == 0   ? FloorMaterial.Get()
                                       : Section == 1 ? WallMaterial.Get()
                                                      : CeilingMaterial.Get();
        if (!Material)
            Material = Guide->TunnelMaterial;
        if (Material)
            Mesh->SetMaterial(Section, Material);
        Section++;
    }
    LightsPlaced=0;
    if(bAddLights)
    {
        TArray<FVector> Positions;
        for(const auto& Branch:Branches)
        {
            const float Length=PolylineLength(Branch.Points);int Count=FMath::Max(1,FMath::CeilToInt(Length/FMath::Max(100.f,LightSpacing)));
            for(int I=0;I<Count&&LightsPlaced<MaxLights;++I)
            {
                float Along=Length*(I+.5f)/Count;FVector P=Branch.Points[0];
                for(int J=1;J<Branch.Points.Num();++J){float L=FVector::Distance(Branch.Points[J-1],Branch.Points[J]);if(Along<=L){P=FMath::Lerp(Branch.Points[J-1],Branch.Points[J],Along/FMath::Max(1.f,L));break;}Along-=L;}
                bool Near=false;for(FVector Other:Positions)if(FVector::Dist2D(P,Other)<LightSpacing*.55f)Near=true;
                if(Near)continue;
                Positions.Add(P);auto* Light=NewObject<UPointLightComponent>(NewActor);Light->SetupAttachment(Root);Light->SetMobility(EComponentMobility::Movable);Light->SetWorldLocation(P+FVector(0,0,Height-35));Light->SetIntensityUnits(ELightUnits::Lumens);Light->SetIntensity(LightIntensity);Light->SetAttenuationRadius(LightRadius);Light->SetLightColor(LightColor);Light->SetCastShadows(false);Light->RegisterComponent();++LightsPlaced;
                if(Guide->TunnelCube){auto* Fixture=NewObject<UStaticMeshComponent>(NewActor);Fixture->SetupAttachment(Root);Fixture->SetMobility(EComponentMobility::Static);Fixture->SetStaticMesh(Guide->TunnelCube);Fixture->SetRelativeLocation(P+FVector(0,0,Height-5));Fixture->SetRelativeScale3D(FVector(.45,.18,.04));Fixture->SetCollisionEnabled(ECollisionEnabled::NoCollision);Fixture->RegisterComponent();}
            }
        }
    }
    if (auto *Body = Mesh->GetBodySetup())
        Body->bDoubleSidedGeometry = true;
    Mesh->RecreatePhysicsState();
    if (GeometryActor)
        GeometryActor->Destroy();
    for (AActor *Node : Nodes)
        if (Node)
            Node->Destroy();
    Nodes.Reset();
    GraphLinks.Reset();
    GeometryActor = NewActor;
    FootprintOuters.Reset();
    FootprintHoles.Reset();
    for (const auto &P : Union)
    {
        FootprintOuters.Add(P.GetOuter().GetVertices());
        for (const auto &Hole : P.GetHoles())
            FootprintHoles.Add(Hole.GetVertices());
    }
    // Split every centreline at turns, T contacts and X crossings before graph registration.
    struct FEdge
    {
        FVector A, B;
        TArray<double> Cuts;
    };
    TArray<FEdge> Edges;
    for (const auto &Branch : Branches)
        for (int I = 1; I < Branch.Points.Num(); ++I)
            if (!Branch.Points[I - 1].Equals(Branch.Points[I], .1))
                Edges.Add({Branch.Points[I - 1], Branch.Points[I], {0, 1}});
    for (int I = 0; I < Edges.Num(); ++I)
        for (int J = I + 1; J < Edges.Num(); ++J)
        {
            double T, U;
            if (LineIntersection(Edges[I].A, Edges[I].B, Edges[J].A, Edges[J].B, T, U))
            {
                Edges[I].Cuts.Add(FMath::Clamp(T, 0., 1.));
                Edges[J].Cuts.Add(FMath::Clamp(U, 0., 1.));
            }
            else
            {
                for (auto P : {Edges[J].A, Edges[J].B})
                {
                    FVector Q = FMath::ClosestPointOnSegment(P, Edges[I].A, Edges[I].B);
                    if (P.Equals(Q, 1))
                        Edges[I].Cuts.Add(FVector::Dist(Edges[I].A, Q) / FVector::Dist(Edges[I].A, Edges[I].B));
                }
                for (auto P : {Edges[I].A, Edges[I].B})
                {
                    FVector Q = FMath::ClosestPointOnSegment(P, Edges[J].A, Edges[J].B);
                    if (P.Equals(Q, 1))
                        Edges[J].Cuts.Add(FVector::Dist(Edges[J].A, Q) / FVector::Dist(Edges[J].A, Edges[J].B));
                }
            }
        }
    auto NodeAt = [&](FVector P) -> AActor * {
        for (AActor *N : Nodes)
            if (N->GetActorLocation().Equals(P, 1))
                return N;
        auto *N = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params);
        N->Tags.Add(TEXT("CorridorNode"));
        auto *C = NewObject<USceneComponent>(N);
        N->SetRootComponent(C);
        C->RegisterComponent();
        N->SetActorLocation(P);
        Nodes.Add(N);
        return N;
    };
    TSet<FString> Added;
    for (auto &E : Edges)
    {
        E.Cuts.Sort();
        for (int I = 1; I < E.Cuts.Num(); ++I)
        {
            if (E.Cuts[I] - E.Cuts[I - 1] < .0001)
                continue;
            auto A = FMath::Lerp(E.A, E.B, E.Cuts[I - 1]), B = FMath::Lerp(E.A, E.B, E.Cuts[I]);
            auto *NA = NodeAt(A);
            auto *NB = NodeAt(B);
            int IA = Nodes.IndexOfByKey(NA), IB = Nodes.IndexOfByKey(NB);
            FString Key = FString::Printf(TEXT("%d/%d"), FMath::Min(IA, IB), FMath::Max(IA, IB));
            if (Added.Contains(Key))
                continue;
            Added.Add(Key);
            FDungeonGuideLink L;
            L.RoomA = NA;
            L.RoomB = NB;
            L.ConnectorPoints = {A, B};
            L.TunnelActor = NewActor;
            GraphLinks.Add(L);
        }
    }
    for (auto *Port : Ports)
    {
        auto P = Port->GetComponentLocation() - FVector(0, 0, Guide->ExitMarkerHeightAboveFloor);
        FDungeonGuideLink L;
        L.RoomA = Port->GetOwner();
        L.RoomB = NodeAt(P);
        L.ExitA = Port;
        L.ExitB = Port;
        L.ConnectorPoints = {P, P};
        L.TunnelActor = NewActor;
        GraphLinks.Add(L);
    }
    TJunctions = 0;
    XJunctions = 0;
    for (AActor *Node : Nodes)
    {
        int Degree = 0;
        for (const auto &L : GraphLinks)
            if (L.RoomA == Node || L.RoomB == Node)
                Degree++;
        if (Degree == 3)
            TJunctions++;
        if (Degree >= 4)
            XJunctions++;
    }
    Guide->ExtraLinks = GraphLinks;
    Guide->TunnelConnectionsMade = Branches.Num();
    return true;
}
void UDungeonCorridorNetworkComponent::BuildNetwork(UDungeonGuidanceComponent *Guide)
{
    if (!bEnabled || !Guide || GeometryActor)
        return;
    Guidance = Guide;
    Branches.Reset();
    BuildReport.Reset();
    for (AActor *Room : Guide->Rooms)
    {
        TInlineComponentArray<UStaticMeshComponent *> Meshes(Room);
        for (auto *M : Meshes)
        {
            if (!FloorMaterial && M->GetName().Contains(TEXT("Floor"), ESearchCase::IgnoreCase))
                FloorMaterial = M->GetMaterial(0);
            if (!WallMaterial && M->GetName().Contains(TEXT("Wall"), ESearchCase::IgnoreCase))
                WallMaterial = M->GetMaterial(0);
        }
        if (FloorMaterial && WallMaterial)
            break;
    }
    if (!CeilingMaterial)
        CeilingMaterial = WallMaterial;
    SearchDeadline = FPlatformTime::Seconds() + FMath::Max(.05f, SearchTimeBudgetSeconds);
    auto Available = Exits(GetOwner(), TEXT("ExitList"));
    auto *Reserved = Cast<USceneComponent>(ObjectField(GetOwner(), TEXT("ExitDoorExit")));
    Available.RemoveAll([&](USceneComponent *P) { return !IsValid(P) || P == Reserved; });
    TSet<USceneComponent *> Used;
    TSet<FString> Failed;
    int Attempts = 0;
    // Short physical links are most useful when they bypass a longer route through existing rooms.
    auto ShortcutScore = [&](AActor* A, AActor* B, double Length)
    {
        TMap<AActor*,int> Hops; TArray<AActor*> Queue; Hops.Add(A,0);Queue.Add(A);
        for(int I=0;I<Queue.Num();++I) for(const auto& L:Guide->Links)
        {
            AActor* Next=L.RoomA==Queue[I]?L.RoomB.Get():(L.RoomB==Queue[I]?L.RoomA.Get():nullptr);
            if(Next&&!Hops.Contains(Next)){Hops.Add(Next,Hops[Queue[I]]+1);Queue.Add(Next);}
        }
        return Length/(1.+.15*FMath::Min(Hops.FindRef(B),12));
    };
    while (Branches.Num() < MaxBranches && Attempts < CandidateBudget && FPlatformTime::Seconds() < SearchDeadline)
    {
        struct FCandidate
        {
            USceneComponent *A;
            USceneComponent *B;
            FVector Target;
            double Score;
            FString Key;
        };
        TArray<FCandidate> Choices;
        for (int I = 0; I < Available.Num(); ++I)
        {
            auto *A = Available[I];
            if (Used.Contains(A))
                continue;
            for (int J = I + 1; J < Available.Num(); ++J)
            {
                auto *B = Available[J];
                if (Used.Contains(B) || A->GetOwner() == B->GetOwner())
                    continue;
                double D = FVector::Dist2D(A->GetComponentLocation(), B->GetComponentLocation());
                if (D > CandidateRange)
                    continue;
                FString Key = FString::Printf(TEXT("P%d/%d"), I, J);
                if (!Failed.Contains(Key))
                    Choices.Add({A, B, B->GetComponentLocation(), ShortcutScore(A->GetOwner(),B->GetOwner(),D), Key});
            }
            if (bAllowSharedJunctions)
                for (int K = 0; K < Branches.Num(); ++K)
                    for (int S = 1; S < Branches[K].Points.Num(); ++S)
                    {
                        FVector Target = FMath::ClosestPointOnSegment(A->GetComponentLocation(),
                                                                      Branches[K].Points[S - 1], Branches[K].Points[S]);
                        double D = FVector::Dist2D(A->GetComponentLocation(), Target);
                        FString Key = FString::Printf(TEXT("N%d/%d/%d"), I, K, S);
                        if (D < CandidateRange && D > Width && !Failed.Contains(Key))
                            Choices.Add({A, nullptr, Target, D * .8, Key});
                    }
        }
        Choices.Sort([](const FCandidate &A, const FCandidate &B) { return A.Score < B.Score; });
        bool Accepted = false;
        for (const auto &C : Choices)
        {
            if (Attempts >= CandidateBudget || FPlatformTime::Seconds() > SearchDeadline)
                break;
            ++Attempts;
            TArray<FVector> Path;
            if (!FindBranch(C.A, C.B, C.Target, Path))
            {
                Failed.Add(C.Key);
                if (bDebugDraw)
                    DrawDebugLine(GetWorld(), C.A->GetComponentLocation() + FVector(0, 0, 100),
                                  C.Target + FVector(0, 0, 100), FColor::Red, false, 30);
                continue;
            }
            FCorridorBranch Branch;
            Branch.Points = Path;
            Branch.PortA = C.A;
            Branch.PortB = C.B;
            Branches.Add(Branch);
            Used.Add(C.A);
            if (C.B)
                Used.Add(C.B);
            Accepted = true;
            if (bDebugDraw)
                for (int I = 1; I < Path.Num(); ++I)
                    DrawDebugLine(GetWorld(), Path[I - 1] + FVector(0, 0, 100), Path[I] + FVector(0, 0, 100),
                                  FColor::Green, false, 30, 0, 4);
            break;
        }
        if (!Accepted)
            break;
    }
    SearchDeadline = 0;
    if (!Branches.IsEmpty() && RebuildGeometryAndGraph(Guide))
    {
        for (auto *Exit : Used)
        {
            RemoveExit(GetOwner(), TEXT("ExitList"), Exit);
            AddExit(Exit->GetOwner(), Exit);
        }
    }
    else
    {
        Branches.Reset();
        Used.Reset();
    }
    BuildReport = FString::Printf(
        TEXT("Ports %d; branches %d; T junctions %d; X junctions %d; attempts %d; rejected %d; budget %.2fs"),
        Available.Num(), Branches.Num(), TJunctions, XJunctions, Attempts, Failed.Num(), SearchTimeBudgetSeconds);
    UE_LOG(LogDungeonGuidance, Display, TEXT("Corridor network: %s"), *BuildReport);
}

