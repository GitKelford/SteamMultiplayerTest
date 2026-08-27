#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "MTPlayerCameraManager.generated.h"

UCLASS()
class MULTIPLAYERTEST_API AMTPlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()
public:
	AMTPlayerCameraManager();
};
