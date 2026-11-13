#include "ProgressEnemyEditorLibrary.h"
#include "Engine/Blueprint.h"
#include "UObject/UnrealType.h"
#include "ProgressEnemyCompatibility.h"
#include "ProgressEnemyCharacter.h"
#include "ProgressEnemySpawner.h"
#include "ProgressEnemyAnimInstance.h"
#if WITH_EDITOR
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_Event.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_CallFunction.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_Self.h"
#include "K2Node_VariableGet.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Factories/AnimBlueprintFactory.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimBlueprintGeneratedClass.h"
#include "Animation/BlendSpace.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimMontage.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "EditorAnimUtils.h"
#include "AnimGraphNode_Root.h"
#include "AnimGraphNode_Slot.h"
#include "AnimGraphNode_BlendSpacePlayer.h"
#include "Components/SkeletalMeshComponent.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#endif

FString UProgressEnemyEditorLibrary::InspectBlueprint(const FString& Path)
{
    FString Out;
#if WITH_EDITOR
    UBlueprint* BP = LoadObject<UBlueprint>(nullptr, *Path);
    if (!BP) return TEXT("Missing blueprint ") + Path;
    Out += TEXT("\nBLUEPRINT ") + Path + TEXT(" Parent=") + GetPathNameSafe(BP->ParentClass) + TEXT("\n");
    if (BP->SimpleConstructionScript)
        for (USCS_Node* N : BP->SimpleConstructionScript->GetAllNodes())
            Out += TEXT("COMPONENT ") + N->GetVariableName().ToString() + TEXT(" ") + GetPathNameSafe(N->ComponentClass) + TEXT("\n");
    if (BP->GeneratedClass)
    {
        UObject* CDO = BP->GeneratedClass->GetDefaultObject();
        for (TFieldIterator<FProperty> It(BP->GeneratedClass); It; ++It)
        {
            FString Value;
            It->ExportText_InContainer(0, Value, CDO, CDO, CDO, PPF_None);
            Out += TEXT("PROPERTY ") + It->GetName() + TEXT(" ") + It->GetCPPType() + TEXT(" = ") + Value + TEXT("\n");
        }
        for (TFieldIterator<UFunction> It(BP->GeneratedClass); It; ++It)
        {
            if (It->GetOwnerClass() == BP->GeneratedClass)
            {
                Out += TEXT("FUNCTION ") + It->GetName();
                for (TFieldIterator<FProperty> P(*It); P && P->HasAnyPropertyFlags(CPF_Parm); ++P)
                    Out += TEXT(" | ") + P->GetName() + TEXT(":") + P->GetCPPType();
                Out += TEXT("\n");
            }
        }
    }
    TArray<UEdGraph*> Graphs;
    BP->GetAllGraphs(Graphs);
    for (UEdGraph* G : Graphs)
    {
        Out += TEXT("GRAPH ") + G->GetName() + TEXT("\n");
        for (UEdGraphNode* N : G->Nodes)
        {
            Out += TEXT(" NODE ") + N->GetName() + TEXT(" ") + N->GetNodeTitle(ENodeTitleType::FullTitle).ToString().Replace(TEXT("\n"), TEXT(" ")) + TEXT("\n");
            for (UEdGraphPin* P : N->Pins)
            {
                Out += TEXT("  PIN ") + P->PinName.ToString() + TEXT(" = ") + P->DefaultValue + TEXT(" ") + GetPathNameSafe(P->DefaultObject);
                for (UEdGraphPin* L : P->LinkedTo) Out += TEXT(" -> ") + L->GetOwningNode()->GetName() + TEXT(".") + L->PinName.ToString();
                Out += TEXT("\n");
            }
        }
    }
#endif
    return Out;
}

