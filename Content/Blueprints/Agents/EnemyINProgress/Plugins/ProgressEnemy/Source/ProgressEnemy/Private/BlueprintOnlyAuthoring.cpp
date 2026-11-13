#include "ProgressEnemyEditorLibrary.h"
#if WITH_EDITOR
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_CallFunction.h"
#include "K2Node_CallArrayFunction.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_Event.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_Self.h"
#include "K2Node_MakeArray.h"
#include "K2Node_SpawnActorFromClass.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_EnhancedInputAction.h"
#include "InputAction.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Factories/AnimBlueprintFactory.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimBlueprintGeneratedClass.h"
#include "AnimGraphNode_Root.h"
#include "AnimGraphNode_Slot.h"
#include "AnimGraphNode_BlendSpacePlayer.h"
#include "Animation/BlendSpace.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/UnrealType.h"

namespace BPOnlyAuthoring
{
using J = TSharedPtr<FJsonObject>;
FString Str(J O, const TCHAR* K, const FString& Default = TEXT("")) { FString V; return O->TryGetStringField(K,V) ? V : Default; }
bool Flag(J O, const TCHAR* K) { bool B=false; O->TryGetBoolField(K,B); return B; }
TArray<TSharedPtr<FJsonValue>> List(J O, const TCHAR* K) { const TArray<TSharedPtr<FJsonValue>>* A; return O->TryGetArrayField(K,A) ? *A : TArray<TSharedPtr<FJsonValue>>(); }
J Obj(J O, const TCHAR* K) { const TSharedPtr<FJsonObject>* A; return O->TryGetObjectField(K,A) ? *A : MakeShared<FJsonObject>(); }
UClass* Class(const FString& P) { return LoadObject<UClass>(nullptr, *P); }
FEdGraphPinType Type(J O)
{
    FEdGraphPinType T;
    FString Kind = Str(O,TEXT("type"));
    if (Kind == TEXT("double") || Kind == TEXT("float")) { T.PinCategory = UEdGraphSchema_K2::PC_Real; T.PinSubCategory = Kind == TEXT("float") ? UEdGraphSchema_K2::PC_Float : UEdGraphSchema_K2::PC_Double; }
    else if (Kind == TEXT("vector")) { T.PinCategory=UEdGraphSchema_K2::PC_Struct; T.PinSubCategoryObject=TBaseStructure<FVector>::Get(); }
    else if (Kind == TEXT("object") || Kind == TEXT("class")) { T.PinCategory= Kind==TEXT("object") ? UEdGraphSchema_K2::PC_Object : UEdGraphSchema_K2::PC_Class; T.PinSubCategoryObject=Class(Str(O,TEXT("class"))); }
    else if (Kind == TEXT("struct")) { T.PinCategory=UEdGraphSchema_K2::PC_Struct; T.PinSubCategoryObject=LoadObject<UScriptStruct>(nullptr,*Str(O,TEXT("struct"))); }
    else if (Kind == TEXT("enum")) { T.PinCategory=UEdGraphSchema_K2::PC_Byte; T.PinSubCategoryObject=LoadObject<UEnum>(nullptr,*Str(O,TEXT("enum"))); }
    else T.PinCategory = FName(*Kind);
    if (Flag(O,TEXT("array"))) T.ContainerType = EPinContainerType::Array;
    return T;
}
void Properties(UObject* O, J Props, FString& Errors)
{
    for (auto& Pair : Props->Values)
    {
        FProperty* P=O ? O->GetClass()->FindPropertyByName(FName(*Pair.Key)) : nullptr;
        if (!P) { Errors += TEXT("Missing property ")+GetNameSafe(O)+TEXT(".")+Pair.Key+TEXT("\n"); continue; }
        if (!P->ImportText_Direct(*Pair.Value->AsString(),P->ContainerPtrToValuePtr<void>(O),O,PPF_None)) Errors += TEXT("Property import failed ")+Pair.Key+TEXT("\n");
    }
}
bool Save(UObject* O)
{
    FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone; Args.SaveFlags=SAVE_NoError;
    return UPackage::SavePackage(O->GetOutermost(),O,*FPackageName::LongPackageNameToFilename(O->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension()),Args);
}
struct FGraph { J Spec; UEdGraph* Graph=nullptr; UK2Node_FunctionEntry* Entry=nullptr; UK2Node_FunctionResult* Result=nullptr; TMap<FString, UEdGraphNode*> Nodes; };
struct FAsset { J Spec; UBlueprint* BP=nullptr; TArray<FGraph> Graphs; };
template<class T> T* NewNode(UEdGraph* G)
{
    auto* N=NewObject<T>(G); G->AddNode(N,false,false); N->CreateNewGuid(); return N;
}
void SetDefaults(UEdGraphNode* N, J D, FString& Errors)
{
    const auto* Schema=GetDefault<UEdGraphSchema_K2>();
    for (auto& Pair : D->Values)
    {
        UEdGraphPin* P=N->FindPin(FName(*Pair.Key));
        if (!P) { Errors+=TEXT("Missing default pin ")+N->GetName()+TEXT(".")+Pair.Key+TEXT("\n"); continue; }
        FString V=Pair.Value->AsString();
        if (P->PinType.PinCategory==UEdGraphSchema_K2::PC_Object || P->PinType.PinCategory==UEdGraphSchema_K2::PC_Class)
            Schema->TrySetDefaultObject(*P,V.IsEmpty() ? nullptr : StaticLoadObject(UObject::StaticClass(),nullptr,*V));
        else Schema->TrySetDefaultValue(*P,V);
    }
}
UEdGraphPin* Pin(FGraph& G,const FString& S,FString& Errors)
{
    FString Id,Name; S.Split(TEXT("."),&Id,&Name);
    auto* N=G.Nodes.FindRef(Id);
    UEdGraphPin* P=N ? N->FindPin(FName(*Name)) : nullptr;
    if (!P && Name==TEXT("As")) if (auto* CastNode=Cast<UK2Node_DynamicCast>(N)) P=CastNode->GetCastResultPin();
    if (!P) Errors+=TEXT("Missing pin ")+S+TEXT(" in ")+G.Graph->GetName()+TEXT("\n");
    return P;
}
}
#endif

