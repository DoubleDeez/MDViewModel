#include "Subsystems/MDLocalPlayerViewModelCache.h"
#include "Engine/LocalPlayer.h"
#include "Util/MDViewModelUtils.h"
#include "ViewModel/MDViewModelBase.h"

bool UMDLocalPlayerViewModelCache::ShouldCreateSubsystem(UObject* Outer) const
{
	TArray<UClass*> ChildClasses;
	GetDerivedClasses(GetClass(), ChildClasses, false);

	// Only create an instance if there is no override implementation defined elsewhere
	return ChildClasses.Num() == 0;
}

void UMDLocalPlayerViewModelCache::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// TODO - Notify the provider that we're available
}

void UMDLocalPlayerViewModelCache::Deinitialize()
{
	BroadcastShutdown();
	
	Super::Deinitialize();
}

UObject* UMDLocalPlayerViewModelCache::GetViewModelOwner() const
{
	return GetLocalPlayer();
}
