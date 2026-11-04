using UnrealBuildTool;
using System.Collections.Generic;

public class PixelSideRLEditorTarget : TargetRules
{
	public PixelSideRLEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("PixelSideRL");
	}
}
