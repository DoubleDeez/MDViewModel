#include "Bindings/MDViewModelDelegateBinding.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "ViewModel/MDViewModelBase.h"
#include "WidgetExtensions/MDViewModelWidgetClassExtension.h"
#include "WidgetExtensions/MDViewModelWidgetExtension.h"

void UMDViewModelDelegateBinding::BindDynamicDelegates(UObject* InInstance) const
{
	if (UUserWidget* Widget = Cast<UUserWidget>(InInstance))
	{
		if (UWidgetBlueprintGeneratedClass* WBGC = Cast<UWidgetBlueprintGeneratedClass>(Widget->GetClass()))
		{
			if (UMDViewModelWidgetClassExtension* Extension = WBGC->GetExtension<UMDViewModelWidgetClassExtension>())
			{
				TWeakObjectPtr<UObject> WeakObject = Widget;
				for (int32 i = 0; i < ViewModelDelegateBindings.Num(); ++i)
				{
					const FMDViewModelDelegateBindingEntry& Entry = ViewModelDelegateBindings[i];
					auto Delegate = FMDVMOnViewModelSet::FDelegate::CreateUObject(this, &UMDViewModelDelegateBinding::OnViewModelChanged, i, WeakObject);
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

void UMDViewModelDelegateBinding::UnbindDynamicDelegates(UObject* InInstance) const
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
