#include "AbilityHUD.h"
#include "BountyHunter.h"
#include "Entertainer.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "Algo/Reverse.h"
#include "Internationalization/Internationalization.h"
#include "Rendering/RenderingCommon.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"
#include "UObject/UnrealType.h"

namespace AbilityHUDImpl
{
UObject *ReadObject(UObject *O, FName Name)
{
    if (O)
        if (auto *P = FindFProperty<FObjectPropertyBase>(O->GetClass(), Name))
            return P->GetObjectPropertyValue_InContainer(O);
    return nullptr;
}
double ReadNumber(UObject *O, FName Name, double Default = 0)
{
    if (O)
        if (auto *P = FindFProperty<FNumericProperty>(O->GetClass(), Name))
        {
            auto *V = P->ContainerPtrToValuePtr<void>(O);
            return P->IsFloatingPoint() ? P->GetFloatingPointPropertyValue(V) : P->GetSignedIntPropertyValue(V);
        }
    return Default;
}
bool ReadBool(UObject *O, FName Name)
{
    if (O)
        if (auto *P = FindFProperty<FBoolProperty>(O->GetClass(), Name))
            return P->GetPropertyValue_InContainer(O);
    return false;
}
TSet<FName> Purchased(UObject *Context)
{
    TSet<FName> IDs;
    auto *Save = ReadObject(UGameplayStatics::GetGameInstance(Context), TEXT("SaveGameRef"));
    if (Save)
        if (auto *P = FindFProperty<FArrayProperty>(Save->GetClass(), TEXT("UnlockedSkillNodeIDs")))
        {
            FScriptArrayHelper A(P, P->ContainerPtrToValuePtr<void>(Save));
            if (auto *Inner = CastField<FNameProperty>(P->Inner))
                for (int I = 0; I < A.Num(); ++I)
                    IDs.Add(Inner->GetPropertyValue(A.GetRawPtr(I)));
        }
    return IDs;
}
FString FriendlyKey(const FKey &K)
{
    if (K == EKeys::RightMouseButton)
        return TEXT("RMB");
    if (K == EKeys::LeftMouseButton)
        return TEXT("LMB");
    if (K == EKeys::MiddleMouseButton)
        return TEXT("MMB");
    if (K == EKeys::LeftControl)
        return TEXT("L CTRL");
    if (K == EKeys::RightControl)
        return TEXT("R CTRL");
    if (K == EKeys::LeftShift)
        return TEXT("L SHIFT");
    if (K == EKeys::RightShift)
        return TEXT("R SHIFT");
    return K.GetDisplayName(false).ToString().ToUpper();
}

// All icon coordinates are normalized to a 64px square and scaled at paint time.
using FPath = TArray<FVector2D>;
FPath Cubic(FVector2D Start, FVector2D C0, FVector2D C1, FVector2D End, int Samples = 16)
{
    FPath P;
    for (int I = 0; I <= Samples; ++I)
    {
        double T = double(I) / Samples, U = 1 - T;
        P.Add(Start * (U * U * U) + C0 * (3 * U * U * T) + C1 * (3 * U * T * T) + End * (T * T * T));
    }
    return P;
}
void AppendCurve(FPath &P, FVector2D C0, FVector2D C1, FVector2D End)
{
    auto Next = Cubic(P.Last(), C0, C1, End);
    Next.RemoveAt(0);
    P.Append(Next);
}
double Cross(FVector2D A, FVector2D B)
{
    return A.X * B.Y - A.Y * B.X;
}
struct FPainter
{
    const FGeometry &G;
    FSlateWindowElementList &Elements;
    int32 Layer;
    FLinearColor Color;
    FPath Transform(const FPath &P, FVector2D Origin, float Size) const
    {
        FPath R;
        for (auto V : P)
            R.Add(Origin + V * (Size / 64.));
        return R;
    }
    void Line(const FPath &P, float Thickness = 2, bool Closed = false) const
    {
        if (P.Num() < 2)
            return;
        TArray<FVector2f> V;
        for (auto A : P)
            V.Add(FVector2f(A));
        if (Closed)
            V.Add(FVector2f(V[0]));
        FSlateDrawElement::MakeLines(Elements, Layer, G.ToPaintGeometry(), V, ESlateDrawEffect::None, Color, true,
                                     Thickness);
    }
    void Fill(FPath P) const
    {
        if (P.Num() < 3)
            return;
        double Area = 0;
        for (int I = 0; I < P.Num(); ++I)
            Area += Cross(P[I], P[(I + 1) % P.Num()]);
        if (Area < 0)
            Algo::Reverse(P);
        TArray<FSlateVertex> V;
        for (auto A : P)
            V.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(G.GetAccumulatedRenderTransform(), FVector2f(A),
                                                                     FVector2f(.5, .5), Color.ToFColor(true)));
        TArray<int32> Ring;
        for (int I = 0; I < P.Num(); ++I)
            Ring.Add(I);
        TArray<SlateIndex> Indices;
        for (int Limit = 0; Ring.Num() > 2 && Limit < P.Num() * P.Num(); ++Limit)
        {
            bool Clipped = false;
            for (int J = 0; J < Ring.Num(); ++J)
            {
                int A = Ring[(J + Ring.Num() - 1) % Ring.Num()], B = Ring[J], C = Ring[(J + 1) % Ring.Num()];
                if (Cross(P[B] - P[A], P[C] - P[B]) <= 1.e-8)
                    continue;
                bool Inside = false;
                for (int K : Ring)
                    if (K != A && K != B && K != C && Cross(P[B] - P[A], P[K] - P[A]) >= -1.e-8 &&
                        Cross(P[C] - P[B], P[K] - P[B]) >= -1.e-8 && Cross(P[A] - P[C], P[K] - P[C]) >= -1.e-8)
                    {
                        Inside = true;
                        break;
                    }
                if (Inside)
                    continue;
                Indices.Append({SlateIndex(A), SlateIndex(B), SlateIndex(C)});
                Ring.RemoveAt(J);
                Clipped = true;
                break;
            }
            if (!Clipped)
                break;
        }
        const auto Handle =
            FSlateApplication::Get().GetRenderer()->GetResourceHandle(*FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")));
        FSlateDrawElement::MakeCustomVerts(Elements, Layer, Handle, V, Indices, nullptr, 0, 0);
    }
    void Ribbon(const FPath &P, float Width, bool Filled) const
    {
        if (Filled)
        {
            Line(P, Width);
            return;
        }
        FPath Left, Right;
        for (int I = 0; I < P.Num(); ++I)
        {
            auto D = (P[FMath::Min(I + 1, P.Num() - 1)] - P[FMath::Max(0, I - 1)]).GetSafeNormal();
            FVector2D N(-D.Y, D.X);
            Left.Add(P[I] + N * Width * .5);
            Right.Add(P[I] - N * Width * .5);
        }
        FPath Ring = Left;
        Algo::Reverse(Right);
        Ring.Append(Right);
        Line(Ring, 1.2, true);
    }
    void Shield(FVector2D O, float Size, bool Filled) const
    {
        FPath P = {{32, 3}, {58, 13}};
        AppendCurve(P, {57, 38}, {48, 52}, {32, 62});
        AppendCurve(P, {16, 52}, {7, 38}, {6, 13});
        P.Add({32, 3});
        P.Pop();
        Line(Transform(P, O, Size), FMath::Max(1.2f, Size * .035f), true);
        if (Filled)
        {
            FPath Inner;
            for (auto V : P)
                Inner.Add(FVector2D(32, 30) + (V - FVector2D(32, 30)) * .78);
            Fill(Transform(Inner, O, Size));
        }
    }
    void Wind(FVector2D O, float Size, bool Filled) const
    {
        FPath A = {{5, 22}, {35, 22}};
        AppendCurve(A, {60, 22}, {58, 3}, {45, 6});
        AppendCurve(A, {34, 8}, {37, 18}, {44, 15});
        FPath B = {{3, 34}, {47, 34}};
        AppendCurve(B, {70, 34}, {66, 17}, {56, 21});
        FPath C = {{12, 46}, {35, 46}};
        AppendCurve(C, {58, 46}, {57, 64}, {44, 60});
        AppendCurve(C, {33, 57}, {37, 51}, {43, 54});
        for (auto *P : {&A, &B, &C})
            Ribbon(Transform(*P, O, Size), Size * .078f, Filled);
    }
    void Gun(FVector2D O, float Size, bool Filled) const
    {
        FPath P = {{4, 16}, {59, 16}, {59, 28}, {36, 28}, {33, 34}, {25, 34}, {30, 55}, {15, 55}, {9, 29}, {4, 29}};
        auto T = Transform(P, O, Size);
        if (Filled)
            Fill(T);
        else
            Line(T, 2, true);
        Line(Transform({{30, 29}, {31, 36}, {39, 36}, {42, 28}}, O, Size), 1.8);
    }
    void Bullet(FVector2D O, float Width, float Height, bool Filled) const
    {
        FPath P = {{13, 60}, {13, 22}};
        AppendCurve(P, {13, 13}, {22, 5}, {32, 2});
        AppendCurve(P, {42, 5}, {51, 13}, {51, 22});
        P.Add({51, 60});
        FPath T;
        for (auto V : P)
            T.Add(O + FVector2D(V.X * Width / 64., V.Y * Height / 64.));
        if (Filled)
            Fill(T);
        else
            Line(T, 1.4, true);
    }
    void Snap(FVector2D O, float Size, bool Filled) const
    {
        FPath Hand = {{22, 60}, {15, 48}, {12, 38}, {17, 33}, {26, 42}, {22, 20}, {27, 16},
                      {32, 20}, {36, 37}, {39, 28}, {44, 26}, {49, 31}, {49, 45}, {43, 60}};
        auto T = Transform(Hand, O, Size);
        if (Filled)
            Fill(T);
        else
            Line(T, 2, true);
        Line(Transform({{37, 14}, {42, 4}}, O, Size), 2);
        Line(Transform({{47, 17}, {58, 10}}, O, Size), 2);
        Line(Transform({{50, 23}, {62, 23}}, O, Size), 2);
    }
    void Icon(EAbilityHUDSymbol Symbol, FVector2D O, float Size, bool Filled) const
    {
        switch (Symbol)
        {
        case EAbilityHUDSymbol::Shield:
            Shield(O, Size, Filled);
            break;
        case EAbilityHUDSymbol::Wind:
            Wind(O, Size, Filled);
            break;
        case EAbilityHUDSymbol::Gun:
            Gun(O, Size, Filled);
            break;
        case EAbilityHUDSymbol::Snap:
            Snap(O, Size, Filled);
            break;
        }
    }
    void Text(const FText &Value, float Center, float Y, int32 Size, float MaxWidth) const
    {
        auto Font = FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), Size);
        auto Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
        auto Extent = Measure->Measure(Value, Font);
        if (Extent.X > MaxWidth)
        {
            Font.Size = FMath::Max(9, FMath::FloorToInt(Size * MaxWidth / Extent.X));
            Extent = Measure->Measure(Value, Font);
        }
        auto Position = FVector2f(Center - Extent.X * .5, Y);
        FSlateDrawElement::MakeText(Elements, Layer,
                                    G.ToPaintGeometry(FVector2f(Extent), FSlateLayoutTransform(Position)), Value, Font,
                                    ESlateDrawEffect::None, Color);
    }
};
} // namespace AbilityHUDImpl

