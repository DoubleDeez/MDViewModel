#include "Components/MDVMPawnUpdatePollingComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"


UMDVMPawnUpdatePollingComponent* UMDVMPawnUpdatePollingComponent::FindOrAddPollingComponent(APawn* Owner)
{
	if (IsValid(Owner))
	{
		UMDVMPawnUpdatePollingComponent* Result = Owner->FindComponentByClass<UMDVMPawnUpdatePollingComponent>();
		if (IsValid(Result))
		{
			return Result;
		}

		constexpr bool bManualAttachment = false;
		constexpr bool bDeferredFinish = false;
		return Cast<UMDVMPawnUpdatePollingComponent>(Owner->AddComponentByClass(StaticClass(), bManualAttachment, FTransform::Identity, bDeferredFinish));
	}

	return nullptr;
}

UMDVMPawnUpdatePollingComponent::UMDVMPawnUpdatePollingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UMDVMPawnUpdatePollingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const APawn* Pawn = CastChecked<APawn>(GetOwner());
	if (OnPlayerStateChanged.IsBound())
	{
		APlayerState* PlayerState = Pawn->GetPlayerState();
		if (CachedPlayerStateObject != PlayerState)
		{
			CachedPlayerStateObject = PlayerState;
			OnPlayerStateChanged.Broadcast();
		}
	}
}

FDelegateHandle UMDVMPawnUpdatePollingComponent::BindOnPlayerStateChanged(FSimpleDelegate&& Delegate)
{
	check(Delegate.IsBound());

	const FDelegateHandle Handle = OnPlayerStateChanged.Add(MoveTemp(Delegate));

	UpdateShouldTick();

	return Handle;
}

void UMDVMPawnUpdatePollingComponent::UnbindOnPlayerStateChanged(const FDelegateHandle& Handle)
{
	OnPlayerStateChanged.Remove(Handle);

	UpdateShouldTick();
}

void UMDVMPawnUpdatePollingComponent::UpdateShouldTick()
{
	const bool bShouldTick = OnPlayerStateChanged.IsBound();
	SetComponentTickEnabled(bShouldTick);
}


