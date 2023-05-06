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

UMDViewModelBase* UMDLocalPlayerViewModelCache::GetOrCreateViewModel(const FName& ViewModelName, TSubclassOf<UMDViewModelBase> ViewModelClass)
{
	const FMDViewModelInstanceKey Key = { MDViewModelUtils::ResolveViewModelName(ViewModelClass, ViewModelName), ViewModelClass };
	check(Key.IsValid());

	TObjectPtr<UMDViewModelBase>& ViewModel = CachedViewModels.FindOrAdd(Key);
	if (ViewModel == nullptr)
	{
		ViewModel = NewObject<UMDViewModelBase>(GetLocalPlayer(), Key.ViewModelClass);
		ViewModel->InitializeViewModel();
	}

	return ViewModel;
}
