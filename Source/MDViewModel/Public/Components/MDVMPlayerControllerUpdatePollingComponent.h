#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MDVMPlayerControllerUpdatePollingComponent.generated.h"

/*
 * Not all the objects we need to bind to have delegates to notify us of them updating,
 * so we have this component poll to check for changes
 */
UCLASS()
class MDVIEWMODEL_API UMDVMPlayerControllerUpdatePollingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	static UMDVMPlayerControllerUpdatePollingComponent* FindOrAddPollingComponent(APlayerController* Owner);

	UMDVMPlayerControllerUpdatePollingComponent();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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
