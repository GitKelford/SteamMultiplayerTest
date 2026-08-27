#include "Core/MTGameMode.h"

#include "Player/MTCharacter.h"
#include "Player/MTPlayerController.h"

AMTGameMode::AMTGameMode()
{
	DefaultPawnClass = AMTCharacter::StaticClass();
	PlayerControllerClass = AMTPlayerController::StaticClass();
}
