#include "Subsystems/MDGlobalViewModelCache.h"
#include "Engine/GameInstance.h"
#include "Util/MDViewModelUtils.h"
#include "ViewModel/MDViewModelBase.h"

bool UMDGlobalViewModelCache::ShouldCreateSubsystem(UObject* Outer) const
{
	TArray<UClass*> ChildClasses;
	GetDerivedClasses(GetClass(), ChildClasses, false);

	// Only create an instance if there is no override implementation defined elsewhere
	return ChildClasses.Num() == 0;
}

void UMDGlobalViewModelCache::Deinitialize()
{
	BroadcastShutdown();

	Super::Deinitialize();
}

UObject* UMDGlobalViewModelCache::GetViewModelOwner() const
{
	return GetGameInstance();
}
