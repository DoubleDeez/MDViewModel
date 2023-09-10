#include "Bindings/MDViewModelChangedBinding.h"

#include "ViewModel/MDViewModelBase.h"

void UMDViewModelChangedBinding::OnViewModelChanged(UMDViewModelBase* OldViewModel, UMDViewModelBase* NewViewModel, int32 EntryIndex, TWeakObjectPtr<UObject> BoundObject) const
{
	if (!ensure(ViewModelChangedBindings.IsValidIndex(EntryIndex)) || !BoundObject.IsValid())
	{
		return;
	}

	const FMDViewModelChangedBindingEntry& Entry = ViewModelChangedBindings[EntryIndex];
	if (UFunction* Func = BoundObject->FindFunction(Entry.FunctionNameToBind))
	{
		struct
		{
			UMDViewModelBase* OldViewModel = nullptr;
			UMDViewModelBase* NewViewModel = nullptr;
		} Params = { OldViewModel, NewViewModel };

		BoundObject->ProcessEvent(Func, &Params);
	}
}
