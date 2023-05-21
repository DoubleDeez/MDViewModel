#include "Components/MDVMPCUpdatePollingComponent.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"


UMDVMPCUpdatePollingComponent* UMDVMPCUpdatePollingComponent::FindOrAddPollingComponent(APlayerController* Owner)
{
	if (IsValid(Owner))
	{
		UMDVMPCUpdatePollingComponent* Result = Owner->FindComponentByClass<UMDVMPCUpdatePollingComponent>();
		if (IsValid(Result))
		{
			return Result;
		}

		constexpr bool bManualAttachment = false;
		constexpr bool bDeferredFinish = false;
		return Cast<UMDVMPCUpdatePollingComponent>(Owner->AddComponentByClass(StaticClass(), bManualAttachment, FTransform::Identity, bDeferredFinish));
	}

	return nullptr;
}

UMDVMPCUpdatePollingComponent::UMDVMPCUpdatePollingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UMDVMPCUpdatePollingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const APlayerController* PC = CastChecked<APlayerController>(GetOwner());
	if (OnHUDChanged.IsBound())
	{
		AHUD* HUD = PC->GetHUD();
		if (CachedHUDObject != HUD)
		{
			CachedHUDObject = HUD;
			OnHUDChanged.Broadcast();
		}
	}

	if (OnPlayerStateChanged.IsBound())
	{
		APlayerState* PlayerState = PC->GetPlayerState<APlayerState>();
		if (CachedPlayerStateObject != PlayerState)
		{
			CachedPlayerStateObject = PlayerState;
			OnPlayerStateChanged.Broadcast();
		}
	}
}

FDelegateHandle UMDVMPCUpdatePollingComponent::BindOnHUDChanged(FSimpleDelegate&& Delegate)
{
	check(Delegate.IsBound());

	const FDelegateHandle Handle = OnHUDChanged.Add(MoveTemp(Delegate));

	UpdateShouldTick();

	return Handle;
}

void UMDVMPCUpdatePollingComponent::UnbindOnHUDChanged(const FDelegateHandle& Handle)
{
	OnHUDChanged.Remove(Handle);

	UpdateShouldTick();
}

FDelegateHandle UMDVMPCUpdatePollingComponent::BindOnPlayerStateChanged(FSimpleDelegate&& Delegate)
{
	check(Delegate.IsBound());

	const FDelegateHandle Handle = OnPlayerStateChanged.Add(MoveTemp(Delegate));

	UpdateShouldTick();

	return Handle;
}

void UMDVMPCUpdatePollingComponent::UnbindOnPlayerStateChanged(const FDelegateHandle& Handle)
{
	OnPlayerStateChanged.Remove(Handle);

	UpdateShouldTick();
}

void UMDVMPCUpdatePollingComponent::UpdateShouldTick()
{
	const bool bShouldTick = OnHUDChanged.IsBound() || OnPlayerStateChanged.IsBound();
	SetComponentTickEnabled(bShouldTick);
}

