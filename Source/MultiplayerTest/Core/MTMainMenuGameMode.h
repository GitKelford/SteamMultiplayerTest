#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MTMainMenuGameMode.generated.h"

UCLASS()
class MULTIPLAYERTEST_API AMTMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	AMTMainMenuGameMode();
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;
};