FString UProgressEnemyEditorLibrary::BuildBlueprintOnly(const FString& SpecificationFile)
{
#if WITH_EDITOR
    using namespace BPOnlyAuthoring;
    FString Source, Errors, Report;
    if (!FFileHelper::LoadFileToString(Source,*SpecificationFile)) return TEXT("FAIL reading specification");
    J Root; if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Source),Root)) return TEXT("FAIL parsing specification");
    LoadObject<UBlueprint>(nullptr,TEXT("/Engine/EditorBlueprintResources/StandardMacros.StandardMacros"));
    TArray<FAsset> Assets;
    // First establish all classes so cross-asset variables and functions can reference them.
    for (auto V : List(Root,TEXT("assets")))
    {
        FAsset A; A.Spec=V->AsObject(); FString Path=Str(A.Spec,TEXT("path"));
        A.BP=LoadObject<UBlueprint>(nullptr,*Path);
        if (Flag(A.Spec,TEXT("patch")))
        {
            if (!A.BP) return TEXT("FAIL patch target missing: ")+Path;
            // Replace only named functions; keep components, animation graphs and user defaults.
            for (auto G : List(A.Spec,TEXT("graphs")))
            {
                const FString Name=Str(G->AsObject(),TEXT("name"));
                if (Name==TEXT("EventGraph")) return TEXT("FAIL patching event graphs is not supported");
                const auto Existing=A.BP->FunctionGraphs;
                for (UEdGraph* Graph:Existing) if (Graph->GetName()==Name)
                {
                    FBlueprintEditorUtils::RemoveGraph(A.BP,Graph);
                    Graph->Rename(nullptr,GetTransientPackage(),REN_DontCreateRedirectors|REN_ForceNoResetLoaders);
                }
            }
            Assets.Add(MoveTemp(A));
            continue;
        }
        if (A.BP) return TEXT("FAIL asset already exists; author into a fresh folder: ")+Path;
        UClass* Parent=Class(Str(A.Spec,TEXT("parent")));
        if (!Parent) return TEXT("FAIL parent ")+Str(A.Spec,TEXT("parent"));
        if (Flag(A.Spec,TEXT("animation")))
        {
            auto* Factory=NewObject<UAnimBlueprintFactory>(); Factory->ParentClass=Parent;
            auto* Mesh=LoadObject<USkeletalMesh>(nullptr,*Str(A.Spec,TEXT("mesh")));
            Factory->TargetSkeleton=Mesh ? Mesh->GetSkeleton() : nullptr;
            A.BP=Cast<UBlueprint>(Factory->FactoryCreateNew(UAnimBlueprint::StaticClass(),CreatePackage(*Path),FName(*FPackageName::GetShortName(Path)),RF_Public|RF_Standalone,nullptr,GWarn));
        }
        else A.BP=FKismetEditorUtilities::CreateBlueprint(Parent,CreatePackage(*Path),FName(*FPackageName::GetShortName(Path)),BPTYPE_Normal,UBlueprint::StaticClass(),UBlueprintGeneratedClass::StaticClass());
        if (!A.BP) return TEXT("FAIL creating ")+Path;
        Assets.Add(MoveTemp(A));
    }
    for (auto& A : Assets)
    {
        for (auto V : List(A.Spec,TEXT("variables")))
        {
            J Var=V->AsObject(); FName Name(*Str(Var,TEXT("name")));
            if (A.BP->NewVariables.ContainsByPredicate([&](const FBPVariableDescription& D){return D.VarName==Name;})) continue;
            FBlueprintEditorUtils::AddMemberVariable(A.BP,Name,Type(Var),Str(Var,TEXT("default")));
            for (auto& Description : A.BP->NewVariables) if (Description.VarName==Name)
            {
                Description.PropertyFlags|=CPF_Edit|CPF_BlueprintVisible;
                Description.PropertyFlags&=~CPF_DisableEditOnInstance;
            }
            FBlueprintEditorUtils::SetBlueprintVariableCategory(A.BP,Name,nullptr,FText::FromString(Str(Var,TEXT("category"),TEXT("Enemy"))));
        }
        for (auto V : List(A.Spec,TEXT("graphs")))
        {
            FGraph G; G.Spec=V->AsObject(); FString Name=Str(G.Spec,TEXT("name"));
            if (Name==TEXT("EventGraph")) G.Graph=FBlueprintEditorUtils::FindEventGraph(A.BP);
            else
            {
                G.Graph=FBlueprintEditorUtils::CreateNewGraph(A.BP,FName(*Name),UEdGraph::StaticClass(),UEdGraphSchema_K2::StaticClass());
                FBlueprintEditorUtils::AddFunctionGraph(A.BP,G.Graph,false,static_cast<UFunction*>(nullptr));
                for (UEdGraphNode* N:G.Graph->Nodes) if (auto* E=Cast<UK2Node_FunctionEntry>(N)) G.Entry=E;
                if (!G.Entry) return TEXT("FAIL creating function ")+Name;
                G.Entry->AddExtraFlags(FUNC_Public|FUNC_BlueprintCallable|(Flag(G.Spec,TEXT("pure")) ? FUNC_BlueprintPure : 0));
                G.Entry->MetaData.Category=FText::FromString(Str(G.Spec,TEXT("category"),TEXT("Enemy")));
                for (auto P : List(G.Spec,TEXT("inputs"))) G.Entry->CreateUserDefinedPin(FName(*Str(P->AsObject(),TEXT("name"))),Type(P->AsObject()),EGPD_Output);
                G.Nodes.Add(TEXT("entry"),G.Entry);
                if (List(G.Spec,TEXT("outputs")).Num() || Flag(G.Spec,TEXT("return")))
                {
                    G.Result=NewNode<UK2Node_FunctionResult>(G.Graph); G.Result->FunctionReference=G.Entry->FunctionReference; G.Result->AllocateDefaultPins();
                    for (auto P : List(G.Spec,TEXT("outputs"))) G.Result->CreateUserDefinedPin(FName(*Str(P->AsObject(),TEXT("name"))),Type(P->AsObject()),EGPD_Input);
                    G.Nodes.Add(TEXT("result"),G.Result);
                }
            }
            A.Graphs.Add(MoveTemp(G));
        }
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(A.BP);
        FKismetEditorUtilities::CompileBlueprint(A.BP);
    }
    // Add components after all Blueprint component classes have their signatures.
    for (auto& A : Assets)
    {
        for (auto V : List(A.Spec,TEXT("components")))
        {
            J C=V->AsObject(); UClass* Cls=Class(Str(C,TEXT("class")));
            if (!Cls || !A.BP->SimpleConstructionScript) return TEXT("FAIL component ")+Str(C,TEXT("name"));
            USCS_Node* N=A.BP->SimpleConstructionScript->CreateNode(Cls,FName(*Str(C,TEXT("name"))));
            A.BP->SimpleConstructionScript->AddNode(N);
            Properties(N->ComponentTemplate,Obj(C,TEXT("properties")),Errors);
        }
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(A.BP);
        FKismetEditorUtilities::CompileBlueprint(A.BP);
    }
    for (auto& A:Assets) for (auto& G:A.Graphs) for (auto V:List(G.Spec,TEXT("nodes")))
    {
        J S=V->AsObject();
        if (Str(S,TEXT("kind"))!=TEXT("call")) continue;
        FString Owner=Str(S,TEXT("owner")); UClass* Cls=Owner.IsEmpty() ? A.BP->GeneratedClass.Get() : Class(Owner);
        if (!Cls || !Cls->FindFunctionByName(FName(*Str(S,TEXT("function"))))) Errors+=TEXT("Missing function ")+Owner+TEXT(":")+Str(S,TEXT("function"))+TEXT("\n");
    }
    if (!Errors.IsEmpty()) return TEXT("FAIL functions\n")+Errors;
    for (auto& A : Assets)
    {
        for (auto& G : A.Graphs)
        {
            if (G.Graph->GetFName()==TEXT("EventGraph"))
            {
                TArray<UEdGraphNode*> Old=G.Graph->Nodes;
                for (auto* N:Old) FBlueprintEditorUtils::RemoveNode(A.BP,N,true);
            }
            int32 Index=0;
            for (auto V : List(G.Spec,TEXT("nodes")))
            {
                J S=V->AsObject(); const FString Kind=Str(S,TEXT("kind")); UK2Node* N=nullptr;
                if (Kind==TEXT("call"))
                {
                    FString Owner=Str(S,TEXT("owner"));
                    UK2Node_CallFunction* C=Owner==TEXT("/Script/Engine.KismetArrayLibrary") ? static_cast<UK2Node_CallFunction*>(NewNode<UK2Node_CallArrayFunction>(G.Graph)) : NewNode<UK2Node_CallFunction>(G.Graph);
                    UClass* Cls=Owner.IsEmpty() ? A.BP->GeneratedClass.Get() : Class(Owner);
                    UFunction* F=Cls ? Cls->FindFunctionByName(FName(*Str(S,TEXT("function")))) : nullptr;
                    if (!F) return TEXT("FAIL missing function ")+Owner+TEXT(":")+Str(S,TEXT("function"));
                    C->SetFromFunction(F);
                    if (Flag(S,TEXT("force_impure")))
                        if (FProperty* Override=C->GetClass()->FindPropertyByName(TEXT("NodePurityOverride")))
                            Override->ImportText_Direct(TEXT("Impure"),Override->ContainerPtrToValuePtr<void>(C),C,PPF_None);
                    N=C;
                }
                else if (Kind==TEXT("get") || Kind==TEXT("set"))
                {
                    FString Owner=Str(S,TEXT("owner")); UClass* Cls=Owner.IsEmpty() ? A.BP->SkeletonGeneratedClass.Get() : Class(Owner);
                    UK2Node_Variable* Var=Kind==TEXT("get") ? static_cast<UK2Node_Variable*>(NewNode<UK2Node_VariableGet>(G.Graph)) : static_cast<UK2Node_Variable*>(NewNode<UK2Node_VariableSet>(G.Graph));
                    if (Owner.IsEmpty()) Var->VariableReference.SetSelfMember(FName(*Str(S,TEXT("variable"))));
                    else Var->VariableReference.SetExternalMember(FName(*Str(S,TEXT("variable"))),Cls);
                    N=Var;
                }
                else if (Kind==TEXT("event"))
                {
                    auto* E=NewNode<UK2Node_Event>(G.Graph);
                    UClass* Cls=Class(Str(S,TEXT("owner")));
                    E->EventReference.SetExternalMember(FName(*Str(S,TEXT("function"))),Cls);
                    E->bOverrideFunction=true; N=E;
                }
                else if (Kind==TEXT("input"))
                {
                    auto* E=NewNode<UK2Node_EnhancedInputAction>(G.Graph);
                    E->InputAction=LoadObject<UInputAction>(nullptr,*Str(S,TEXT("action"))); N=E;
                }
                else if (Kind==TEXT("branch")) N=NewNode<UK2Node_IfThenElse>(G.Graph);
                else if (Kind==TEXT("sequence")) N=NewNode<UK2Node_ExecutionSequence>(G.Graph);
                else if (Kind==TEXT("self")) N=NewNode<UK2Node_Self>(G.Graph);
                else if (Kind==TEXT("cast")) { auto* C=NewNode<UK2Node_DynamicCast>(G.Graph); C->TargetType=Class(Str(S,TEXT("class"))); C->SetPurity(Flag(S,TEXT("pure"))); N=C; }
                else if (Kind==TEXT("array")) N=NewNode<UK2Node_MakeArray>(G.Graph);
                else if (Kind==TEXT("spawn")) N=NewNode<UK2Node_SpawnActorFromClass>(G.Graph);
                else if (Kind==TEXT("macro"))
                {
                    auto* Macro=NewNode<UK2Node_MacroInstance>(G.Graph);
                    auto* Library=LoadObject<UBlueprint>(nullptr,TEXT("/Engine/EditorBlueprintResources/StandardMacros.StandardMacros"));
                    UEdGraph* Target=nullptr;
                    if (Library) for (UEdGraph* Candidate : Library->MacroGraphs) if (Candidate->GetName()==Str(S,TEXT("macro"))) Target=Candidate;
                    if (!Target) return TEXT("FAIL macro ")+Str(S,TEXT("macro"));
                    Macro->SetMacroGraph(Target); N=Macro;
                }
                else return TEXT("FAIL unknown node ")+Kind;
                N->AllocateDefaultPins();
                if (Kind==TEXT("sequence"))
                    for (int32 I=2;I<int32(S->GetNumberField(TEXT("count")));++I) CastChecked<UK2Node_ExecutionSequence>(N)->AddInputPin();
                if (Kind==TEXT("array"))
                    for (int32 I=1;I<int32(S->GetNumberField(TEXT("count")));++I) CastChecked<UK2Node_MakeArray>(N)->AddInputPin();
                if (Kind==TEXT("spawn"))
                {
                    GetDefault<UEdGraphSchema_K2>()->TrySetDefaultObject(*N->FindPinChecked(TEXT("Class")),Class(Str(S,TEXT("class"))));
                    N->ReconstructNode();
                }
                N->NodePosX=(Index%6)*340; N->NodePosY=(Index/6)*300; ++Index;
                N->NodeComment=Str(S,TEXT("comment"));
                G.Nodes.Add(Str(S,TEXT("id")),N);
                SetDefaults(N,Obj(S,TEXT("defaults")),Errors);
            }
            // Link in listed order: wildcard inputs are deliberately resolved first.
            for (auto V : List(G.Spec,TEXT("links")))
            {
                const auto& L=V->AsArray(); FString From=L[0]->AsString(),To=L[1]->AsString();
                auto* P=Pin(G,From,Errors); auto* Q=Pin(G,To,Errors);
                if (P && Q && !GetDefault<UEdGraphSchema_K2>()->TryCreateConnection(P,Q)) Errors+=TEXT("Connection failed ")+From+TEXT(" -> ")+To+TEXT("\n");
            }
            if (Flag(G.Spec,TEXT("pure")) && G.Entry && G.Result)
            {
                auto* Exec=G.Entry->FindPin(UEdGraphSchema_K2::PN_Then);
                auto* Return=G.Result->FindPin(UEdGraphSchema_K2::PN_Execute);
                if (Exec && Return) GetDefault<UEdGraphSchema_K2>()->TryCreateConnection(Exec,Return);
            }
            for (auto V:List(G.Spec,TEXT("nodes")))
            {
                J S=V->AsObject();
                if (Str(S,TEXT("kind"))==TEXT("array")) SetDefaults(G.Nodes.FindRef(Str(S,TEXT("id"))),Obj(S,TEXT("defaults")),Errors);
            }
            if (G.Entry) { G.Entry->NodePosX=-450; G.Entry->NodePosY=-250; }
            if (G.Result) { G.Result->NodePosX=2100; G.Result->NodePosY=-250; SetDefaults(G.Result,Obj(G.Spec,TEXT("defaults")),Errors); }
        }
        if (Flag(A.Spec,TEXT("animation")))
        {
            TArray<UEdGraph*> Graphs; A.BP->GetAllGraphs(Graphs);
            for (auto* Graph:Graphs) if (Graph->GetFName()==TEXT("AnimGraph"))
            {
                UAnimGraphNode_Root* R=nullptr; for (UEdGraphNode* N:Graph->Nodes) if (auto* X=Cast<UAnimGraphNode_Root>(N)) R=X;
                auto* Player=NewObject<UAnimGraphNode_BlendSpacePlayer>(Graph); Graph->AddNode(Player,false,false); Player->CreateNewGuid(); Player->SetAnimationAsset(LoadObject<UBlendSpace>(nullptr,*Str(A.Spec,TEXT("blendspace")))); Player->AllocateDefaultPins();
                auto* Slot=NewObject<UAnimGraphNode_Slot>(Graph); Graph->AddNode(Slot,false,false); Slot->CreateNewGuid(); Slot->Node.SlotName=TEXT("DefaultSlot"); Slot->AllocateDefaultPins();
                auto* Speed=NewNode<UK2Node_VariableGet>(Graph); Speed->VariableReference.SetSelfMember(TEXT("Speed")); Speed->AllocateDefaultPins();
                auto* Schema=GetDefault<UEdGraphSchema_K2>();
                Schema->TryCreateConnection(Player->FindPinChecked(TEXT("Pose")),Slot->FindPinChecked(TEXT("Source")));
                Schema->TryCreateConnection(Slot->FindPinChecked(TEXT("Pose")),R->FindPinChecked(TEXT("Result")));
                Schema->TryCreateConnection(Speed->FindPinChecked(TEXT("Speed")),Player->FindPinChecked(TEXT("X")));
                Player->NodePosX=-600; Slot->NodePosX=-250; Speed->NodePosX=-900;
            }
        }
    }
    if (!Errors.IsEmpty()) return TEXT("FAIL graph assembly\n")+Errors;
    for (auto& A:Assets)
    {
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(A.BP);
        FCompilerResultsLog Log; FKismetEditorUtilities::CompileBlueprint(A.BP,EBlueprintCompileOptions::None,&Log);
        Report+=A.BP->GetName()+FString::Printf(TEXT(" errors=%d warnings=%d\n"),Log.NumErrors,Log.NumWarnings);
        if (Log.NumErrors) return TEXT("FAIL compile\n")+Report;
        UObject* CDO=A.BP->GeneratedClass->GetDefaultObject(); Properties(CDO,Obj(A.Spec,TEXT("defaults")),Errors);
        if (auto* Character=Cast<ACharacter>(CDO))
        {
            if (!Str(A.Spec,TEXT("mesh")).IsEmpty()) Character->GetMesh()->SetSkeletalMeshAsset(LoadObject<USkeletalMesh>(nullptr,*Str(A.Spec,TEXT("mesh"))));
            if (!Str(A.Spec,TEXT("animclass")).IsEmpty()) Character->GetMesh()->SetAnimInstanceClass(Class(Str(A.Spec,TEXT("animclass"))));
            Properties(Character->GetMesh(),Obj(A.Spec,TEXT("mesh_properties")),Errors);
            Properties(Character->GetCharacterMovement(),Obj(A.Spec,TEXT("movement")),Errors);
        }
        if (!Save(A.BP)) Errors+=TEXT("Save failed ")+A.BP->GetName()+TEXT("\n");
    }
    return Errors.IsEmpty() ? TEXT("PASS\n")+Report : TEXT("FAIL defaults\n")+Errors;
#else
    return TEXT("Editor only");
#endif
}
