// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "RakNetPrivatePCH.h"

IMPLEMENT_MODULE(FRakNet, RakNet)

DEFINE_LOG_CATEGORY(LogRakNet);

void FRakNet::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FRakNet::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}
