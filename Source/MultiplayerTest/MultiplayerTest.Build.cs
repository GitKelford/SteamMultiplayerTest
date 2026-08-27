using UnrealBuildTool;

public class MultiplayerTest : ModuleRules
{
    public MultiplayerTest(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicIncludePaths.Add(ModuleDirectory);

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "EnhancedInput",
            "InputCore",
            "UMG",
            "OnlineSubsystem"
        });

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "Slate", "SlateCore",
            "OnlineSubsystemUtils",
            "OnlineSubsystemSteam", "Niagara"
        });
    }
}
