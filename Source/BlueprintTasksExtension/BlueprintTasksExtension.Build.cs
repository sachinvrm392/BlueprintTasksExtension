// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;       // Required for Directory and Path

public class BlueprintTasksExtension : ModuleRules
{
	public BlueprintTasksExtension(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		//Path to the current plugin directory
		string PluginPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../"));
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core", 
				"BlueprintTaskForge",
				"GameplayTags", 
				"GameFeatures", 
				"OmniToolbox",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("BlueprintTaskForgeEditor");
		}
		
		//Check if the Cog plugin exists
		if (Directory.Exists(Path.Combine(PluginPath, "Cog")))
		{
			PublicDefinitions.Add("COG_INSTALLED=1");
			PublicDependencyModuleNames.Add("CogCommon");
			// Other Cog plugins can be added only for specific target configuration
			if (Target.Configuration != UnrealTargetConfiguration.Shipping)
			{
				PublicDependencyModuleNames.AddRange(new string[]
				{
					"Cog",
					"CogDebug",
					"CogEngine",
					"CogImgui",
				});
			}
		}
		else
		{
			PublicDefinitions.Add("COG_INSTALLED=0");
		}
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
