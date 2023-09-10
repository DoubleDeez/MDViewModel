#include "Bindings/MDViewModelDelegateBinding.h"

#include "ViewModel/MDViewModelBase.h"

void UMDViewModelDelegateBinding::OnViewModelChanged(UMDViewModelBase* OldViewModel, UMDViewModelBase* NewViewModel, int32 EntryIndex, TWeakObjectPtr<UObject> BoundObject) const
{
	if (!ensure(ViewModelDelegateBindings.IsValidIndex(EntryIndex)) || !BoundObject.IsValid())
	{
		return;
	}

	const FMDViewModelDelegateBindingEntry& Entry = ViewModelDelegateBindings[EntryIndex];

	if (const FMulticastDelegateProperty* MulticastDelegateProp = FindFProperty<FMulticastDelegateProperty>(Entry.ViewModelClass, Entry.DelegatePropertyName))
	{
		FScriptDelegate Delegate;
		Delegate.BindUFunction(BoundObject.Get(), Entry.FunctionNameToBind);

		if (IsValid(OldViewModel))
		{
			MulticastDelegateProp->RemoveDelegate(Delegate, OldViewModel);
		}

		if (IsValid(NewViewModel))
		{
			MulticastDelegateProp->AddDelegate(MoveTemp(Delegate), NewViewModel);
		}
	}
}
