#include "ViewModelTab/FieldInspector/MDViewModelFieldInspector.h"

#include "BlueprintEditor.h"
#include "EdGraphSchema_K2.h"
#include "UObject/WeakFieldPtr.h"
#include "ViewModel/MDViewModelBase.h"
#include "ViewModelTab/FieldInspector/DragAndDrop/MDVMInspectorDragAndDropCommand.h"
#include "ViewModelTab/FieldInspector/DragAndDrop/MDVMInspectorDragAndDropFunctionBase.h"
#include "ViewModelTab/FieldInspector/DragAndDrop/MDVMInspectorDragAndDropGetter.h"
#include "ViewModelTab/FieldInspector/DragAndDrop/MDVMInspectorDragAndDropHelper.h"
#include "ViewModelTab/FieldInspector/MDViewModelChangedDebugLineItem.h"
#include "ViewModelTab/FieldInspector/MDViewModelEventDebugLineItem.h"
#include "ViewModelTab/FieldInspector/MDViewModelFieldDebugLineItem.h"
#include "Widgets/Input/SButton.h"

void SMDViewModelFieldInspector::Construct(const FArguments& InArgs, const TSharedPtr<FBlueprintEditor>& BlueprintEditor)
{
	BlueprintEditorPtr = BlueprintEditor;

	InspectorType = InArgs._InspectorType;

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

	if (InspectorType == EMDViewModelFieldInspectorType::Events)
	{
		if (!VMChangedItem.IsValid())
		{
			VMChangedItem = MakeShared<FMDViewModelChangedDebugLineItem>(BlueprintEditorPtr, ViewModelClass, ViewModelName);
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

	if (InspectorType == EMDViewModelFieldInspectorType::Properties || InspectorType == EMDViewModelFieldInspectorType::Events)
	{
		for (TFieldIterator<const FProperty> It(ViewModelClass); It; ++It)
		{
			if (const FProperty* Prop = *It)
			{
				if (Prop->GetOwnerUObject() == UMDViewModelBase::StaticClass())
				{
					continue;
				}

				const bool bIsFieldNotify = FieldNotifySupportedNames.Contains(Prop->GetFName());
				if (InspectorType == EMDViewModelFieldInspectorType::Properties && Prop->HasAnyPropertyFlags(CPF_BlueprintVisible))
				{
					void* ValuePtr = bIsDebugging ? Prop->ContainerPtrToValuePtr<void>(DebugViewModel.Get()) : nullptr;
					TSharedPtr<FMDViewModelFieldDebugLineItem>& Item = PropertyTreeItems.FindOrAdd(Prop);
					if (!Item.IsValid())
					{
						Item = MakeShared<FMDViewModelFieldDebugLineItem>(Prop, ValuePtr, Prop->GetDisplayNameText(), Prop->GetToolTipText(), DebugViewModel, BlueprintEditorPtr, bIsFieldNotify, ViewModelClass, ViewModelName);
					}
					else
					{
						Item->UpdateViewModel(ViewModelName, ViewModelClass);
						Item->UpdateValuePtr(ValuePtr);
					}

					AddTreeItemUnique(Item);
				}
				else if (InspectorType == EMDViewModelFieldInspectorType::Events && Prop->HasAnyPropertyFlags(CPF_BlueprintAssignable))
				{
					if (const FMulticastDelegateProperty* DelegateProp = CastField<const FMulticastDelegateProperty>(Prop))
					{
						TSharedPtr<FMDViewModelEventDebugLineItem>& Item = EventTreeItems.FindOrAdd(DelegateProp);
						if (!Item.IsValid())
						{
							Item = MakeShared<FMDViewModelEventDebugLineItem>(DelegateProp, DebugViewModel, BlueprintEditorPtr, false, ViewModelClass, ViewModelName);
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
	}

	if (InspectorType != EMDViewModelFieldInspectorType::Events)
	{
		for (TFieldIterator<UFunction> It(ViewModelClass); It; ++It)
		{
			if (const UFunction* Func = *It)
			{
				if (Func->GetOuterUClass() == UMDViewModelBase::StaticClass())
				{
					continue;
				}

				// We only want visible functions that are actually callable on the view model instance
				if (Func->HasAnyFunctionFlags(FUNC_Static | FUNC_Private) || !Func->HasAnyFunctionFlags(FUNC_BlueprintCallable))
				{
					continue;
				}

				const bool bIsFieldNotify = FieldNotifySupportedNames.Contains(Func->GetFName());
				FMDVMDragAndDropCreatorFunc DragAndDropCreatorFunc;
				GetDragAndDropCreatorForFunction(*Func, bIsFieldNotify, DragAndDropCreatorFunc);

				if (DragAndDropCreatorFunc.IsBound())
				{
					TSharedPtr<FMDViewModelFunctionDebugLineItem>& Item = FunctionTreeItems.FindOrAdd(Func);
					if (!Item.IsValid())
					{
						Item = MakeShared<FMDViewModelFunctionDebugLineItem>(Func, Func->GetDisplayNameText(), Func->GetToolTipText(), DebugViewModel, BlueprintEditorPtr, DragAndDropCreatorFunc, bIsFieldNotify, ViewModelClass, ViewModelName);
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
}

void SMDViewModelFieldInspector::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SPinValueInspector::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (bIsDebugging && !DebugViewModel.IsValid())
	{
		RefreshList();
	}
}

void SMDViewModelFieldInspector::GetDragAndDropCreatorForFunction(const UFunction& Func, bool bIsFieldNotify, FMDVMDragAndDropCreatorFunc& DragAndDropCreatorFunc) const
{
	const bool bIsPure = Func.HasAllFunctionFlags(FUNC_BlueprintPure);

	if (InspectorType == EMDViewModelFieldInspectorType::Properties)
	{
		if (bIsFieldNotify || (Func.GetReturnProperty() != nullptr && Func.NumParms == 1 && bIsPure))
		{
			DragAndDropCreatorFunc = FMDVMDragAndDropCreatorFunc::CreateStatic(&CreateMDVMDragAndDrop<FMDVMInspectorDragAndDropGetter>);
		}
	}
	else if (InspectorType == EMDViewModelFieldInspectorType::Events)
	{
		// Functions are never displayed under Events
	}
	else if (InspectorType == EMDViewModelFieldInspectorType::Commands)
	{
		if (!bIsPure)
		{
			DragAndDropCreatorFunc = FMDVMDragAndDropCreatorFunc::CreateStatic(&CreateMDVMDragAndDrop<FMDVMInspectorDragAndDropCommand>);
		}
	}
	else if (InspectorType == EMDViewModelFieldInspectorType::Helpers)
	{
		// Skip anything that's a "Getter"
		if (bIsPure && !bIsFieldNotify && (Func.GetReturnProperty() == nullptr || Func.NumParms != 1))
		{
			DragAndDropCreatorFunc = FMDVMDragAndDropCreatorFunc::CreateStatic(&CreateMDVMDragAndDrop<FMDVMInspectorDragAndDropHelper>);
		}
	}
}
