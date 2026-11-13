#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProgressEnemyEditorLibrary.generated.h"

UCLASS()
class PROGRESSENEMY_API UProgressEnemyEditorLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Enemy|Editor")
    static FString InspectBlueprint(const FString& Path);
    UFUNCTION(BlueprintCallable, Category="Enemy|Editor")
    static bool InstallHealthCompatibility();
    UFUNCTION(BlueprintCallable, Category="Enemy|Editor")
    static bool CreateProjectAssets();
    UFUNCTION(BlueprintCallable, Category="Enemy|Editor")
    static FString RunIntegrationChecks();
    UFUNCTION(BlueprintCallable, Category="Enemy|Editor")
    static bool CreateTestScene();
    /** Editor authoring only: emits standard Blueprint nodes, never runtime native calls. */
    UFUNCTION(BlueprintCallable, Category="Enemy|Editor")
    static FString BuildBlueprintOnly(const FString& SpecificationFile);
    UFUNCTION(BlueprintCallable, Category="Enemy|Editor")
    static FString VerifyBlueprintOnly();
    UFUNCTION(BlueprintCallable, Category="Enemy|Editor")
    static FString MatchPunchAnimationSetup();
    UFUNCTION(BlueprintCallable, Category="Enemy|Editor")
    static FString VerifyPunchAnimationPoses();
};
