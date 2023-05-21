#include "Components/MDVMPSDynamicDelegateIntermediate.h"

#include "GameFramework/PlayerState.h"

UMDVMPSDynamicDelegateIntermediate* UMDVMPSDynamicDelegateIntermediate::FindOrAddListener(APlayerState* Owner)
{
	if (IsValid(Owner))
	{
		UMDVMPSDynamicDelegateIntermediate* Result = Owner->FindComponentByClass<UMDVMPSDynamicDelegateIntermediate>();
		if (IsValid(Result))
		{
			return Result;
		}

		constexpr bool bManualAttachment = false;
		constexpr bool bDeferredFinish = false;
		return Cast<UMDVMPSDynamicDelegateIntermediate>(Owner->AddComponentByClass(StaticClass(), bManualAttachment, FTransform::Identity, bDeferredFinish));
	}

	return nullptr;
}

void UMDVMPSDynamicDelegateIntermediate::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerState* PlayerState = Cast<APlayerState>(GetOwner()))
	{
		PlayerState->OnPawnSet.AddDynamic(this, &UMDVMPSDynamicDelegateIntermediate::OnPSPawnChanged);
	}
}

void UMDVMPSDynamicDelegateIntermediate::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APlayerState* PlayerState = Cast<APlayerState>(GetOwner()))
	{
		PlayerState->OnPawnSet.RemoveAll(this);
	}

	Super::EndPlay(EndPlayReason);
}

void UMDVMPSDynamicDelegateIntermediate::OnPSPawnChanged(APlayerState* Player, APawn* NewPawn, APawn* OldPawn)
{
	OnPawnChanged.Broadcast();
}


