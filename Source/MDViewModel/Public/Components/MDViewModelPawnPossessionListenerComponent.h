#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MDViewModelPawnPossessionListenerComponent.generated.h"

class APawn;

/*
 * Specific component used to be notified when a player's pawn has changed
 */
UCLASS()
class MDVIEWMODEL_API UMDViewModelPawnPossessionListenerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	static UMDViewModelPawnPossessionListenerComponent* FindOrAddListener(APlayerController* Owner);

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	FSimpleMulticastDelegate OnPawnChanged;
private:
	UFUNCTION()
	void OnPCPawnChanged(APawn* OldPawn, APawn* NewPawn);
};
