#include "Components/MDVMPlayerControllerUpdatePollingComponent.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"


UMDVMPlayerControllerUpdatePollingComponent* UMDVMPlayerControllerUpdatePollingComponent::FindOrAddPollingComponent(APlayerController* Owner)
{
	if (IsValid(Owner))
	{
		UMDVMPlayerControllerUpdatePollingComponent* Result = Owner->FindComponentByClass<UMDVMPlayerControllerUpdatePollingComponent>();
		if (IsValid(Result))
		{
			return Result;
		}

		constexpr bool bManualAttachment = false;
		constexpr bool bDeferredFinish = false;
		return Cast<UMDVMPlayerControllerUpdatePollingComponent>(Owner->AddComponentByClass(UMDVMPlayerControllerUpdatePollingComponent::StaticClass(), bManualAttachment, FTransform::Identity, bDeferredFinish));
	}

	return nullptr;
}

UMDVMPlayerControllerUpdatePollingComponent::UMDVMPlayerControllerUpdatePollingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UMDVMPlayerControllerUpdatePollingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// TODO - Notify the cached provider
}

void UMDVMPlayerControllerUpdatePollingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
	{
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
				OnHUDChanged.Broadcast();
			}
		}
	}
}

FDelegateHandle UMDVMPlayerControllerUpdatePollingComponent::BindOnHUDChanged(FSimpleDelegate&& Delegate)
{
	check(Delegate.IsBound());

	const FDelegateHandle Handle = OnHUDChanged.Add(MoveTemp(Delegate));

	UpdateShouldTick();

	return Handle;
}

void UMDVMPlayerControllerUpdatePollingComponent::UnbindOnHUDChanged(const FDelegateHandle& Handle)
{
	OnHUDChanged.Remove(Handle);

	UpdateShouldTick();
}

FDelegateHandle UMDVMPlayerControllerUpdatePollingComponent::BindOnPlayerStateChanged(FSimpleDelegate&& Delegate)
{
	check(Delegate.IsBound());

	const FDelegateHandle Handle = OnPlayerStateChanged.Add(MoveTemp(Delegate));

	UpdateShouldTick();

	return Handle;
}

void UMDVMPlayerControllerUpdatePollingComponent::UnbindOnPlayerStateChanged(const FDelegateHandle& Handle)
{
	OnPlayerStateChanged.Remove(Handle);

	UpdateShouldTick();
}

void UMDVMPlayerControllerUpdatePollingComponent::UpdateShouldTick()
{
	const bool bShouldTick = OnHUDChanged.IsBound() || OnPlayerStateChanged.IsBound();
	SetComponentTickEnabled(bShouldTick);
}

