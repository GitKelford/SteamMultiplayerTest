#include "Core/MTMainMenuGameMode.h"

#include "Player/MTPlayerController.h"

AMTMainMenuGameMode::AMTMainMenuGameMode()
{
	DefaultPawnClass = nullptr;
	SpectatorClass = nullptr;
	PlayerControllerClass = AMTPlayerController::StaticClass();
}

UClass* AMTMainMenuGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	return nullptr;
}
