#include "Components/MDVMPCDynamicDelegateIntermediate.h"

#include "GameFramework/PlayerController.h"


UMDVMPCDynamicDelegateIntermediate* UMDVMPCDynamicDelegateIntermediate::FindOrAddListener(APlayerController* Owner)
{
	if (IsValid(Owner))
	{
		UMDVMPCDynamicDelegateIntermediate* Result = Owner->FindComponentByClass<UMDVMPCDynamicDelegateIntermediate>();
		if (IsValid(Result))
		{
			return Result;
		}

		constexpr bool bManualAttachment = false;
		constexpr bool bDeferredFinish = false;
		return Cast<UMDVMPCDynamicDelegateIntermediate>(Owner->AddComponentByClass(StaticClass(), bManualAttachment, FTransform::Identity, bDeferredFinish));
	}

	return nullptr;
}

void UMDVMPCDynamicDelegateIntermediate::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
	{
		PC->OnPossessedPawnChanged.AddUniqueDynamic(this, &UMDVMPCDynamicDelegateIntermediate::OnPCPawnChanged);
	}
}

void UMDVMPCDynamicDelegateIntermediate::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
	{
		PC->OnPossessedPawnChanged.RemoveAll(this);
	}

	Super::EndPlay(EndPlayReason);
}

void UMDVMPCDynamicDelegateIntermediate::OnPCPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	OnPawnChanged.Broadcast();
}
