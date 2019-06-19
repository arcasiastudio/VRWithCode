// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class HTCVive_ExplorerEditorTarget : TargetRules
{
	public HTCVive_ExplorerEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;

		ExtraModuleNames.AddRange( new string[] { "HTCVive_Explorer" } );
	}
}
