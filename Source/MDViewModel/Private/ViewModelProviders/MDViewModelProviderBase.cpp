#include "ViewModelProviders/MDViewModelProviderBase.h"

#include "ViewModel/MDViewModelBase.h"


bool UMDViewModelProviderBase::ShouldCreateSubsystem(UObject* Outer) const
{
	// by default, all providers can be overidden
	
	TArray<UClass*> ChildClasses;
	GetDerivedClasses(GetClass(), ChildClasses, false);

	// Only create an instance if there is no override implementation defined elsewhere
	return ChildClasses.Num() == 0;
}

#if WITH_EDITOR
void UMDViewModelProviderBase::GetSupportedViewModelClasses(TArray<FMDViewModelSupportedClass>& OutViewModelClasses)
{
	// By default, providers support all view model classes
	OutViewModelClasses.Add({ UMDViewModelBase::StaticClass() });
}
#endif

void UMDViewModelProviderBase::BroadcastViewModelUpdated(TSubclassOf<UMDViewModelBase> ViewModelClass) const
{
	OnViewModelUpdated.Broadcast(ViewModelClass);
}