#if WITH_EDITOR
namespace
{
    bool SaveEnemyAsset(UObject* Asset)
    {
        if (!Asset) return false;
        Asset->MarkPackageDirty();
        FSavePackageArgs Args;
        Args.TopLevelFlags = RF_Public | RF_Standalone;
        Args.SaveFlags = SAVE_NoError;
        return UPackage::SavePackage(Asset->GetOutermost(), Asset, *FPackageName::LongPackageNameToFilename(Asset->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension()), Args);
    }
    template<typename T> T* NewGraphNode(UEdGraph* Graph, int32 X, int32 Y)
    {
        FGraphNodeCreator<T> Creator(*Graph);
        T* Node = Creator.CreateNode();
        Node->NodePosX = X; Node->NodePosY = Y;
        Creator.Finalize();
        return Node;
    }
    UBlueprint* EnemyBlueprint(UClass* Parent, const FString& Name)
    {
        const FString Path = TEXT("/Game/Blueprints/Agents/EnemyINProgress/") + Name;
        if (UBlueprint* Existing = LoadObject<UBlueprint>(nullptr, *Path)) return Existing;
        return FKismetEditorUtilities::CreateBlueprint(Parent, CreatePackage(*Path), FName(*Name), BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
    }
    UAnimMontage* CompatibleMontage(UAnimMontage* Original, USkeleton* Skeleton, const FString& Folder)
    {
        if (!Original || Original->GetSkeleton() == Skeleton) return Original;
        const FString Name = TEXT("Progress_") + Original->GetName();
        if (auto* Existing = LoadObject<UAnimMontage>(nullptr, *(Folder/TEXT("Animations")/Name)))
        {
            // UE 5.5's sequencer-backed models also need their FK rig rebound.
            TArray<UAnimationAsset*> Referred;
            Existing->GetAllAnimationSequencesReferred(Referred);
            for (auto* Asset : Referred)
                if (auto* Sequence = Cast<UAnimSequence>(Asset))
                {
                    Sequence->GetController().UpdateWithSkeleton(Skeleton, false);
                    if (!SaveEnemyAsset(Sequence)) return nullptr;
                }
            return Existing->GetSkeleton() == Skeleton ? Existing : nullptr;
        }
        EditorAnimUtils::FNameDuplicationRule Rule;
        Rule.Prefix = TEXT("Progress_");
        Rule.FolderPath = Folder/TEXT("Animations");
        TArray<TWeakObjectPtr<UObject>> Assets = { Original };
        EditorAnimUtils::FAnimationRetargetContext Context(Assets, true, true);
        Context.DuplicateAssetsToRetarget(Skeleton->GetOutermost(), &Rule);
        for (UObject* Asset : Context.GetAllDuplicates())
            if (auto* Sequence = Cast<UAnimSequence>(Asset)) Sequence->GetController().UpdateWithSkeleton(Skeleton, false);
        Context.RetargetAnimations(Original->GetSkeleton(), Skeleton);
        for (UObject* Asset : Context.GetAllDuplicates()) if (!SaveEnemyAsset(Asset)) return nullptr;
        return Cast<UAnimMontage>(Context.GetDuplicate(Original));
    }
}
#endif

bool UProgressEnemyEditorLibrary::InstallHealthCompatibility()
{
#if WITH_EDITOR
    auto* BP = LoadObject<UBlueprint>(nullptr, TEXT("/Game/Blueprints/CombatSystem/Health/AC_EnemyHealth.AC_EnemyHealth"));
    if (!BP) return false;
    const TSet<FName> Events = { TEXT("ReceiveBeginPlay"), TEXT("ProcessDamage"), TEXT("Finisher"), TEXT("FinisherVis"), TEXT("FinisherVisOff"), TEXT("MoveEnemy") };
    TArray<UEdGraph*> Graphs;
    BP->GetAllGraphs(Graphs);
    int32 Patched = 0;
    for (auto* Graph : Graphs)
    {
        TArray<UEdGraphNode*> OriginalNodes = Graph->Nodes;
        for (auto* Node : OriginalNodes)
        {
            FName EventName;
            if (auto* Custom = Cast<UK2Node_CustomEvent>(Node)) EventName = Custom->CustomFunctionName;
            else if (auto* Event = Cast<UK2Node_Event>(Node)) EventName = Event->EventReference.GetMemberName();
            if (!Events.Contains(EventName)) continue;
            auto* Exec = Node->FindPin(UEdGraphSchema_K2::PN_Then);
            if (!Exec) return false;
            if (Exec->LinkedTo.Num() && Exec->LinkedTo[0]->GetOwningNode()->NodeComment == TEXT("ProgressEnemyCompatibility")) { ++Patched; continue; }
            const TArray<UEdGraphPin*> OriginalLinks = Exec->LinkedTo;
            Exec->BreakAllPinLinks();
            auto* Route = NewObject<UK2Node_CallFunction>(Graph);
            Graph->AddNode(Route, false, false);
            Route->CreateNewGuid();
            Route->SetFromFunction(UProgressEnemyCompatibility::StaticClass()->FindFunctionByName(TEXT("RouteHealthEvent")));
            Route->NodeComment = TEXT("ProgressEnemyCompatibility");
            Route->bCommentBubbleVisible = true;
            Route->NodePosX = Node->NodePosX + 300; Route->NodePosY = Node->NodePosY;
            Route->AllocateDefaultPins();
            Route->FindPinChecked(TEXT("EventName"))->DefaultValue = EventName.ToString();
            auto* Self = NewGraphNode<UK2Node_Self>(Graph, Route->NodePosX, Route->NodePosY+220);
            auto* Branch = NewGraphNode<UK2Node_IfThenElse>(Graph, Route->NodePosX+330, Route->NodePosY);
            const auto* Schema = Graph->GetSchema();
            bool Ok = Schema->TryCreateConnection(Exec, Route->GetExecPin());
            Ok &= Schema->TryCreateConnection(Self->FindPinChecked(UEdGraphSchema_K2::PN_Self), Route->FindPinChecked(TEXT("Component")));
            Ok &= Schema->TryCreateConnection(Route->GetThenPin(), Branch->GetExecPin());
            Ok &= Schema->TryCreateConnection(Route->GetReturnValuePin(), Branch->GetConditionPin());
            for (auto* Link : OriginalLinks) Ok &= Schema->TryCreateConnection(Branch->GetElsePin(), Link);
            for (FName Param : {FName(TEXT("DamageAmount")), FName(TEXT("DeathDelay"))})
                if (auto* Pin = Node->FindPin(Param)) Ok &= Schema->TryCreateConnection(Pin, Route->FindPinChecked(Param));
            if (!Ok) return false;
            ++Patched;
        }
    }
    if (Patched != Events.Num()) return false;
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FCompilerResultsLog Results;
    FKismetEditorUtilities::CompileBlueprint(BP, EBlueprintCompileOptions::None, &Results);
    return Results.NumErrors == 0 && SaveEnemyAsset(BP);
#else
    return false;
#endif
}

bool UProgressEnemyEditorLibrary::CreateProjectAssets()
{
#if WITH_EDITOR
    const FString Root = TEXT("/Game/Blueprints/Agents/EnemyINProgress/");
    auto* SourceBP = LoadObject<UBlueprint>(nullptr, TEXT("/Game/Blueprints/Agents/Enemy-Agent/EnemyAI/Enemy_punch/BP_Enemy_punch.BP_Enemy_punch"));
    auto* Source = SourceBP ? Cast<ACharacter>(SourceBP->GeneratedClass->GetDefaultObject()) : nullptr;
    auto* Blend = LoadObject<UBlendSpace>(nullptr, TEXT("/Game/Blueprints/Agents/Enemies/Animations/AI/BS_EnemyWalkRun.BS_EnemyWalkRun"));
    if (!Source || !Blend || !Source->GetMesh()->GetSkeletalMeshAsset()) return false;
    UAnimBlueprint* AnimBP = LoadObject<UAnimBlueprint>(nullptr, *(Root+TEXT("ABP_ProgressEnemy")));
    if (!AnimBP)
    {
        auto* Factory = NewObject<UAnimBlueprintFactory>();
        Factory->ParentClass = UProgressEnemyAnimInstance::StaticClass();
        Factory->TargetSkeleton = Source->GetMesh()->GetSkeletalMeshAsset()->GetSkeleton();
        AnimBP = Cast<UAnimBlueprint>(Factory->FactoryCreateNew(UAnimBlueprint::StaticClass(), CreatePackage(*(Root+TEXT("ABP_ProgressEnemy"))), TEXT("ABP_ProgressEnemy"), RF_Public|RF_Standalone, nullptr, GWarn));
        if (!AnimBP) return false;
        UEdGraph* Graph = FBlueprintEditorUtils::FindEventGraph(AnimBP);
        TArray<UEdGraph*> Graphs;
        AnimBP->GetAllGraphs(Graphs);
        for (auto* G : Graphs) if (G->GetFName() == TEXT("AnimGraph")) Graph = G;
        if (!Graph || Graph->GetFName() != TEXT("AnimGraph")) return false;
        UAnimGraphNode_Root* RootNode = nullptr;
        for (UEdGraphNode* N : Graph->Nodes) if (auto* R = Cast<UAnimGraphNode_Root>(N)) RootNode = R;
        if (!RootNode) RootNode = NewGraphNode<UAnimGraphNode_Root>(Graph, 600, 0);
        auto* Player = NewObject<UAnimGraphNode_BlendSpacePlayer>(Graph);
        Graph->AddNode(Player, false, false);
        Player->CreateNewGuid();
        Player->SetAnimationAsset(Blend);
        Player->NodePosX = 0;
        Player->AllocateDefaultPins();
        auto* Slot = NewGraphNode<UAnimGraphNode_Slot>(Graph, 320, 0);
        Slot->Node.SlotName = TEXT("DefaultSlot");
        auto* Speed = NewObject<UK2Node_VariableGet>(Graph);
        Graph->AddNode(Speed, false, false);
        Speed->CreateNewGuid();
        Speed->VariableReference.SetExternalMember(TEXT("Speed"), UProgressEnemyAnimInstance::StaticClass());
        Speed->NodePosX = -250;
        Speed->AllocateDefaultPins();
        const auto* Schema = Graph->GetSchema();
        if (!Schema->TryCreateConnection(Player->FindPinChecked(TEXT("Pose")), Slot->FindPinChecked(TEXT("Source"))) ||
            !Schema->TryCreateConnection(Slot->FindPinChecked(TEXT("Pose")), RootNode->FindPinChecked(TEXT("Result"))) ||
            !Schema->TryCreateConnection(Speed->FindPinChecked(TEXT("Speed")), Player->FindPinChecked(TEXT("X")))) return false;
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(AnimBP);
    }
    FCompilerResultsLog AnimResults;
    FKismetEditorUtilities::CompileBlueprint(AnimBP, EBlueprintCompileOptions::None, &AnimResults);
    if (AnimResults.NumErrors || !SaveEnemyAsset(AnimBP)) return false;
    auto* EnemyBP = EnemyBlueprint(AProgressEnemyCharacter::StaticClass(), TEXT("BP_ProgressEnemy"));
    if (!EnemyBP) return false;
    auto* Enemy = Cast<AProgressEnemyCharacter>(EnemyBP->GeneratedClass->GetDefaultObject());
    Enemy->GetMesh()->SetSkeletalMeshAsset(Source->GetMesh()->GetSkeletalMeshAsset());
    Enemy->GetMesh()->SetRelativeTransform(Source->GetMesh()->GetRelativeTransform());
    Enemy->GetMesh()->SetAnimInstanceClass(AnimBP->GeneratedClass);
    Enemy->MeleeMontages.Empty();
    for (const TCHAR* Name : { TEXT("Cross_Punch_Montage"), TEXT("Hook_Punch_Montage"), TEXT("Punching_Montage") })
        if (auto* Montage = LoadObject<UAnimMontage>(nullptr, *(FString(TEXT("/Game/Blueprints/Agents/Enemies/Animations/AI/"))+Name))) Enemy->MeleeMontages.Add(Montage);
    // No block animation exists in this checkout; use a clearly named, rig-compatible placeholder.
    UAnimSequence* Block = LoadObject<UAnimSequence>(nullptr, *(Root+TEXT("AS_BlockPlaceholder")));
    if (!Block)
    {
        auto* Idle = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Blueprints/Agents/Enemies/Animations/AI/Raw/Idle_1_.Idle_1_"));
        if (!Idle) return false;
        Block = DuplicateObject<UAnimSequence>(Idle, CreatePackage(*(Root+TEXT("AS_BlockPlaceholder"))), TEXT("AS_BlockPlaceholder"));
        Block->SetFlags(RF_Public|RF_Standalone);
        if (!SaveEnemyAsset(Block)) return false;
    }
    Enemy->BlockAnimation = Block;
    Enemy->HitReactionMontage = LoadObject<UAnimMontage>(nullptr, TEXT("/Game/Blueprints/CombatSystem/Animations/EnemyHitReactionSide_Montage.EnemyHitReactionSide_Montage"));
    Enemy->EnemyFinisherMontage = LoadObject<UAnimMontage>(nullptr, TEXT("/Game/Blueprints/CombatSystem/Animations/EnemyFinisherStruggleSuperrMontageGayMan.EnemyFinisherStruggleSuperrMontageGayMan"));
    USkeleton* EnemySkeleton = Enemy->GetMesh()->GetSkeletalMeshAsset()->GetSkeleton();
    Enemy->HitReactionMontage = CompatibleMontage(Enemy->HitReactionMontage, EnemySkeleton, Root);
    Enemy->EnemyFinisherMontage = CompatibleMontage(Enemy->EnemyFinisherMontage, EnemySkeleton, Root);
    if (!Enemy->HitReactionMontage || !Enemy->EnemyFinisherMontage) return false;
    Enemy->PlayerFinisherMontage = LoadObject<UAnimMontage>(nullptr, TEXT("/Game/Blueprints/CombatSystem/Animations/Superhuman_Choke_Lift_Montage.Superhuman_Choke_Lift_Montage"));
    if (!SaveEnemyAsset(EnemyBP)) return false;
    auto* SpawnerBP = EnemyBlueprint(AProgressEnemySpawner::StaticClass(), TEXT("BP_ProgressEnemySpawner"));
    if (!SpawnerBP) return false;
    Cast<AProgressEnemySpawner>(SpawnerBP->GeneratedClass->GetDefaultObject())->EnemyClass = EnemyBP->GeneratedClass;
    return SaveEnemyAsset(SpawnerBP);
#else
    return false;
#endif
}
