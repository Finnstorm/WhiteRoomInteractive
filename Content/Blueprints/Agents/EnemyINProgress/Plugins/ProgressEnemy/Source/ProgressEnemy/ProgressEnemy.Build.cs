using UnrealBuildTool;
public class ProgressEnemy : ModuleRules
{
    public ProgressEnemy(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new [] { "Core", "CoreUObject", "Engine", "AIModule", "NavigationSystem", "GameplayTasks", "UMG", "EnhancedInput", "AnimGraphRuntime", "DeveloperSettings" });
        if (Target.bBuildEditor)
            PrivateDependencyModuleNames.AddRange(new [] { "UnrealEd", "BlueprintGraph", "Kismet", "KismetCompiler", "AnimGraph", "Json", "InputBlueprintNodes" });
    }
}
