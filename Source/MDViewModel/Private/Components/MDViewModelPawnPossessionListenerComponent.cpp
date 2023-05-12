#include "Components/MDViewModelPawnPossessionListenerComponent.h"


UMDViewModelPawnPossessionListenerComponent* UMDViewModelPawnPossessionListenerComponent::FindOrAddListener(APlayerController* Owner)
{
	if (IsValid(Owner))
	{
		UMDViewModelPawnPossessionListenerComponent* Result = Owner->FindComponentByClass<UMDViewModelPawnPossessionListenerComponent>();
		if (IsValid(Result))
		{
			return Result;
		}

		constexpr bool bManualAttachment = false;
		constexpr bool bDeferredFinish = false;
		return Cast<UMDViewModelPawnPossessionListenerComponent>(Owner->AddComponentByClass(UMDViewModelPawnPossessionListenerComponent::StaticClass(), bManualAttachment, FTransform::Identity, bDeferredFinish));
	}

	return nullptr;
}

void UMDViewModelPawnPossessionListenerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
	{
		PC->OnPossessedPawnChanged.AddUniqueDynamic(this, &UMDViewModelPawnPossessionListenerComponent::OnPCPawnChanged);
	}
}

void UMDViewModelPawnPossessionListenerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
	{
		PC->OnPossessedPawnChanged.RemoveAll(this);
	}
	
	Super::EndPlay(EndPlayReason);
}

void UMDViewModelPawnPossessionListenerComponent::OnPCPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	OnPawnChanged.Broadcast();
}
