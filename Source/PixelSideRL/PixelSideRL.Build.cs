using UnrealBuildTool;

public class PixelSideRL : ModuleRules
{
	public PixelSideRL(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"Paper2D"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"ImageWrapper"
		});
	}
}
