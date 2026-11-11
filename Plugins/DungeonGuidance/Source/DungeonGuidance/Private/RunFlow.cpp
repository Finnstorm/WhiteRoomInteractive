#include "RunFlow.h"
#include "Engine/GameInstance.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"
namespace
{
UGameInstance *Game(UObject *Context)
{
    return UGameplayStatics::GetGameInstance(Context);
}
URunOptionsSubsystem *Options(UObject *Context)
{
    auto *GI = Game(Context);
    return GI ? GI->GetSubsystem<URunOptionsSubsystem>() : nullptr;
}
void CallNoArgs(UObject *Object, FName Name)
{
    if (Object)
        if (auto *Fn = Object->FindFunction(Name))
            Object->ProcessEvent(Fn, nullptr);
}
FName NameProperty(UObject *Object, FName Name)
{
    if (Object)
        if (auto *P = FindFProperty<FNameProperty>(Object->GetClass(), Name))
            return P->GetPropertyValue_InContainer(Object);
    return NAME_None;
}
void FocusMenu(UUserWidget *Widget)
{
    Widget->SetIsFocusable(true);
    if (auto *PC = Widget->GetOwningPlayer())
    {
        PC->bShowMouseCursor = true;
        UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(PC, Widget, EMouseLockMode::DoNotLock, true);
    }
}
void ResetProgress(UObject *Context)
{
    CallNoArgs(Game(Context), TEXT("ResetAbilityTreeForTesting"));
}
} // namespace
void URunOptionsSubsystem::PrepareRun(int32 Seed, ERunDifficulty Difficulty)
{
    RequestedSeed = Seed;
    SelectedDifficulty = Difficulty;
    bHasPendingSeed = true;
}
bool URunFlowLibrary::ParseSeed(const FString &Text, int32 &OutSeed)
{
    FString S = Text.TrimStartAndEnd();
    if (S.IsEmpty())
    {
        OutSeed = -1;
        return true;
    }
    int64 V = 0;
    if (!LexTryParseString(V, *S) || V < MIN_int32 || V > MAX_int32)
        return false;
    int I = S.StartsWith(TEXT("-")) || S.StartsWith(TEXT("+")) ? 1 : 0;
    if (I == S.Len())
        return false;
    for (; I < S.Len(); ++I)
        if (!FChar::IsDigit(S[I]))
            return false;
    OutSeed = int32(V);
    return true;
}
int32 URunFlowLibrary::LatestSeed(UObject *Context)
{
    if (auto *GI = Game(Context))
        if (auto *P = FindFProperty<FIntProperty>(GI->GetClass(), TEXT("Seed")))
            return P->GetPropertyValue_InContainer(GI);
    return -1;
}
void URunFlowLibrary::ApplyPendingSeed(AActor *Generator)
{
    if (!Generator)
        return;
    if (auto *O = Options(Generator))
        if (O->bHasPendingSeed)
        {
            if (auto *P = FindFProperty<FIntProperty>(Generator->GetClass(), TEXT("Seed")))
            {
                P->SetPropertyValue_InContainer(Generator, O->RequestedSeed);
                O->bHasPendingSeed = false;
            }
        }
}
void URunFlowLibrary::StartRun(UUserWidget *Source, int32 Seed, ERunDifficulty Difficulty)
{
    if (!Source)
        return;
    if (auto *O = Options(Source))
        O->PrepareRun(Seed, Difficulty);
    CallNoArgs(Game(Source), TEXT("SaveGame"));
    UGameplayStatics::SetGamePaused(Source, false);
    if (auto *PC = Source->GetOwningPlayer())
    {
        PC->bShowMouseCursor = false;
        UWidgetBlueprintLibrary::SetInputMode_GameOnly(PC, true);
    }
    Source->RemoveFromParent();
    UGameplayStatics::OpenLevel(Source, TEXT("/Game/Maps/ProceduralDungeon"));
}
void URunFlowLibrary::HideGameplayHUD(UObject *Context)
{
    TArray<UUserWidget *> Widgets;
    UWidgetBlueprintLibrary::GetAllWidgetsOfClass(Context, Widgets, UUserWidget::StaticClass(), false);
    for (auto *W : Widgets)
        if (W && W->GetClass()->GetName() == TEXT("IngameHUD_C"))
            W->SetVisibility(ESlateVisibility::Collapsed);
}
void URunFlowLibrary::RefreshTreeEquipState(UUserWidget *Tree)
{
    if (!Tree)
        return;
    const FName Current = NameProperty(Tree, TEXT("CurrentLoadedTree"));
    bool Active = Current == TEXT("BaseTree");
    UObject *Save = nullptr;
    if (auto *P = FindFProperty<FObjectPropertyBase>(Tree->GetClass(), TEXT("SaveGameRef")))
        Save = P->GetObjectPropertyValue_InContainer(Tree);
    const FName Equipped = NameProperty(Save, TEXT("EquippedSkillTree"));
    Active |= !Current.IsNone() && Current == Equipped;
    if (auto *W = Tree->GetWidgetFromName(TEXT("Btn_Equipp")))
        W->SetVisibility(Active ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    if (auto *W = Cast<UTextBlock>(Tree->GetWidgetFromName(TEXT("Txt_TreeActive"))))
    {
        W->SetText(FText::FromString(TEXT("Active")));
        W->SetVisibility(Active ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }
}
void URunFlowLibrary::ShowRunSummary(UUserWidget *Tree, TSubclassOf<UUserWidget> ScreenClass)
{
    if (!Tree)
        return;
    UClass *Class = ScreenClass.Get();
    if (!Class)
        return;
    auto *Summary = CreateWidget<URunSummaryWidget>(Tree->GetOwningPlayer(), Class);
    if (!Summary)
        return;
    Summary->PreviousTree = Tree;
    Summary->PreviousTreeID = NameProperty(Tree, TEXT("CurrentLoadedTree"));
    Tree->RemoveFromParent();
    Summary->AddToViewport(20);
    FocusMenu(Summary);
}
void URunFlowLibrary::ShowRunSetup(UUserWidget *MainMenu, TSubclassOf<UUserWidget> ScreenClass)
{
    if (!MainMenu)
        return;
    UClass *Class = ScreenClass.Get();
    if (!Class)
        return;
    auto *Setup = CreateWidget<URunSetupWidget>(MainMenu->GetOwningPlayer(), Class);
    if (!Setup)
        return;
    Setup->PreviousMenu = MainMenu;
    MainMenu->RemoveFromParent();
    Setup->AddToViewport(20);
    FocusMenu(Setup);
}
void URunSummaryWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    Btn_CopySeed->OnClicked.AddDynamic(this, &URunSummaryWidget::CopySeed);
    Btn_PlaySeedAgain->OnClicked.AddDynamic(this, &URunSummaryWidget::ReplaySeed);
    Btn_PlayAgain->OnClicked.AddDynamic(this, &URunSummaryWidget::PlayAgain);
    Btn_MainMenu->OnClicked.AddDynamic(this, &URunSummaryWidget::MainMenu);
    Btn_QuitGame->OnClicked.AddDynamic(this, &URunSummaryWidget::QuitGame);
    Btn_BackToTrees->OnClicked.AddDynamic(this, &URunSummaryWidget::BackToTrees);
}
void URunSummaryWidget::NativeConstruct()
{
    Super::NativeConstruct();
    DisplayedSeed = URunFlowLibrary::LatestSeed(this);
    Txt_Seed->SetText(FText::FromString(FString::Printf(TEXT("Run seed: %d"), DisplayedSeed)));
    FocusMenu(this);
}
void URunSummaryWidget::CopySeed()
{
    FPlatformApplicationMisc::ClipboardCopy(*FString::FromInt(DisplayedSeed));
}
void URunSummaryWidget::ReplaySeed()
{
    auto *O = Options(this);
    URunFlowLibrary::StartRun(this, DisplayedSeed, O ? O->SelectedDifficulty : ERunDifficulty::Medium);
}
void URunSummaryWidget::PlayAgain()
{
    auto *O = Options(this);
    URunFlowLibrary::StartRun(this, -1, O ? O->SelectedDifficulty : ERunDifficulty::Medium);
}
void URunSummaryWidget::MainMenu()
{
    ResetProgress(this);
    UGameplayStatics::SetGamePaused(this, false);
    RemoveFromParent();
    UGameplayStatics::OpenLevel(this, TEXT("/Game/Maps/L_MainMenu"));
}
void URunSummaryWidget::QuitGame()
{
    ResetProgress(this);
    UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}
void URunSummaryWidget::BackToTrees()
{
    if (PreviousTree)
    {
        RemoveFromParent();
        PreviousTree->AddToViewport(20);
        if (auto *Fn = PreviousTree->FindFunction(TEXT("LoadSkillTree")))
        {
            FStructOnScope Args(Fn);
            if (auto *P = FindFProperty<FNameProperty>(Fn, TEXT("TargetTreeID")))
            {
                P->SetPropertyValue_InContainer(Args.GetStructMemory(), PreviousTreeID);
                PreviousTree->ProcessEvent(Fn, Args.GetStructMemory());
            }
        }
        FocusMenu(PreviousTree);
    }
}
void URunSetupWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    Btn_PlayGame->OnClicked.AddDynamic(this, &URunSetupWidget::PlayGame);
    Btn_Back->OnClicked.AddDynamic(this, &URunSetupWidget::Back);
    Select_Difficulty->ClearOptions();
    for (const TCHAR *Label : {TEXT("Easy"), TEXT("Medium"), TEXT("Hard"), TEXT("Smart Difficulty")})
        Select_Difficulty->AddOption(Label);
    Select_Difficulty->SetSelectedIndex(1);
}
void URunSetupWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (auto *O = Options(this))
        Select_Difficulty->SetSelectedIndex(int32(O->SelectedDifficulty));
    FocusMenu(this);
}
void URunSetupWidget::PlayGame()
{
    int32 Seed;
    if (!URunFlowLibrary::ParseSeed(Input_Seed->GetText().ToString(), Seed))
    {
        Txt_Validation->SetText(
            FText::FromString(TEXT("Enter a whole number from -2147483648 to 2147483647, or leave blank.")));
        return;
    }
    Txt_Validation->SetText(FText::GetEmpty());
    URunFlowLibrary::StartRun(this, Seed, ERunDifficulty(FMath::Clamp(Select_Difficulty->GetSelectedIndex(), 0, 3)));
}
void URunSetupWidget::Back()
{
    if (PreviousMenu)
    {
        RemoveFromParent();
        PreviousMenu->AddToViewport(10);
        FocusMenu(PreviousMenu);
    }
}

