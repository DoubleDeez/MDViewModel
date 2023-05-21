#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MDVMPCDynamicDelegateIntermediate.generated.h"

class APawn;
class APlayerController;

/*
 * Specific component used as an intermediate for dynamic delegates on APlayerController
 */
UCLASS()
class MDVIEWMODEL_API UMDVMPCDynamicDelegateIntermediate : public UActorComponent
{
	GENERATED_BODY()

public:
	static UMDVMPCDynamicDelegateIntermediate* FindOrAddListener(APlayerController* Owner);

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	FSimpleMulticastDelegate OnPawnChanged;
private:
	UFUNCTION()
	void OnPCPawnChanged(APawn* OldPawn, APawn* NewPawn);
};
