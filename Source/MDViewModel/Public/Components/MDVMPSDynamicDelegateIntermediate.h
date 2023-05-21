#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MDVMPSDynamicDelegateIntermediate.generated.h"

class APawn;
class APlayerState;


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MDVIEWMODEL_API UMDVMPSDynamicDelegateIntermediate : public UActorComponent
{
	GENERATED_BODY()

public:
	static UMDVMPSDynamicDelegateIntermediate* FindOrAddListener(APlayerState* Owner);

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	FSimpleMulticastDelegate OnPawnChanged;
private:
	UFUNCTION()
	void OnPSPawnChanged(APlayerState* Player, APawn* NewPawn, APawn* OldPawn);
};
