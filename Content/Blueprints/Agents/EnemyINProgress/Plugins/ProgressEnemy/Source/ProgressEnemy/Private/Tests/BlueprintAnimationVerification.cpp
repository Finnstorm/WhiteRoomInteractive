#include "ProgressEnemyEditorLibrary.h"
#if WITH_EDITOR
#include "PreviewScene.h"
#include "Engine/Blueprint.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "ProgressEnemyReflection.h"
#endif

FString UProgressEnemyEditorLibrary::VerifyPunchAnimationPoses()
{
    FString Report;
#if WITH_EDITOR
    using namespace ProgressEnemyReflection;
    int32 Failures=0;
    auto Check=[&](bool Ok,const FString& Label) { Report+=(Ok ? TEXT("PASS ") : TEXT("FAIL "))+Label+TEXT("\n"); if (!Ok) ++Failures; };
    FPreviewScene Preview(FPreviewScene::ConstructionValues().SetEditor(true).SetTransactional(false));
    UWorld* World=Preview.GetWorld();
    UClass* ReferenceClass=LoadClass<ACharacter>(nullptr,TEXT("/Game/Blueprints/Agents/Enemy-Agent/EnemyAI/Enemy_punch/BP_Enemy_punch.BP_Enemy_punch_C"));
    UClass* EnemyClass=LoadClass<ACharacter>(nullptr,TEXT("/Game/Blueprints/Agents/EnemyINProgress/BlueprintOnly/BP_Enemy.BP_Enemy_C"));
    if (!ReferenceClass || !EnemyClass) return TEXT("FAIL missing character class");
    FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Reference=World->SpawnActor<ACharacter>(ReferenceClass,FVector(0,-200,85),FRotator::ZeroRotator,Params);
    auto* Enemy=World->SpawnActor<ACharacter>(EnemyClass,FVector(0,200,85),FRotator::ZeroRotator,Params);
    if (!Reference || !Enemy) return TEXT("FAIL preview characters");
    auto* A=Reference->GetMesh(); auto* B=Enemy->GetMesh();
    Check(A->GetRelativeTransform().Equals(B->GetRelativeTransform(),.001f),TEXT("Mesh transform matches BP_Enemy_punch"));
    Check(FMath::IsNearlyEqual(Reference->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(),Enemy->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()),TEXT("Capsule height matches BP_Enemy_punch"));
    Check(A->GetSkeletalMeshAsset()==B->GetSkeletalMeshAsset(),TEXT("Same nurse skeletal mesh"));
    Check(B->GetAnimClass()->GetSuperClass()==UAnimInstance::StaticClass(),TEXT("Animation Blueprint keeps engine AnimInstance parent"));
    TArray<FTransform> Idle;
    auto Evaluate=[&](float Speed,UAnimMontage* Montage,float Rate,const FString& Label,EMovementMode MovementMode=MOVE_Walking)
    {
        for (auto* Actor:TArray<ACharacter*>{Reference,Enemy})
        {
            Actor->GetCharacterMovement()->Velocity=FVector(Speed,0,0);
            Actor->GetCharacterMovement()->SetMovementMode(MovementMode);
            auto* Mesh=Actor->GetMesh();
            Mesh->SetUpdateAnimationInEditor(true);
            Mesh->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
            Mesh->InitAnim(true);
            Mesh->TickAnimation(.01f,false); Mesh->RefreshBoneTransforms();
            if (Montage) Actor->PlayAnimMontage(Montage,Rate);
        }
        for (int32 Frame=0;Frame<8;++Frame)
            for (auto* Mesh:TArray<USkeletalMeshComponent*>{A,B}) { Mesh->TickAnimation(.05f,false); Mesh->RefreshBoneTransforms(); }
        const auto& RefPose=A->GetComponentSpaceTransforms(); const auto& Pose=B->GetComponentSpaceTransforms();
        Check(RefPose.Num()>20 && RefPose.Num()==Pose.Num(),Label+TEXT(" evaluates matching skeleton bones"));
        double MaxDistance=0,IdleDifference=0;
        for (int32 I=0;I<FMath::Min(RefPose.Num(),Pose.Num());++I)
        {
            MaxDistance=FMath::Max(MaxDistance,FVector::Dist(RefPose[I].GetLocation(),Pose[I].GetLocation()));
            if (Idle.IsValidIndex(I)) IdleDifference=FMath::Max(IdleDifference,FVector::Dist(Idle[I].GetLocation(),Pose[I].GetLocation()));
        }
        Check(MaxDistance<.1,Label+FString::Printf(TEXT(" matches reference pose (max position difference %.4f cm)"),MaxDistance));
        if (Label==TEXT("Idle")) Idle=Pose;
        else Check(IdleDifference>1,Label+TEXT(" is animated rather than a reference/idle pose"));
        Check(FMath::IsNearlyEqual(Number(B->GetAnimInstance(),TEXT("Velocity")),double(Speed),.01),Label+TEXT(" updates the locomotion velocity binding"));
    };
    Evaluate(0,nullptr,1,TEXT("Idle"));
    Evaluate(160,nullptr,1,TEXT("Walk"));
    Evaluate(360,nullptr,1,TEXT("Run"));
    Evaluate(0,nullptr,1,TEXT("Falling"),MOVE_Falling);
    auto* Punch=LoadObject<UAnimMontage>(nullptr,TEXT("/Game/Blueprints/Agents/Enemies/Animations/AI/Cross_Punch_Montage.Cross_Punch_Montage"));
    Evaluate(0,Punch,1.35f,TEXT("Light punch"));
    Evaluate(0,Punch,.75f,TEXT("Heavy punch"));
    Reference->Destroy(); Enemy->Destroy();
    Report+=FString::Printf(TEXT("RESULT failures=%d\n"),Failures);
#endif
    return Report;
}
