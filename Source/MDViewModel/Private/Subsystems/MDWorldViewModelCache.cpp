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

void UMDWorldViewModelCache::Deinitialize()
{
	BroadcastShutdown();

	Super::Deinitialize();
}

UObject* UMDWorldViewModelCache::GetViewModelOwner() const
{
	return GetWorld();
}

FString UMDWorldViewModelCache::GetCacheDebugName() const
{
	return FString::Printf(TEXT("World Cache (%s)"), *GetWorld()->GetMapName());
}
