#include "ProgressEnemyEditorLibrary.h"
#if WITH_EDITOR
#include "ProgressEnemyReflection.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/Blueprint.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "Animation/AnimMontage.h"
#endif

FString UProgressEnemyEditorLibrary::VerifyBlueprintOnly()
{
    FString Report;
#if WITH_EDITOR
    using namespace ProgressEnemyReflection;
    int32 Failures=0;
    auto Check=[&](bool Ok,const TCHAR* Label) { Report+=(Ok ? TEXT("PASS ") : TEXT("FAIL "))+FString(Label)+TEXT("\n"); if (!Ok) ++Failures; };
    const FString Root=TEXT("/Game/Blueprints/Agents/EnemyINProgress/BlueprintOnly/");
    UClass* EnemyClass=nullptr;
    for (const TCHAR* Name : {TEXT("AC_EnemyHealth_BP"),TEXT("AC_EnemyCombat_BP"),TEXT("ABP_Enemy"),TEXT("BP_PlayerBridge"),TEXT("BP_Enemy"),TEXT("BP_EnemySpawner")})
    {
        UBlueprint* BP=LoadObject<UBlueprint>(nullptr,*(Root+Name));
        if (!BP) return FString(TEXT("FAIL missing asset "))+Name;
        FCompilerResultsLog Log; FKismetEditorUtilities::CompileBlueprint(BP,EBlueprintCompileOptions::None,&Log);
        Check(Log.NumErrors==0,*(FString(Name)+TEXT(" compiles after reopening")));
        Check(BP->ParentClass->GetOutermost()->GetName()!=TEXT("/Script/ProgressEnemy"),*(FString(Name)+TEXT(" has an engine parent")));
        if (FString(Name)==TEXT("BP_Enemy")) EnemyClass=BP->GeneratedClass;
    }
    if (Failures || !EnemyClass) return Report+TEXT("RESULT failures=")+FString::FromInt(Failures);
    UWorld::InitializationValues Values;
    Values.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(true).ShouldSimulatePhysics(false).SetTransactional(false);
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,TEXT("BlueprintEnemyVerification"),nullptr,true,ERHIFeatureLevel::SM5,&Values);
    FWorldContext& Context=GEngine->CreateNewWorldContext(EWorldType::Game); Context.SetCurrentWorld(World);
    auto* GI=NewObject<UGameInstance>(GEngine); World->SetGameInstance(GI); Context.OwningGameInstance=GI;
    World->GetWorldSettings()->DefaultGameMode=AGameModeBase::StaticClass(); World->SetGameMode(FURL()); World->InitializeActorsForPlay(FURL()); World->BeginPlay(); World->SetBegunPlay(true);
    FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    ACharacter* Player=World->SpawnActor<ACharacter>(ACharacter::StaticClass(),FVector(120,0,100),FRotator::ZeroRotator,Params);
    Player->SetCanBeDamaged(true);
    APlayerController* PC=World->SpawnActor<APlayerController>(); PC->SetPlayerState(World->SpawnActor<APlayerState>()); PC->Possess(Player);
    ACharacter* Enemy=World->SpawnActor<ACharacter>(EnemyClass,FVector(0,0,100),FRotator::ZeroRotator,Params);
    Enemy->SetActorTickEnabled(false);
    SetNumber(Enemy,TEXT("DecisionTime"),999);
    UObject* Health=Object(Enemy,TEXT("Health"));
    Check(Health && Object(Enemy,TEXT("Combat")),TEXT("Pure Blueprint health and combat components initialize"));
    Check(FMath::IsNearlyEqual(Number(Health,TEXT("CurrentHealth")),100.),TEXT("Health starts at 100"));
    auto State=[&](int32 S) { Call(Enemy,TEXT("SetState"),[&](UFunction* F,uint8* P){ParamNumber(F,P,TEXT("NewState"),S);}); };
    auto Target=[&](AActor* A) { SetObject(Enemy,TEXT("Target"),A); };
    auto Tick=[&](double Delta)
    {
        SetNumber(Enemy,TEXT("DecisionTime"),999);
        for (FName Name : {FName(TEXT("UpdateResources")),FName(TEXT("UpdateBrain")),FName(TEXT("AdvanceFinisher"))})
            Call(Enemy,Name,[&](UFunction* F,uint8* P){ParamNumber(F,P,TEXT("Delta"),Delta);});
    };
    auto Attack=[&](int32 Index) { return Call(Enemy,TEXT("TryAttack"),[&](UFunction* F,uint8* P){ParamNumber(F,P,TEXT("Index"),Index);}) && Bool(Enemy,TEXT("AttackAccepted")); };
    auto Toggle=[&](int32 Index,bool Enabled) { Call(Enemy,TEXT("SetAttackEnabled"),[&](UFunction* F,uint8* P){ParamNumber(F,P,TEXT("Index"),Index); if(auto* B=FindFProperty<FBoolProperty>(F,TEXT("Enabled"))) B->SetPropertyValue_InContainer(P,Enabled);}); };
    auto Damage=[&](float Amount) { FDamageEvent Event; return Enemy->TakeDamage(Amount,Event,PC,Player); };
    Target(Player); State(1);
    {
        UFunction* F=Enemy->FindFunction(TEXT("AttackEnabled")); FStructOnScope P(F);
        ParamNumber(F,P.GetStructMemory(),TEXT("Index"),0); Enemy->ProcessEvent(F,P.GetStructMemory());
        bool Enabled=FindFProperty<FBoolProperty>(F,TEXT("Enabled"))->GetPropertyValue_InContainer(P.GetStructMemory());
        Call(Enemy,TEXT("ClearLineTo"),[&](UFunction* Fn,uint8* Data){FindFProperty<FObjectPropertyBase>(Fn,TEXT("Actor"))->SetObjectPropertyValue_InContainer(Data,Player);});
        Report+=FString::Printf(TEXT("DIAGNOSTIC Enabled=%d Light=%d State=%.0f Clear=%d Distance=%.1f Range=%.1f\n"),Enabled,Bool(Enemy,TEXT("EnableLight")),Number(Enemy,TEXT("State")),Bool(Enemy,TEXT("ClearSight")),FVector::Dist(Enemy->GetActorLocation(),Player->GetActorLocation()),Number(Enemy,TEXT("MeleeRange")));
    }
    Check(Attack(0),TEXT("Light melee starts")); Tick(.5);
    Check(!Bool(Enemy,TEXT("PendingImpact")),TEXT("Light hit resolves after windup"));
    auto FinishAttack=[&]() { Tick(FMath::Max(0.,Number(Enemy,TEXT("ActionTime")))+.1); };
    UObject* ActiveMontage=Object(Enemy,TEXT("ActiveMeleeMontage"));
    Check(ActiveMontage!=nullptr,TEXT("Selected punch montage is cached for playback timing"));
    auto* ActivePunch=Cast<UAnimMontage>(ActiveMontage);
    Check(ActivePunch && Number(Enemy,TEXT("ActionTime"))+.5 >= ActivePunch->GetPlayLength()/FMath::Max(.01,Number(Enemy,TEXT("AnimationRate"))),TEXT("Attack recovery covers the complete selected punch montage"));
    FinishAttack();
    Check(Attack(1),TEXT("Heavy melee starts"));
    Check(Number(Enemy,TEXT("ImpactTime"))>.6,TEXT("Heavy uses slower timing than light")); Tick(.7); FinishAttack();
    Toggle(0,false); Check(!Attack(0),TEXT("Disabled light attack cannot start"));
    Attack(1); Toggle(1,false); Toggle(2,false);
    Check(!Bool(Enemy,TEXT("PendingImpact")) && Number(Enemy,TEXT("State"))==1,TEXT("Disabling attack cancels pending damage"));
    Call(Enemy,TEXT("TryBlock")); Check(Bool(Enemy,TEXT("BlockAccepted")),TEXT("Block starts with charges"));
    const double Before=Number(Health,TEXT("CurrentHealth"));
    for (int32 I=0;I<3;++I) Damage(20);
    Check(FMath::IsNearlyEqual(Number(Health,TEXT("CurrentHealth")),Before-15),TEXT("Three block hits absorb 75 percent"));
    Check(Number(Enemy,TEXT("BlockHits"))==0 && Number(Enemy,TEXT("BlockRecharge"))>0,TEXT("Shield exhaustion starts recharge"));
    Tick(6.1); Check(Number(Enemy,TEXT("BlockHits"))==3,TEXT("Shield recharges"));
    Call(Enemy,TEXT("SetDifficultyLevels"),[&](UFunction* F,uint8* P){ParamNumber(F,P,TEXT("Damage"),2);ParamNumber(F,P,TEXT("Block"),2);ParamNumber(F,P,TEXT("HealthLevelInput"),3);ParamNumber(F,P,TEXT("MovementSpeed"),4);});
    Check(Number(Health,TEXT("MaximumHealth"))==160 && Number(Enemy,TEXT("MaximumBlockHits"))==5,TEXT("Health and block scale independently"));
    Check(FMath::IsNearlyEqual(Number(Health,TEXT("CurrentHealth"))/160.,.85),TEXT("Live scaling preserves health percentage"));
    Check(FMath::IsNearlyEqual(Number(Enemy,TEXT("IntelligenceLevel")),2.75) && FMath::IsNearlyEqual(Number(Enemy,TEXT("DecisionInterval")),.28125),TEXT("Intelligence averages all four levels and scales decision delay"));
    auto IntelligenceLevels=[&](int32 Level) { Call(Enemy,TEXT("SetDifficultyLevels"),[&](UFunction* F,uint8* P){for (FName N:{FName(TEXT("Damage")),FName(TEXT("Block")),FName(TEXT("HealthLevelInput")),FName(TEXT("MovementSpeed"))}) ParamNumber(F,P,N,Level);}); };
    IntelligenceLevels(10000);
    Check(Number(Enemy,TEXT("IntelligenceAlpha"))==1 && FMath::IsNearlyEqual(Number(Enemy,TEXT("DecisionInterval")),.1) && FMath::IsNearlyEqual(Number(Enemy,TEXT("PathUpdateInterval")),.15),TEXT("Extreme intelligence respects decision and navigation limits"));
    Player->GetCharacterMovement()->Velocity=FVector(300,0,0); Call(Enemy,TEXT("RefreshCombatChoices"));
    Check(FMath::IsNearlyZero(Number(Enemy,TEXT("HeavyDecisionChance"))),TEXT("Smart enemy avoids heavy attacks against moving targets"));
    Player->GetCharacterMovement()->Velocity=FVector::ZeroVector; Call(Enemy,TEXT("RefreshCombatChoices"));
    Check(FMath::IsNearlyEqual(Number(Enemy,TEXT("HeavyDecisionChance")),.8),TEXT("Smart enemy favors heavy attacks on close stationary targets"));
    Check(Number(Enemy,TEXT("BlockDecisionChance"))>Number(Enemy,TEXT("BlockChance")),TEXT("Smart wounded enemy increases defensive priority"));
    IntelligenceLevels(-1);
    Check(Number(Enemy,TEXT("IntelligenceLevel"))==0 && FMath::IsNearlyEqual(Number(Enemy,TEXT("DecisionInterval")),.35),TEXT("Negative levels restore base intelligence safely"));
    Call(Enemy,TEXT("SetDifficultyLevels"),[&](UFunction* F,uint8* P){ParamNumber(F,P,TEXT("Damage"),2);ParamNumber(F,P,TEXT("Block"),2);ParamNumber(F,P,TEXT("HealthLevelInput"),3);ParamNumber(F,P,TEXT("MovementSpeed"),4);});
    Player->SetActorLocation(FVector(500,0,100)); Toggle(2,true); State(1); Target(Player);
    Check(Attack(2),TEXT("First taser attack starts")); Tick(.5); Tick(1.1);
    Check(Number(Enemy,TEXT("TaserShots"))==1 && Number(Enemy,TEXT("TaserRecharge"))==0,TEXT("First taser shot does not start magazine cooldown"));
    Check(Attack(2),TEXT("Second taser attack starts")); Tick(.5); Tick(1.1);
    Check(Number(Enemy,TEXT("TaserShots"))==0 && !Attack(2),TEXT("Two shots exhaust taser magazine"));
    Tick(6.1); Check(Number(Enemy,TEXT("TaserShots"))==2,TEXT("Taser refills both shots"));
    SetBool(Enemy,TEXT("EnableBlock"),false); State(0); SetNumber(Enemy,TEXT("SightRadius"),1); SetNumber(Enemy,TEXT("ProximitySeconds"),2);
    Player->SetActorLocation(FVector(-100,0,100)); Tick(1);
    Check(Number(Enemy,TEXT("State"))==0,TEXT("Proximity does not alert immediately"));
    Player->SetActorLocation(FVector(-500,0,100)); Tick(.1); Player->SetActorLocation(FVector(-100,0,100)); Tick(1.1);
    Check(Number(Enemy,TEXT("State"))==0,TEXT("Leaving proximity resets notice timer")); Tick(1);
    Check(Number(Enemy,TEXT("State"))==1,TEXT("Continuous close presence triggers chase"));
    SetNumber(Enemy,TEXT("ProximityRadius"),1); Tick(11); Check(Number(Enemy,TEXT("State"))==2,TEXT("Losing awareness starts search")); Tick(11); Check(Number(Enemy,TEXT("State"))==0,TEXT("Search timeout returns to patrol"));
    SetNumber(Enemy,TEXT("SightRadius"),1800); Player->SetActorLocation(FVector(500,0,100)); Tick(.1);
    Check(Number(Enemy,TEXT("State"))==1,TEXT("Vision cone detects player"));
    Check(FMath::IsNearlyEqual(Enemy->GetCharacterMovement()->MaxWalkSpeed,475.2f,.01f),TEXT("Movement difficulty scales speed"));
    Player->SetActorLocation(FVector(120,0,100)); State(0); Damage(1);
    Check(Number(Enemy,TEXT("State"))==1,TEXT("Player damage alerts enemy"));
    SetNumber(Health,TEXT("CurrentHealth"),20); Call(Enemy,TEXT("UpdatePresentation"));
    Check(Bool(Health,TEXT("CanFinish")),TEXT("Low health enables finisher"));
    Call(Enemy,TEXT("BeginFinisher"),[&](UFunction* F,uint8* P){if(auto* O=FindFProperty<FObjectPropertyBase>(F,TEXT("PlayerActor"))) O->SetObjectPropertyValue_InContainer(P,Player);});
    Check(Number(Enemy,TEXT("State"))==5 && Player->GetCharacterMovement()->MovementMode==MOVE_None,TEXT("Finisher locks enemy and player movement"));
    Enemy->Destroy(); Check(Player->GetCharacterMovement()->MovementMode!=MOVE_None,TEXT("Removing enemy releases finisher movement lock"));
    for (int32 Level : {0,10})
    {
        Player->SetActorLocation(FVector(120,5000+Level*100,100));
        auto* DecisionEnemy=World->SpawnActor<ACharacter>(EnemyClass,FVector(0,5000+Level*100,100),FRotator::ZeroRotator,Params);
        DecisionEnemy->SetActorTickEnabled(false);
        SetBool(DecisionEnemy,TEXT("EnableBlock"),false); SetBool(DecisionEnemy,TEXT("EnableHeavy"),false); SetBool(DecisionEnemy,TEXT("EnableTaser"),false);
        Call(DecisionEnemy,TEXT("SetDifficultyLevels"),[&](UFunction* F,uint8* P){for (FName N:{FName(TEXT("Damage")),FName(TEXT("Block")),FName(TEXT("HealthLevelInput")),FName(TEXT("MovementSpeed"))}) ParamNumber(F,P,N,Level);});
        Call(DecisionEnemy,TEXT("NotifyPlayerDamage"),[&](UFunction* F,uint8* P){FindFProperty<FObjectPropertyBase>(F,TEXT("Source"))->SetObjectPropertyValue_InContainer(P,Player);});
        auto DecisionTick=[&](double Delta){for (FName N:{FName(TEXT("UpdateResources")),FName(TEXT("UpdateBrain"))}) Call(DecisionEnemy,N,[&](UFunction* F,uint8* P){ParamNumber(F,P,TEXT("Delta"),Delta);});};
        DecisionTick(.05);
        Check(Number(DecisionEnemy,TEXT("State"))==1,TEXT("Newly alerted enemy waits for its decision delay"));
        DecisionTick(.06);
        Check(Number(DecisionEnemy,TEXT("State"))==(Level==10 ? 3 : 1),TEXT("High intelligence attacks sooner than base intelligence"));
        if (Level==0) { DecisionTick(.25); Check(Number(DecisionEnemy,TEXT("State"))==3,TEXT("Base intelligence attacks after the longer delay")); }
        DecisionEnemy->Destroy();
    }
    World->EndPlay(EEndPlayReason::Quit); World->DestroyWorld(false); GEngine->DestroyWorldContext(World);
    Report+=FString::Printf(TEXT("RESULT failures=%d\n"),Failures);
#endif
    return Report;
}