FText UAbilityHUDWidget::KeyLabel(UInputAction *Action) const
{
    if (!Action)
        return FText::FromString(TEXT("—"));
    TArray<FKey> Keys;
    bool Live = false;
    if (auto *PC = GetOwningPlayer())
        if (auto *LP = PC->GetLocalPlayer())
            if (auto *S = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
            {
                Live = S->GetPlayerInput() != nullptr;
                if (Live)
                    Keys = S->QueryKeysMappedToAction(Action);
            }
    if (!Live && FallbackMapping)
        for (const auto &M : FallbackMapping->GetMappings())
            if (M.Action == Action)
                Keys.AddUnique(M.Key);
    // Keyboard/mouse labels are preferred; retain a gamepad label if it is the only binding.
    const bool HasKeyboard = Keys.ContainsByPredicate([](const FKey &K) { return !K.IsGamepadKey() && K.IsValid(); });
    TArray<FString> Labels;
    for (auto K : Keys)
        if (K.IsValid() && (!HasKeyboard || !K.IsGamepadKey()))
            Labels.AddUnique(AbilityHUDImpl::FriendlyKey(K));
    return FText::FromString(Labels.IsEmpty() ? TEXT("—") : FString::Join(Labels, TEXT(" / ")));
}
FText UAbilityHUDWidget::Seconds(float Value) const
{
    FNumberFormattingOptions Format;
    Format.UseGrouping = false;
    Format.MinimumFractionalDigits = 1;
    Format.MaximumFractionalDigits = 1;
    auto Culture = DecimalComma ? FInternationalization::Get().GetCulture(TEXT("de-DE"))
                                : FInternationalization::Get().GetCurrentCulture();
    return FText::Format(FText::FromString(TEXT("{0} s")), FText::AsNumber(FMath::Max(0.f, Value), &Format, Culture));
}
void UAbilityHUDWidget::RefreshState()
{
    using namespace AbilityHUDImpl;
    ForceVolatile(true);
    Entries.Reset();
    Blocking = false;
    MaxBlockHits = RemainingBlockHits = 0;
    auto *Player = GetOwningPlayerPawn();
    if (!Player || ReadNumber(ReadObject(Player, TEXT("AC_PlayerHealth")), TEXT("CurrentHealth")) <= 0)
        return;
    auto IDs = Purchased(this);
    auto *Combat = ReadObject(Player, TEXT("AC_CombatSystem"));
    if (Combat)
    {
        FAbilityHUDEntry E;
        E.Symbol = EAbilityHUDSymbol::Shield;
        float Power = FMath::Clamp(
            ReadNumber(Combat, TEXT("BlockDamageReduction")) - ReadNumber(Combat, TEXT("BlockPowerLoss")), 0., 1.);
        E.Filled = Power > 0;
        E.Key = KeyLabel(BlockAction);
        E.Value = FText::FromString(FString::Printf(TEXT("%.0f%%"), Power * 100));
        Entries.Add(E);
        Blocking = ReadBool(Combat, TEXT("BlockActive"));
        MaxBlockHits = FMath::Clamp(int(ReadNumber(Combat, TEXT("MaxBlockHits"))), 0, 64);
        RemainingBlockHits = FMath::Clamp(int(ReadNumber(Combat, TEXT("RemainingBlockHits"))), 0, MaxBlockHits);
    }
    if (IDs.Contains(TEXT("BaseTree_Main3")))
    {
        FAbilityHUDEntry E;
        E.Symbol = EAbilityHUDSymbol::Wind;
        float Time =
            FMath::Max(0.f, UKismetSystemLibrary::K2_GetTimerRemainingTime(Player, TEXT("FinishDashCooldown")));
        E.Filled = !ReadBool(Player, TEXT("DashCooldownActive")) && Time <= 0;
        E.Key = KeyLabel(DashAction);
        E.Value = Seconds(Time);
        Entries.Add(E);
    }
    if (auto *C = Player->FindComponentByClass<UBountyHunterComponent>())
        if (IDs.Contains(TEXT("BountyHunter_Unlock")))
        {
            FAbilityHUDEntry E;
            E.Symbol = EAbilityHUDSymbol::Gun;
            E.Filled = C->Stats.Equipped && C->Bullets > 0 && C->CooldownRemaining <= 0 && !C->bWindingUp;
            E.Key = KeyLabel(C->FireAction);
            E.Capacity = FMath::Clamp(C->Stats.MaxBullets, 0, 64);
            E.Remaining = FMath::Clamp(C->Bullets, 0, E.Capacity);
            Entries.Add(E);
        }
    if (auto *C = Player->FindComponentByClass<UEntertainerComponent>())
        if (IDs.Contains(TEXT("Entertainer_Unlock")))
        {
            FAbilityHUDEntry E;
            E.Symbol = EAbilityHUDSymbol::Snap;
            E.Filled = C->Stats.Equipped && C->Combo + KINDA_SMALL_NUMBER >= C->Stats.Cost &&
                       C->CooldownRemaining <= 0 && !C->Pending;
            E.Key = KeyLabel(C->ActivateAction);
            E.Value = C->CooldownRemaining > 0
                          ? Seconds(C->CooldownRemaining)
                          : FText::FromString(FString::Printf(TEXT("%.0f / %.0f"), C->Combo, C->Stats.Cost));
            Entries.Add(E);
        }
}
TArray<float> UAbilityHUDWidget::SlotCenters(float ViewWidth) const
{
    TArray<float> Centers;
    float Step = FMath::Max(1.f, SlotWidth) + FMath::Max(0.f, SlotGap);
    for (int I = 0; I < Entries.Num(); ++I)
        Centers.Add(ViewWidth * .5f + (I - (Entries.Num() - 1) * .5f) * Step);
    return Centers;
}
void UAbilityHUDWidget::NativeTick(const FGeometry &G, float Dt)
{
    Super::NativeTick(G, Dt);
    RefreshState();
}
int32 UAbilityHUDWidget::NativePaint(const FPaintArgs &Args, const FGeometry &G, const FSlateRect &Culling,
                                     FSlateWindowElementList &Elements, int32 Layer, const FWidgetStyle &Style,
                                     bool ParentEnabled) const
{
    int32 Top = Super::NativePaint(Args, G, Culling, Elements, Layer, Style, ParentEnabled) + 1;
    AbilityHUDImpl::FPainter P{G, Elements, Top, Color * Style.GetColorAndOpacityTint()};
    auto Size = G.GetLocalSize();
    auto Centers = SlotCenters(Size.X);
    float Glyph = FMath::Clamp(IconSize, 24.f, SlotWidth - 8), LineHeight = FontSize + 6,
          IconTop = Size.Y - BottomMargin - Glyph - LineHeight * 2 - 10, ValueY = IconTop + Glyph + 8,
          KeyY = ValueY + LineHeight;
    for (int I = 0; I < Entries.Num(); ++I)
    {
        const auto &E = Entries[I];
        float X = Centers[I];
        P.Icon(E.Symbol, {X - Glyph * .5, IconTop}, Glyph, E.Filled);
        if (E.Capacity > 0)
        {
            float W = FMath::Min(15.f, (SlotWidth - 12) / FMath::Max(1, E.Capacity)), Step = W + 2;
            float Start = X - (E.Capacity * Step - 2) * .5;
            for (int J = 0; J < E.Capacity; ++J)
                P.Bullet({Start + J * Step, ValueY - 1}, W, LineHeight - 2, J < E.Remaining);
        }
        else
            P.Text(E.Value, X, ValueY, FontSize, SlotWidth - 8);
        P.Text(E.Key, X, KeyY, FontSize, SlotWidth - 4);
    }
    if (Blocking && MaxBlockHits > 0)
    {
        float GlyphSize = FMath::Clamp(SmallShieldSize, 8.f, 40.f);
        float Step = GlyphSize + FMath::Max(0.f, SmallShieldGap);
        float Width = MaxBlockHits * Step - SmallShieldGap;
        float Fit = FMath::Min(1.f, (Size.X * .65f) / FMath::Max(1.f, Width));
        GlyphSize *= Fit;
        Step *= Fit;
        float Start = Size.X * .5f - (MaxBlockHits * Step - SmallShieldGap * Fit) * .5f;
        for (int I = 0; I < MaxBlockHits; ++I)
            P.Shield({Start + I * Step, Size.Y * .5f + CrosshairShieldOffset}, GlyphSize, I < RemainingBlockHits);
    }
    return Top;
}
