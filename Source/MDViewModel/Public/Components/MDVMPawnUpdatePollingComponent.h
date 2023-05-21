#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MDVMPawnUpdatePollingComponent.generated.h"

class APlayerController;

/*
 * Component to poll a Pawn for changes to properties
 *
 * Not all the objects we need to bind to have delegates to notify us of them updating,
 * so we have this component poll to check for changes
 *
 * TODO - Figure out a way for games to provide events in-place of using polling (eg. UCommonLocalPlayer's OnPlayerStateSet)
 */
UCLASS()
class MDVIEWMODEL_API UMDVMPawnUpdatePollingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	static UMDVMPawnUpdatePollingComponent* FindOrAddPollingComponent(APawn* Owner);

	UMDVMPawnUpdatePollingComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	FDelegateHandle BindOnPlayerStateChanged(FSimpleDelegate&& Delegate);
	void UnbindOnPlayerStateChanged(const FDelegateHandle& Handle);

protected:
	void UpdateShouldTick();
	
	FSimpleMulticastDelegate OnPlayerStateChanged;
	TWeakObjectPtr<UObject> CachedPlayerStateObject;
};
