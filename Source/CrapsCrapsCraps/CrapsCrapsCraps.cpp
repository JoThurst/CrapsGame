// Copyright Epic Games, Inc. All Rights Reserved.

#include "CrapsCrapsCraps.h"
#include "Modules/ModuleManager.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealType.h"

class FCrapsCrapsCrapsModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		FDefaultGameModuleImpl::StartupModule();

		// Automatically enable Remote Python Execution on URemoteControlSettings CDO for Antigravity MCP
		if (UClass* RCSettingsClass = FindObject<UClass>(nullptr, TEXT("/Script/RemoteControlCommon.RemoteControlSettings")))
		{
			if (UObject* CDO = RCSettingsClass->GetDefaultObject())
			{
				if (FBoolProperty* RestrictProp = FindFProperty<FBoolProperty>(RCSettingsClass, TEXT("bRestrictServerAccess")))
				{
					RestrictProp->SetPropertyValue_InContainer(CDO, true);
				}
				if (FBoolProperty* PyProp = FindFProperty<FBoolProperty>(RCSettingsClass, TEXT("bEnableRemotePythonExecution")))
				{
					PyProp->SetPropertyValue_InContainer(CDO, true);
				}
			}
		}
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE( FCrapsCrapsCrapsModule, CrapsCrapsCraps, "CrapsCrapsCraps" );

