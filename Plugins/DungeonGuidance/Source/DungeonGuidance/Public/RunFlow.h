#pragma once
#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RunFlow.generated.h"
class UButton;
class UTextBlock;
class UEditableTextBox;
class UComboBoxString;
UENUM(BlueprintType)
enum class ERunDifficulty : uint8
{
    Easy,
    Medium,
    Hard,
    SmartDifficulty UMETA(DisplayName = "Smart Difficulty")
};
// Run options persist across level travel. Difficulty is stored only; gameplay can consume it later.
UCLASS()
class DUNGEONGUIDANCE_API URunOptionsSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
  public:
    UPROPERTY(BlueprintReadWrite, Category = "Run Options") ERunDifficulty SelectedDifficulty = ERunDifficulty::Medium;
    UPROPERTY(BlueprintReadOnly, Category = "Run Options") int32 RequestedSeed = -1;
    UPROPERTY(BlueprintReadOnly, Category = "Run Options") bool bHasPendingSeed = false;
    UFUNCTION(BlueprintCallable, Category = "Run Options") void PrepareRun(int32 Seed, ERunDifficulty Difficulty);
};
UCLASS()
class DUNGEONGUIDANCE_API URunFlowLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable, Category = "Run Flow", meta = (WorldContext = "Context"))
    static void HideGameplayHUD(UObject *Context);
    UFUNCTION(BlueprintCallable, Category = "Run Flow")
    static void ShowRunSummary(UUserWidget *Tree, TSubclassOf<UUserWidget> ScreenClass);
    UFUNCTION(BlueprintCallable, Category = "Run Flow")
    static void ShowRunSetup(UUserWidget *MainMenu, TSubclassOf<UUserWidget> ScreenClass);
    UFUNCTION(BlueprintCallable, Category = "Run Flow") static void RefreshTreeEquipState(UUserWidget *Tree);
    UFUNCTION(BlueprintCallable, Category = "Run Flow") static void ApplyPendingSeed(AActor *Generator);
    static void StartRun(UUserWidget *Source, int32 Seed, ERunDifficulty Difficulty);
    static int32 LatestSeed(UObject *Context);
    static bool ParseSeed(const FString &Text, int32 &OutSeed);
};
UCLASS(Abstract, Blueprintable)
class DUNGEONGUIDANCE_API URunSummaryWidget : public UUserWidget
{
    GENERATED_BODY()
  public:
    UPROPERTY(BlueprintReadOnly, Category = "Run") int32 DisplayedSeed = -1;
    UPROPERTY(Transient) TObjectPtr<UUserWidget> PreviousTree;
    UPROPERTY(Transient) FName PreviousTreeID;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Txt_Seed;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Btn_CopySeed;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Btn_PlaySeedAgain;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Btn_PlayAgain;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Btn_MainMenu;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Btn_QuitGame;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Btn_BackToTrees;
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    UFUNCTION() void CopySeed();
    UFUNCTION() void ReplaySeed();
    UFUNCTION() void PlayAgain();
    UFUNCTION() void MainMenu();
    UFUNCTION() void QuitGame();
    UFUNCTION() void BackToTrees();
};
UCLASS(Abstract, Blueprintable)
class DUNGEONGUIDANCE_API URunSetupWidget : public UUserWidget
{
    GENERATED_BODY()
  public:
    UPROPERTY(Transient) TObjectPtr<UUserWidget> PreviousMenu;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UEditableTextBox> Input_Seed;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UComboBoxString> Select_Difficulty;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Txt_Validation;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Btn_PlayGame;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Btn_Back;
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    UFUNCTION() void PlayGame();
    UFUNCTION() void Back();
};
