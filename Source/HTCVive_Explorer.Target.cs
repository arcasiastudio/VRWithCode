// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class HTCVive_ExplorerTarget : TargetRules
{
	public HTCVive_ExplorerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;

		ExtraModuleNames.AddRange( new string[] { "HTCVive_Explorer" } );
	}
}
