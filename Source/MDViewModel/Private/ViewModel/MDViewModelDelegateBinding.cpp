#include "ViewModel/MDViewModelDelegateBinding.h"

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
				TWeakObjectPtr<UUserWidget> WeakWidget = Widget;
				for (int32 i = 0; i < ViewModelDelegateBindings.Num(); ++i)
				{
					const FMDViewModelDelegateBindingEntry& Entry = ViewModelDelegateBindings[i];
					auto Delegate = FMDVMOnViewModelAssigned::FDelegate::CreateUObject(this, &UMDViewModelDelegateBinding::OnViewModelChanged, i, WeakWidget);
					Extension->QueueListenForChanges(Widget, MoveTemp(Delegate), Entry.ViewModelClass, Entry.ViewModelName);
				}
			}
		}
	}
}

void UMDViewModelDelegateBinding::UnbindDynamicDelegates(UObject* InInstance) const
{
	if (UUserWidget* Widget = Cast<UUserWidget>(InInstance))
	{
		UMDViewModelWidgetExtension* Extension = Widget->GetExtension<UMDViewModelWidgetExtension>();
		if (IsValid(Extension))
		{
			Extension->StopListeningForAllViewModelsChanges(this);
		}
	}
}

void UMDViewModelDelegateBinding::OnViewModelChanged(UMDViewModelBase* OldViewModel, UMDViewModelBase* NewViewModel, int32 EntryIndex, TWeakObjectPtr<UUserWidget> BoundWidget) const
{
	if (!ensure(ViewModelDelegateBindings.IsValidIndex(EntryIndex)) || !BoundWidget.IsValid())
	{
		return;
	}

	const FMDViewModelDelegateBindingEntry& Entry = ViewModelDelegateBindings[EntryIndex];

	if (const FMulticastDelegateProperty* MulticastDelegateProp = FindFProperty<FMulticastDelegateProperty>(Entry.ViewModelClass, Entry.DelegatePropertyName))
	{
		FScriptDelegate Delegate;
		Delegate.BindUFunction(BoundWidget.Get(), Entry.FunctionNameToBind);

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
