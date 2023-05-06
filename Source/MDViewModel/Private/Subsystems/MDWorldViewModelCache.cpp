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

UMDViewModelBase* UMDWorldViewModelCache::GetOrCreateViewModel(const FName& ViewModelName, TSubclassOf<UMDViewModelBase> ViewModelClass)
{
	const FMDViewModelInstanceKey Key = { MDViewModelUtils::ResolveViewModelName(ViewModelClass, ViewModelName), ViewModelClass };
	check(Key.IsValid());

	TObjectPtr<UMDViewModelBase>& ViewModel = CachedViewModels.FindOrAdd(Key);
	if (ViewModel == nullptr)
	{
		ViewModel = NewObject<UMDViewModelBase>(GetWorld(), Key.ViewModelClass);
		ViewModel->InitializeViewModel();
	}

	return ViewModel;
}
