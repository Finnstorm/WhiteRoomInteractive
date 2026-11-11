using UnrealBuildTool;
public class DungeonGuidance : ModuleRules {
 public DungeonGuidance(ReadOnlyTargetRules Target) : base(Target) {
  PCHUsage=PCHUsageMode.UseExplicitOrSharedPCHs;
  PublicDependencyModuleNames.AddRange(new[]{"Core","CoreUObject","Engine","NavigationSystem","ProceduralMeshComponent","GeometryCore","GeometryAlgorithms","UMG","Slate","SlateCore","ApplicationCore","EnhancedInput","InputCore"});
 }
}


