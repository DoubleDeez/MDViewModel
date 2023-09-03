#include "Bindings/MDViewModelChangedBinding.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "WidgetExtensions/MDViewModelWidgetClassExtension.h"

void UMDViewModelChangedBinding::BindDynamicDelegates(UObject* InInstance) const
{
	if (UUserWidget* Widget = Cast<UUserWidget>(InInstance))
	{
		if (UWidgetBlueprintGeneratedClass* WBGC = Cast<UWidgetBlueprintGeneratedClass>(Widget->GetClass()))
		{
			if (UMDViewModelWidgetClassExtension* Extension = WBGC->GetExtension<UMDViewModelWidgetClassExtension>())
			{
				TWeakObjectPtr<UObject> WeakObject = Widget;
				for (int32 i = 0; i < ViewModelChangedBindings.Num(); ++i)
				{
					const FMDViewModelChangedBindingEntry& Entry = ViewModelChangedBindings[i];
					auto Delegate = FMDVMOnViewModelSet::FDelegate::CreateUObject(this, &UMDViewModelChangedBinding::OnViewModelChanged, i, WeakObject);
					Extension->QueueListenForChanges(Widget, MoveTemp(Delegate), Entry.ViewModelClass, Entry.ViewModelName);
				}
			}
		}
	}
	else
	{
		// TODO - Actor View Models
	}
}

void UMDViewModelChangedBinding::UnbindDynamicDelegates(UObject* InInstance) const
{
	if (const UUserWidget* Widget = Cast<UUserWidget>(InInstance))
	{
		UMDViewModelWidgetExtension* Extension = Widget->GetExtension<UMDViewModelWidgetExtension>();
		if (IsValid(Extension))
		{
			Extension->StopListeningForAllNativeViewModelsChanged(this);
		}
	}
	else
	{
		// TODO - Actor View Models
	}
}

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
		} Params = {OldViewModel, NewViewModel};

		BoundObject->ProcessEvent(Func, &Params);
	}
}
