#include "ViewModel/MDViewModelFieldNotifyBinding.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "ViewModel/MDViewModelBase.h"
#include "WidgetExtensions/MDViewModelWidgetClassExtension.h"

void UMDViewModelFieldNotifyBinding::BindDynamicDelegates(UObject* InInstance) const
{
	if (UUserWidget* Widget = Cast<UUserWidget>(InInstance))
	{
		if (UWidgetBlueprintGeneratedClass* WBGC = Cast<UWidgetBlueprintGeneratedClass>(Widget->GetClass()))
		{
			if (UMDViewModelWidgetClassExtension* Extension = WBGC->GetExtension<UMDViewModelWidgetClassExtension>())
			{
				TWeakObjectPtr<UUserWidget> WeakWidget = Widget;
				for (int32 i = 0; i < ViewModelFieldNotifyBindings.Num(); ++i)
				{
					const FMDViewModelFieldNotifyBindingEntry& Entry = ViewModelFieldNotifyBindings[i];
					auto Delegate = FMDVMOnViewModelSet::FDelegate::CreateUObject(this, &UMDViewModelFieldNotifyBinding::OnViewModelChanged, i, WeakWidget);
					Extension->QueueListenForChanges(Widget, MoveTemp(Delegate), Entry.ViewModelClass, Entry.ViewModelName);
				}
			}
		}
	}
}

void UMDViewModelFieldNotifyBinding::UnbindDynamicDelegates(UObject* InInstance) const
{
	if (UUserWidget* Widget = Cast<UUserWidget>(InInstance))
	{
		UMDViewModelWidgetExtension* Extension = Widget->GetExtension<UMDViewModelWidgetExtension>();
		if (IsValid(Extension))
		{
			Extension->StopListeningForAllNativeViewModelsChanged(this);
		}
	}
}

void UMDViewModelFieldNotifyBinding::OnViewModelChanged(UMDViewModelBase* OldViewModel, UMDViewModelBase* NewViewModel, int32 EntryIndex, TWeakObjectPtr<UUserWidget> BoundWidget) const
{
	if (!ensure(ViewModelFieldNotifyBindings.IsValidIndex(EntryIndex)) || !BoundWidget.IsValid())
	{
		return;
	}

	const FMDViewModelFieldNotifyBindingEntry& Entry = ViewModelFieldNotifyBindings[EntryIndex];

	if (IsValid(OldViewModel))
	{
		FDelegateHandle* HandlePtr = BoundDelegates.Find(BoundWidget);
		if (HandlePtr != nullptr && HandlePtr->IsValid())
		{
			const UE::FieldNotification::FFieldId FieldId = OldViewModel->GetFieldNotificationDescriptor().GetField(Entry.ViewModelClass, Entry.FieldNotifyName);
			OldViewModel->RemoveFieldValueChangedDelegate(FieldId, *HandlePtr);
			HandlePtr->Reset();
		}
	}

	if (IsValid(NewViewModel))
	{
		const UE::FieldNotification::FFieldId FieldId = NewViewModel->GetFieldNotificationDescriptor().GetField(Entry.ViewModelClass, Entry.FieldNotifyName);
		const INotifyFieldValueChanged::FFieldValueChangedDelegate Delegate = INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(this, &UMDViewModelFieldNotifyBinding::OnFieldValueChanged, EntryIndex, BoundWidget);
		BoundDelegates.FindOrAdd(BoundWidget) = NewViewModel->AddFieldValueChangedDelegate(FieldId, Delegate);

		// Execute with the currently held value
		OnFieldValueChanged(NewViewModel, FieldId, EntryIndex, BoundWidget);
	}
}

void UMDViewModelFieldNotifyBinding::OnFieldValueChanged(UObject* ViewModel, UE::FieldNotification::FFieldId Field, int32 EntryIndex, TWeakObjectPtr<UUserWidget> BoundWidget) const
{
	UMDViewModelBase* BoundViewModel = Cast<UMDViewModelBase>(ViewModel);
	if (!ensure(ViewModelFieldNotifyBindings.IsValidIndex(EntryIndex)) || !BoundWidget.IsValid() || !IsValid(BoundViewModel))
	{
		return;
	}

	const FMDViewModelFieldNotifyBindingEntry& Entry = ViewModelFieldNotifyBindings[EntryIndex];

	if (UFunction* BoundFunc = BoundWidget->FindFunctionChecked(Entry.FunctionNameToBind))
	{
		check(IsValid(BoundFunc) && BoundFunc->NumParms == 1);

		void* ValuePtr = nullptr;
		void* AllocatedParamPtr = nullptr;
		if (const FProperty* Prop = FindFProperty<FProperty>(BoundViewModel->GetClass(), Field.GetName()))
		{
			check(BoundFunc->ParmsSize == Prop->GetSize());
			ValuePtr = Prop->ContainerPtrToValuePtr<void>(BoundViewModel);
		}
		else if (UFunction* Func = FindUField<UFunction>(BoundViewModel->GetClass(), Field.GetName()))
		{
			const FProperty* ReturnProp = Func->GetReturnProperty();
			check(ReturnProp != nullptr && Func->NumParms == 1);

			AllocatedParamPtr = FMemory::Malloc(ReturnProp->GetSize(), ReturnProp->GetMinAlignment());
			ReturnProp->InitializeValue(AllocatedParamPtr);

			check(Func->ParmsSize == ReturnProp->GetSize());
			check(BoundFunc->ParmsSize == ReturnProp->GetSize());

			BoundViewModel->ProcessEvent(Func, AllocatedParamPtr);

			ValuePtr = AllocatedParamPtr;
		}

		if (ValuePtr != nullptr)
		{
			BoundWidget->ProcessEvent(BoundFunc, ValuePtr);
		}

		if (AllocatedParamPtr != nullptr)
		{
			FMemory::Free(AllocatedParamPtr);
			AllocatedParamPtr = nullptr;
		}
	}
}
