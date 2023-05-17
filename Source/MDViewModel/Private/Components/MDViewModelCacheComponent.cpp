#include "Components/MDViewModelCacheComponent.h"

#include "GameFramework/Actor.h"
#include "Util/MDViewModelInstanceKey.h"
#include "Util/MDViewModelUtils.h"
#include "ViewModel/MDViewModelBase.h"


UMDViewModelCacheComponent* UMDViewModelCacheComponent::FindOrAddCache(AActor* Owner)
{
	if (IsValid(Owner))
	{
		UMDViewModelCacheComponent* Result = Owner->FindComponentByClass<UMDViewModelCacheComponent>();
		if (IsValid(Result))
		{
			return Result;
		}

		constexpr bool bManualAttachment = false;
		constexpr bool bDeferredFinish = false;
		return Cast<UMDViewModelCacheComponent>(Owner->AddComponentByClass(UMDViewModelCacheComponent::StaticClass(), bManualAttachment, FTransform::Identity, bDeferredFinish));
	}

	return nullptr;
}

void UMDViewModelCacheComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	BroadcastShutdown();
	
	Super::EndPlay(EndPlayReason);
}

UObject* UMDViewModelCacheComponent::GetViewModelOwner() const
{
	return GetOwner();
}
