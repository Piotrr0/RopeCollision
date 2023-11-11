// Copyright Epic Games, Inc. All Rights Reserved.

#include "RopeGameMode.h"
#include "RopeCharacter.h"
#include "UObject/ConstructorHelpers.h"

ARopeGameMode::ARopeGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
