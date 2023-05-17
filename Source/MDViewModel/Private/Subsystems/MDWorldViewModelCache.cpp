#include "Subsystems/MDWorldViewModelCache.h"
#include "Engine/World.h"
#include "Util/MDViewModelUtils.h"
#include "ViewModel/MDViewModelBase.h"

bool UMDWorldViewModelCache::ShouldCreateSubsystem(UObject* Outer) const
{
	TArray<UClass*> ChildClasses;
	GetDerivedClasses(GetClass(), ChildClasses, false);

	// Only create an instance if there is no override implementation defined elsewhere
	return ChildClasses.Num() == 0;
}

void UMDWorldViewModelCache::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// TODO - Notify the provider that we're available
}

void UMDWorldViewModelCache::Deinitialize()
{
	BroadcastShutdown();
	
	Super::Deinitialize();
}

UObject* UMDWorldViewModelCache::GetViewModelOwner() const
{
	return GetWorld();
}
