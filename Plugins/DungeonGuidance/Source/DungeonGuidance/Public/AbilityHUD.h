#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AbilityHUD.generated.h"
class UInputAction;
class UInputMappingContext;
UENUM(BlueprintType)
enum class EAbilityHUDSymbol : uint8
{
    Shield,
    Wind,
    Gun,
    Snap
};
USTRUCT(BlueprintType)
struct DUNGEONGUIDANCE_API FAbilityHUDEntry
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category = "Ability HUD") EAbilityHUDSymbol Symbol = EAbilityHUDSymbol::Shield;
    UPROPERTY(BlueprintReadOnly, Category = "Ability HUD") bool Filled = true;
    UPROPERTY(BlueprintReadOnly, Category = "Ability HUD") FText Value;
    UPROPERTY(BlueprintReadOnly, Category = "Ability HUD") FText Key;
    UPROPERTY(BlueprintReadOnly, Category = "Ability HUD") int32 Capacity = 0;
    UPROPERTY(BlueprintReadOnly, Category = "Ability HUD") int32 Remaining = 0;
};
// Drawn as transparent vectors: no baked key labels or state-specific image assets.
UCLASS(Blueprintable)
class DUNGEONGUIDANCE_API UAbilityHUDWidget : public UUserWidget
{
    GENERATED_BODY()
  public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability HUD|Input")
    TObjectPtr<UInputMappingContext> FallbackMapping;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability HUD|Input") TObjectPtr<UInputAction> BlockAction;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability HUD|Input") TObjectPtr<UInputAction> DashAction;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability HUD|Layout") float SlotWidth = 104;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability HUD|Layout") float SlotGap = 24;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability HUD|Layout") float IconSize = 64;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability HUD|Layout") float BottomMargin = 32;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability HUD|Layout") float CrosshairShieldOffset = 44;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability HUD|Layout") float SmallShieldSize = 22;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability HUD|Layout") float SmallShieldGap = 6;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability HUD|Appearance")
    FLinearColor Color = FLinearColor(.94, .97, 1, 1);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability HUD|Appearance") int32 FontSize = 18;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability HUD|Appearance") bool DecimalComma = true;
    UPROPERTY(BlueprintReadOnly, Transient, Category = "Ability HUD|State") TArray<FAbilityHUDEntry> Entries;
    UPROPERTY(BlueprintReadOnly, Transient, Category = "Ability HUD|State") bool Blocking = false;
    UPROPERTY(BlueprintReadOnly, Transient, Category = "Ability HUD|State") int32 MaxBlockHits = 0;
    UPROPERTY(BlueprintReadOnly, Transient, Category = "Ability HUD|State") int32 RemainingBlockHits = 0;
    UFUNCTION(BlueprintCallable, Category = "Ability HUD") void RefreshState();
    UFUNCTION(BlueprintPure, Category = "Ability HUD") FText KeyLabel(UInputAction *Action) const;
    UFUNCTION(BlueprintPure, Category = "Ability HUD") TArray<float> SlotCenters(float ViewWidth) const;
    virtual void NativeTick(const FGeometry &Geometry, float Dt) override;
    virtual int32 NativePaint(const FPaintArgs &Args, const FGeometry &Geometry, const FSlateRect &CullingRect,
                              FSlateWindowElementList &Elements, int32 Layer, const FWidgetStyle &Style,
                              bool ParentEnabled) const override;

  private:
    FText Seconds(float Value) const;
};
