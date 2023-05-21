#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MDVMPCUpdatePollingComponent.generated.h"

class APlayerController;

/*
 * Component to poll a Player Controller for changes to properties
 *
 * Not all the objects we need to bind to have delegates to notify us of them updating,
 * so we have this component poll to check for changes
 *
 * TODO - Figure out a way for games to provide events in-place of using polling (eg. UCommonLocalPlayer's OnPlayerStateSet)
 */
UCLASS()
class MDVIEWMODEL_API UMDVMPCUpdatePollingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	static UMDVMPCUpdatePollingComponent* FindOrAddPollingComponent(APlayerController* Owner);

	UMDVMPCUpdatePollingComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	FDelegateHandle BindOnHUDChanged(FSimpleDelegate&& Delegate);
	void UnbindOnHUDChanged(const FDelegateHandle& Handle);

	FDelegateHandle BindOnPlayerStateChanged(FSimpleDelegate&& Delegate);
	void UnbindOnPlayerStateChanged(const FDelegateHandle& Handle);

protected:
	void UpdateShouldTick();

	FSimpleMulticastDelegate OnHUDChanged;
	TWeakObjectPtr<UObject> CachedHUDObject;

	FSimpleMulticastDelegate OnPlayerStateChanged;
	TWeakObjectPtr<UObject> CachedPlayerStateObject;
};
