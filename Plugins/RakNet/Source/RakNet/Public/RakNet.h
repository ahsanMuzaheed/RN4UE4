// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IRakNet.h"

class FRakNet : public IRakNet
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};