#include "ViewModelTab/FieldInspector/MDViewModelFieldInspector.h"

#include "EdGraphSchema_K2.h"
#include "WidgetBlueprint.h"
#include "UObject/WeakFieldPtr.h"
#include "ViewModel/MDViewModelBase.h"
#include "ViewModelTab/FieldInspector/MDViewModelChangedDebugLineItem.h"
#include "ViewModelTab/FieldInspector/MDViewModelEventDebugLineItem.h"
#include "ViewModelTab/FieldInspector/MDViewModelFieldDebugLineItem.h"
#include "Widgets/Input/SButton.h"

void SMDViewModelFieldInspector::Construct(const FArguments& InArgs, UWidgetBlueprint* WidgetBP)
{
	WidgetBPPtr = WidgetBP;

	bIncludeBlueprintVisibleProperties = InArgs._bIncludeBlueprintVisibleProperties;
	bIncludeBlueprintAssignableProperties = InArgs._bIncludeBlueprintAssignableProperties;
	bIncludeBlueprintCallable = InArgs._bIncludeBlueprintCallable;
	bIncludeBlueprintPure = InArgs._bIncludeBlueprintPure;
	bIncludeFieldNotifyFunctions = InArgs._bIncludeFieldNotifyFunctions;

	SPinValueInspector::Construct({});
}

void SMDViewModelFieldInspector::SetReferences(TSubclassOf<UMDViewModelBase> InViewModelClass, UMDViewModelBase* InDebugViewModel, const FName& InViewModelName)
{
	ViewModelClass = InViewModelClass;
	DebugViewModel = InDebugViewModel;
	ViewModelName = InViewModelName;

	RefreshList();
}

void SMDViewModelFieldInspector::RefreshList()
{
	// Hack to refresh the list since it's not exposed
	SetPinRef({});
}

void SMDViewModelFieldInspector::PopulateTreeView()
{
	if (!IsValid(ViewModelClass))
	{
		return;
	}

	bIsDebugging = DebugViewModel.IsValid();

	if (bIncludeBlueprintAssignableProperties)
	{
		if (!VMChangedItem.IsValid())
		{
			VMChangedItem = MakeShared<FMDViewModelChangedDebugLineItem>(WidgetBPPtr.Get(), ViewModelClass, ViewModelName);
		}
		else
		{
			VMChangedItem->UpdateViewModel(ViewModelName, ViewModelClass);
		}

		AddTreeItemUnique(VMChangedItem);
	}

	TSet<FName> FieldNotifySupportedNames;
	{
		const TScriptInterface<INotifyFieldValueChanged> DefaultVM = ViewModelClass->GetDefaultObject();
		const UE::FieldNotification::IClassDescriptor& ClassDescriptor = DefaultVM->GetFieldNotificationDescriptor();
		ClassDescriptor.ForEachField(ViewModelClass, [&FieldNotifySupportedNames](UE::FieldNotification::FFieldId FieldId)
		{
			FieldNotifySupportedNames.Add(FieldId.GetName());
			return true;
		});
	}

	for (TFieldIterator<const FProperty> It(ViewModelClass); It; ++It)
	{
		if (const FProperty* Prop = *It)
		{
			if (Prop->GetOwnerUObject() == UMDViewModelBase::StaticClass())
			{
				continue;
			}

			const bool bIsFieldNotify = FieldNotifySupportedNames.Contains(Prop->GetFName());
			if (bIncludeBlueprintVisibleProperties && Prop->HasAnyPropertyFlags(CPF_BlueprintVisible))
			{
				void* ValuePtr = bIsDebugging ? Prop->ContainerPtrToValuePtr<void>(DebugViewModel.Get()) : nullptr;
				TSharedPtr<FMDViewModelFieldDebugLineItem>& Item = PropertyTreeItems.FindOrAdd(Prop);
				if (!Item.IsValid())
				{
					Item = MakeShared<FMDViewModelFieldDebugLineItem>(Prop, ValuePtr, Prop->GetDisplayNameText(), Prop->GetToolTipText(), DebugViewModel, bIsFieldNotify, WidgetBPPtr.Get(), ViewModelClass, ViewModelName);
				}
				else
				{
					Item->UpdateViewModel(ViewModelName, ViewModelClass);
					Item->UpdateValuePtr(ValuePtr);
				}

				AddTreeItemUnique(Item);
			}
			else if (bIncludeBlueprintAssignableProperties && Prop->HasAnyPropertyFlags(CPF_BlueprintAssignable))
			{
				if (const FMulticastDelegateProperty* DelegateProp = CastField<const FMulticastDelegateProperty>(Prop))
				{
					TSharedPtr<FMDViewModelEventDebugLineItem>& Item = EventTreeItems.FindOrAdd(DelegateProp);
					if (!Item.IsValid())
					{
						Item = MakeShared<FMDViewModelEventDebugLineItem>(DelegateProp, DebugViewModel, false, WidgetBPPtr.Get(), ViewModelClass, ViewModelName);
					}
					else
					{
						Item->UpdateViewModel(ViewModelName, ViewModelClass);
					}

					AddTreeItemUnique(Item);
				}
			}
		}
	}

	for (TFieldIterator<UFunction> It(ViewModelClass); It; ++It)
	{
		if (const UFunction* Func = *It)
		{
			if (Func->GetOuterUClass() == UMDViewModelBase::StaticClass())
			{
				continue;
			}

			bool bIsCommand = false;
			bool bIsGetter = false;
			const bool bIsFieldNotify = FieldNotifySupportedNames.Contains(Func->GetFName());
			const bool bShouldAddFunction = [&]()
			{
				if (bIncludeFieldNotifyFunctions && bIsFieldNotify)
				{
					bIsGetter = true;
					return true;
				}

				const bool bIsPure = Func->HasAnyFunctionFlags(FUNC_BlueprintPure);
				if (bIncludeBlueprintPure && bIsPure && Func->GetReturnProperty() != nullptr && Func->NumParms == 1)
				{
					bIsGetter = true;
					return true;
				}

				if (!bIsPure && bIncludeBlueprintCallable && Func->HasAnyFunctionFlags(FUNC_BlueprintCallable) && !Func->HasAnyFunctionFlags(FUNC_Static))
				{
					bIsCommand = true;
					return true;
				}

				return false;
			}();

			if (bShouldAddFunction)
			{
				TSharedPtr<FMDViewModelFunctionDebugLineItem>& Item = FunctionTreeItems.FindOrAdd(Func);
				if (!Item.IsValid())
				{
					Item = MakeShared<FMDViewModelFunctionDebugLineItem>(Func, Func->GetDisplayNameText(), Func->GetToolTipText(), DebugViewModel, bIsCommand, bIsGetter, bIsFieldNotify, WidgetBPPtr.Get(), ViewModelClass, ViewModelName);
				}
				else
				{
					Item->UpdateViewModel(ViewModelName, ViewModelClass);
				}

				Item->UpdateIsDebugging(bIsDebugging);
				AddTreeItemUnique(Item);
			}
		}
	}
}

void SMDViewModelFieldInspector::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SPinValueInspector::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (bIsDebugging && !DebugViewModel.IsValid())
	{
		RefreshList();
	}
}
