#include "ProgressEnemyEditorLibrary.h"
#if WITH_EDITOR
#include "Engine/Blueprint.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimInstance.h"
#include "AnimBlueprintExtension.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "EdGraphUtilities.h"
#include "K2Node_CallFunction.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#endif

FString UProgressEnemyEditorLibrary::MatchPunchAnimationSetup()
{
#if WITH_EDITOR
    const TCHAR* Root=TEXT("/Game/Blueprints/Agents/EnemyINProgress/BlueprintOnly/");
    auto* ReferenceBP=LoadObject<UBlueprint>(nullptr,TEXT("/Game/Blueprints/Agents/Enemy-Agent/EnemyAI/Enemy_punch/BP_Enemy_punch.BP_Enemy_punch"));
    auto* BP=LoadObject<UBlueprint>(nullptr,*(FString(Root)+TEXT("BP_Enemy")));
    auto* Anim=LoadObject<UAnimBlueprint>(nullptr,*(FString(Root)+TEXT("ABP_Enemy")));
    auto* Reference=ReferenceBP ? Cast<ACharacter>(ReferenceBP->GeneratedClass->GetDefaultObject()) : nullptr;
    auto* ReferenceAnim=Reference ? Cast<UAnimBlueprint>(Reference->GetMesh()->GetAnimClass()->ClassGeneratedBy) : nullptr;
    if (!BP || !Anim || !Reference || !ReferenceAnim) return TEXT("FAIL missing animation reference");
    const auto* Schema=GetDefault<UEdGraphSchema_K2>();
    auto Save=[](UObject* Object)
    {
        FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone; Args.SaveFlags=SAVE_NoError;
        return UPackage::SavePackage(Object->GetOutermost(),Object,*FPackageName::LongPackageNameToFilename(Object->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension()),Args);
    };
    for (FName Name : {FName(TEXT("bIsInAir")),FName(TEXT("IsMoving"))})
        if (FBlueprintEditorUtils::FindNewVariableIndex(Anim,Name)==INDEX_NONE)
        {
            FEdGraphPinType Type; Type.PinCategory=UEdGraphSchema_K2::PC_Boolean;
            FBlueprintEditorUtils::AddMemberVariable(Anim,Name,Type,TEXT("false"));
        }
    if (FBlueprintEditorUtils::FindNewVariableIndex(Anim,TEXT("Velocity"))==INDEX_NONE)
    {
        FEdGraphPinType Type; Type.PinCategory=UEdGraphSchema_K2::PC_Real; Type.PinSubCategory=UEdGraphSchema_K2::PC_Double;
        FBlueprintEditorUtils::AddMemberVariable(Anim,TEXT("Velocity"),Type,TEXT("0"));
    }
    FKismetEditorUtilities::CompileBlueprint(Anim);
    UEdGraph* SourceGraph=nullptr;
    for (UEdGraph* Graph:ReferenceAnim->FunctionGraphs) if (Graph->GetFName()==TEXT("AnimGraph")) SourceGraph=Graph;
    if (!SourceGraph) return TEXT("FAIL reference AnimGraph missing");
    TArray<UEdGraph*> Previous=Anim->FunctionGraphs;
    for (UEdGraph* Graph:Previous) if (Graph->GetFName()==TEXT("AnimGraph"))
    {
        Anim->FunctionGraphs.Remove(Graph);
        Graph->Rename(nullptr,GetTransientPackage(),REN_DontCreateRedirectors|REN_NonTransactional);
    }
    FCompilerResultsLog CloneLog;
    UEdGraph* Clone=FEdGraphUtilities::CloneGraph(SourceGraph,Anim,&CloneLog,false);
    Clone->Rename(TEXT("AnimGraph"),Anim,REN_DontCreateRedirectors|REN_NonTransactional);
    Anim->FunctionGraphs.Add(Clone);
    TArray<UEdGraph*> Graphs; Anim->GetAllGraphs(Graphs);
    for (UEdGraph* Graph:Graphs) if (Graph!=FBlueprintEditorUtils::FindEventGraph(Anim))
        for (UEdGraphNode* Node:Graph->Nodes) if (auto* Get=Cast<UK2Node_VariableGet>(Node))
        {
            FName Name=Get->VariableReference.GetMemberName();
            Get->VariableReference.SetSelfMember(Name);
            Get->ReconstructNode();
        }
    // Keep the generic owner update, rather than copying casts to the old enemy classes.
    UEdGraph* EventGraph=FBlueprintEditorUtils::FindEventGraph(Anim);
    UK2Node_VariableSet* SetSpeed=nullptr;
    UK2Node_CallFunction* Owner=nullptr;
    for (UEdGraphNode* Node:EventGraph->Nodes)
    {
        if (auto* Set=Cast<UK2Node_VariableSet>(Node)) if (Set->VariableReference.GetMemberName()==TEXT("Speed")) SetSpeed=Set;
        if (auto* Call=Cast<UK2Node_CallFunction>(Node)) if (Call->FunctionReference.GetMemberName()==TEXT("TryGetPawnOwner")) Owner=Call;
    }
    if (!SetSpeed || !Owner) return TEXT("FAIL generic animation update missing");
    auto Variable=[&](UEdGraph* G,FName Name,bool Set,UClass* External=nullptr)->UK2Node_Variable*
    {
        UK2Node_Variable* Node=Set ? static_cast<UK2Node_Variable*>(NewObject<UK2Node_VariableSet>(G)) : static_cast<UK2Node_Variable*>(NewObject<UK2Node_VariableGet>(G));
        G->AddNode(Node,false,false); Node->CreateNewGuid();
        if (External) Node->VariableReference.SetExternalMember(Name,External); else Node->VariableReference.SetSelfMember(Name);
        Node->AllocateDefaultPins(); return Node;
    };
    auto Call=[&](UEdGraph* G,UClass* Class,FName Name)->UK2Node_CallFunction*
    {
        auto* Node=NewObject<UK2Node_CallFunction>(G); G->AddNode(Node,false,false); Node->CreateNewGuid();
        Node->SetFromFunction(Class->FindFunctionByName(Name)); Node->AllocateDefaultPins(); return Node;
    };
    // Re-running this patch should replace its update tail rather than append another one.
    TArray<UEdGraphNode*> Existing=EventGraph->Nodes;
    for (UEdGraphNode* Node:Existing) if (Node->NodeComment==TEXT("PunchReferenceUpdate")) FBlueprintEditorUtils::RemoveNode(Anim,Node,true);
    auto* Character=NewObject<UK2Node_DynamicCast>(EventGraph); EventGraph->AddNode(Character,false,false); Character->CreateNewGuid(); Character->TargetType=ACharacter::StaticClass(); Character->SetPurity(true); Character->AllocateDefaultPins();
    auto* Movement=Variable(EventGraph,TEXT("CharacterMovement"),false,ACharacter::StaticClass());
    auto* Mode=Variable(EventGraph,TEXT("MovementMode"),false,UCharacterMovementComponent::StaticClass());
    auto* Falling=Call(EventGraph,UKismetMathLibrary::StaticClass(),TEXT("EqualEqual_ByteByte"));
    Schema->TrySetDefaultValue(*Falling->FindPinChecked(TEXT("B")),TEXT("3"));
    auto* SetAir=Variable(EventGraph,TEXT("bIsInAir"),true);
    auto* Speed=Variable(EventGraph,TEXT("Speed"),false);
    auto* SetVelocity=Variable(EventGraph,TEXT("Velocity"),true);
    auto* Moving=Call(EventGraph,UKismetMathLibrary::StaticClass(),TEXT("Greater_DoubleDouble"));
    Schema->TrySetDefaultValue(*Moving->FindPinChecked(TEXT("B")),TEXT("0"));
    auto* SetMoving=Variable(EventGraph,TEXT("IsMoving"),true);
    TArray<UEdGraphNode*> Tail={Character,Movement,Mode,Falling,SetAir,Speed,SetVelocity,Moving,SetMoving};
    int32 Position=0; for (auto* Node:Tail) { Node->NodeComment=TEXT("PunchReferenceUpdate"); Node->NodePosX=900+(Position%3)*320; Node->NodePosY=350+(Position/3)*220; ++Position; }
    bool Ok=true;
    Ok &= Schema->TryCreateConnection(Owner->GetReturnValuePin(),Character->GetCastSourcePin());
    Ok &= Schema->TryCreateConnection(Character->GetCastResultPin(),Movement->FindPinChecked(TEXT("self")));
    Ok &= Schema->TryCreateConnection(Movement->FindPinChecked(TEXT("CharacterMovement")),Mode->FindPinChecked(TEXT("self")));
    Ok &= Schema->TryCreateConnection(Mode->FindPinChecked(TEXT("MovementMode")),Falling->FindPinChecked(TEXT("A")));
    Ok &= Schema->TryCreateConnection(Falling->GetReturnValuePin(),SetAir->FindPinChecked(TEXT("bIsInAir")));
    Ok &= Schema->TryCreateConnection(Speed->FindPinChecked(TEXT("Speed")),Moving->FindPinChecked(TEXT("A")));
    Ok &= Schema->TryCreateConnection(Speed->FindPinChecked(TEXT("Speed")),SetVelocity->FindPinChecked(TEXT("Velocity")));
    Ok &= Schema->TryCreateConnection(Moving->GetReturnValuePin(),SetMoving->FindPinChecked(TEXT("IsMoving")));
    SetSpeed->FindPinChecked(TEXT("then"))->BreakAllPinLinks();
    Ok &= Schema->TryCreateConnection(SetSpeed->FindPinChecked(TEXT("then")),SetVelocity->FindPinChecked(TEXT("execute")));
    Ok &= Schema->TryCreateConnection(SetVelocity->FindPinChecked(TEXT("then")),SetAir->FindPinChecked(TEXT("execute")));
    Ok &= Schema->TryCreateConnection(SetAir->FindPinChecked(TEXT("then")),SetMoving->FindPinChecked(TEXT("execute")));
    if (!Ok) return TEXT("FAIL connecting reference animation update");
    Anim->TargetSkeleton=Reference->GetMesh()->GetSkeletalMeshAsset()->GetSkeleton();
    for (auto* Extension:UAnimBlueprintExtension::GetExtensions(ReferenceAnim))
        UAnimBlueprintExtension::RequestExtension(Anim,Extension->GetClass());
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Anim);
    FCompilerResultsLog AnimLog; FKismetEditorUtilities::CompileBlueprint(Anim,EBlueprintCompileOptions::None,&AnimLog);
    if (AnimLog.NumErrors) return FString::Printf(TEXT("FAIL animation compile errors=%d"),AnimLog.NumErrors);

    // Cache the selected montage so its duration and the played asset cannot randomly differ.
    if (FBlueprintEditorUtils::FindNewVariableIndex(BP,TEXT("ActiveMeleeMontage"))==INDEX_NONE)
    {
        FEdGraphPinType Type; Type.PinCategory=UEdGraphSchema_K2::PC_Object; Type.PinSubCategoryObject=UAnimMontage::StaticClass();
        FBlueprintEditorUtils::AddMemberVariable(BP,TEXT("ActiveMeleeMontage"),Type);
        FKismetEditorUtilities::CompileBlueprint(BP);
    }
    UEdGraph* Attack=nullptr;
    for (UEdGraph* Graph:BP->FunctionGraphs) if (Graph->GetFName()==TEXT("TryAttack")) Attack=Graph;
    if (!Attack) return TEXT("FAIL TryAttack graph missing");
    UK2Node_CallFunction* Play=nullptr;
    bool AlreadyPatched=false;
    for (UEdGraphNode* Node:Attack->Nodes)
    {
        if (Node->NodeComment==TEXT("FullPunchMontageDuration")) AlreadyPatched=true;
        if (auto* C=Cast<UK2Node_CallFunction>(Node)) if (C->FunctionReference.GetMemberName()==TEXT("PlayAnimMontage")) Play=C;
    }
    if (!Play) return TEXT("FAIL melee montage play node missing");
    if (!AlreadyPatched)
    {
        auto* Cache=Variable(Attack,TEXT("ActiveMeleeMontage"),true);
        auto* Selected=Variable(Attack,TEXT("ActiveMeleeMontage"),false);
        auto* Length=Call(Attack,UAnimSequenceBase::StaticClass(),TEXT("GetPlayLength"));
        auto* Rate=Variable(Attack,TEXT("AnimationRate"),false);
        auto* Duration=Call(Attack,UKismetMathLibrary::StaticClass(),TEXT("Divide_DoubleDouble"));
        auto* Padding=Call(Attack,UKismetMathLibrary::StaticClass(),TEXT("Add_DoubleDouble")); Schema->TrySetDefaultValue(*Padding->FindPinChecked(TEXT("B")),TEXT("0.1"));
        auto* OldTime=Variable(Attack,TEXT("ActionTime"),false);
        auto* Maximum=Call(Attack,UKismetMathLibrary::StaticClass(),TEXT("FMax"));
        auto* SetTime=Variable(Attack,TEXT("ActionTime"),true);
        for (auto* Node:TArray<UEdGraphNode*>{Cache,Selected,Length,Rate,Duration,Padding,OldTime,Maximum,SetTime}) Node->NodeComment=TEXT("FullPunchMontageDuration");
        UEdGraphPin* Asset=Play->FindPinChecked(TEXT("AnimMontage"));
        const auto AssetSources=Asset->LinkedTo; Asset->BreakAllPinLinks();
        for (auto* Source:AssetSources) Ok &= Schema->TryCreateConnection(Source,Cache->FindPinChecked(TEXT("ActiveMeleeMontage")));
        const auto ExecSources=Play->GetExecPin()->LinkedTo; Play->GetExecPin()->BreakAllPinLinks();
        for (auto* Source:ExecSources) Ok &= Schema->TryCreateConnection(Source,Cache->FindPinChecked(TEXT("execute")));
        Ok &= Schema->TryCreateConnection(Cache->FindPinChecked(TEXT("then")),Play->GetExecPin());
        Ok &= Schema->TryCreateConnection(Selected->FindPinChecked(TEXT("ActiveMeleeMontage")),Asset);
        Ok &= Schema->TryCreateConnection(Selected->FindPinChecked(TEXT("ActiveMeleeMontage")),Length->FindPinChecked(TEXT("self")));
        Ok &= Schema->TryCreateConnection(Length->GetReturnValuePin(),Duration->FindPinChecked(TEXT("A")));
        Ok &= Schema->TryCreateConnection(Rate->FindPinChecked(TEXT("AnimationRate")),Duration->FindPinChecked(TEXT("B")));
        Ok &= Schema->TryCreateConnection(Duration->GetReturnValuePin(),Padding->FindPinChecked(TEXT("A")));
        Ok &= Schema->TryCreateConnection(OldTime->FindPinChecked(TEXT("ActionTime")),Maximum->FindPinChecked(TEXT("A")));
        Ok &= Schema->TryCreateConnection(Padding->GetReturnValuePin(),Maximum->FindPinChecked(TEXT("B")));
        Ok &= Schema->TryCreateConnection(Maximum->GetReturnValuePin(),SetTime->FindPinChecked(TEXT("ActionTime")));
        const auto Next=Play->GetThenPin()->LinkedTo; Play->GetThenPin()->BreakAllPinLinks();
        Ok &= Schema->TryCreateConnection(Play->GetThenPin(),SetTime->FindPinChecked(TEXT("execute")));
        for (auto* Destination:Next) Ok &= Schema->TryCreateConnection(SetTime->FindPinChecked(TEXT("then")),Destination);
    }
    if (!Ok) return TEXT("FAIL connecting montage duration");
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FCompilerResultsLog EnemyLog; FKismetEditorUtilities::CompileBlueprint(BP,EBlueprintCompileOptions::None,&EnemyLog);
    if (EnemyLog.NumErrors) return FString::Printf(TEXT("FAIL enemy compile errors=%d"),EnemyLog.NumErrors);
    auto* Enemy=Cast<ACharacter>(BP->GeneratedClass->GetDefaultObject());
    Enemy->GetMesh()->SetSkeletalMeshAsset(Reference->GetMesh()->GetSkeletalMeshAsset());
    Enemy->GetMesh()->SetRelativeTransform(Reference->GetMesh()->GetRelativeTransform());
    Enemy->GetMesh()->SetAnimationMode(Reference->GetMesh()->GetAnimationMode());
    Enemy->GetMesh()->SetAnimInstanceClass(Anim->GeneratedClass);
    Enemy->GetMesh()->VisibilityBasedAnimTickOption=Reference->GetMesh()->VisibilityBasedAnimTickOption;
    Enemy->GetCapsuleComponent()->SetCapsuleSize(Reference->GetCapsuleComponent()->GetUnscaledCapsuleRadius(),Reference->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight());
    if (!Save(Anim) || !Save(BP)) return TEXT("FAIL saving animation fix");
    return FString::Printf(TEXT("PASS\nMatched %s AnimGraph, mesh transform, capsule and animation tick policy.\nKept generic Blueprint owner updates and block placeholder.\nMelee recovery now waits for selected montage length / play rate.\nAnim errors=%d warnings=%d; enemy errors=%d warnings=%d\n"),*ReferenceBP->GetName(),AnimLog.NumErrors,AnimLog.NumWarnings,EnemyLog.NumErrors,EnemyLog.NumWarnings);
#else
    return TEXT("Editor only");
#endif
}
