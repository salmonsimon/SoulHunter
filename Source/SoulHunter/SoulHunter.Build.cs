// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class SoulHunter : ModuleRules
{
	public SoulHunter(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "HairStrandsCore", "Niagara", "GeometryCollectionEngine", "UMG", "AIModule", "LockOnTarget" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });
	}
}
