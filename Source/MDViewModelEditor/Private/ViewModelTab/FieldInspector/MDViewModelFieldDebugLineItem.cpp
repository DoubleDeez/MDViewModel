#include "ViewModelTab/FieldInspector/MDViewModelFieldDebugLineItem.h"

#include "BlueprintEditor.h"
#include "EdGraphSchema_K2.h"
#include "Nodes/MDVMNode_ViewModelFieldNotify.h"
#include "Util/MDViewModelGraphStatics.h"
#include "ViewModel/MDViewModelBase.h"
#include "ViewModelTab/FieldInspector/DragAndDrop/MDVMDragAndDropWrapperButton.h"
#include "ViewModelTab/FieldInspector/DragAndDrop/MDVMInspectorDragAndDropProperty.h"
#include "Widgets/Images/SLayeredImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SWidgetSwitcher.h"

FMDViewModelFieldDebugLineItem::FMDViewModelFieldDebugLineItem(const TWeakPtr<FBlueprintEditor>& BlueprintEditorPtr, const FMDViewModelAssignmentReference& Assignment, const FProperty* Property, void* InValuePtr, bool bIsFieldNotify)
	: FMDViewModelDebugLineItemBase(BlueprintEditorPtr, Assignment)
	, PropertyPtr(Property)
	, ValuePtr(InValuePtr)
	, bIsFieldNotify(bIsFieldNotify)
{
}

TSharedRef<SWidget> FMDViewModelFieldDebugLineItem::GetNameIcon()
{
	if (const FProperty* ItemProperty = PropertyPtr.Get())
	{
		FSlateColor BaseColor;
		FSlateColor SecondaryColor;
		FSlateBrush const* SecondaryIcon = nullptr;
		const FSlateBrush* IconBrush = FBlueprintEditor::GetVarIconAndColorFromProperty(
			ItemProperty,
			BaseColor,
			SecondaryIcon,
			SecondaryColor
		);

		return SNew(SMDVMDragAndDropWrapperButton, StaticCastSharedRef<FMDViewModelFieldDebugLineItem>(AsShared()))
			.bCanDrag(this, &FMDViewModelFieldDebugLineItem::CanDrag)
			[
				SNew(SLayeredImage, SecondaryIcon, SecondaryColor)
				.Image(IconBrush)
				.ColorAndOpacity(BaseColor)
				.ToolTipText(UEdGraphSchema_K2::TypeToText(ItemProperty))
			];
	}

	return FDebugLineItem::GetNameIcon();
}

TSharedRef<SWidget> FMDViewModelFieldDebugLineItem::GenerateNameWidget(TSharedPtr<FString> InSearchString)
{
	return SNew(SMDVMDragAndDropWrapperButton, StaticCastSharedRef<FMDViewModelFieldDebugLineItem>(AsShared()))
		.bCanDrag(this, &FMDViewModelFieldDebugLineItem::CanDrag)
		[
			FMDViewModelDebugLineItemBase::GenerateNameWidget(InSearchString)
		];
}

TSharedRef<SWidget> FMDViewModelFieldDebugLineItem::GenerateValueWidget(TSharedPtr<FString> InSearchString)
{
	return SNew(SMDVMDragAndDropWrapperButton, StaticCastSharedRef<FMDViewModelFieldDebugLineItem>(AsShared()))
		.bCanDrag(this, &FMDViewModelFieldDebugLineItem::CanDrag)
		[
			SNew(SWidgetSwitcher)
			.WidgetIndex(this, &FMDViewModelFieldDebugLineItem::GetShouldDisplayFieldNotifyIndex)
			+SWidgetSwitcher::Slot()
			[
				SNew(SButton)
				.ContentPadding(FMargin(3.0, 2.0))
				.OnClicked(this, &FMDViewModelFieldDebugLineItem::OnAddOrViewBoundFunctionClicked)
				.IsEnabled(this, &FMDViewModelFieldDebugLineItem::CanCreateNodes)
				[
					SNew(SWidgetSwitcher)
					.WidgetIndex(this, &FMDViewModelFieldDebugLineItem::GetAddOrViewBoundFunctionIndex)
					+SWidgetSwitcher::Slot()
					[
						SNew(SImage)
						.ColorAndOpacity(FSlateColor::UseForeground())
						.Image(FAppStyle::Get().GetBrush("Icons.SelectInViewport"))
						.ToolTipText(INVTEXT("Focus the existing bound function."))
					]
					+SWidgetSwitcher::Slot()
					[
						SNew(SImage)
						.ColorAndOpacity(FSlateColor::UseForeground())
						.Image(FAppStyle::Get().GetBrush("Icons.Plus"))
						.ToolTipText(INVTEXT("Create a BP event bound to this view model field notify property"))
					]
				]
			]
			+SWidgetSwitcher::Slot()
			[
				SNew(STextBlock)
				.Text(this, &FMDViewModelFieldDebugLineItem::GetDisplayValue)
				.ToolTipText(this, &FMDViewModelFieldDebugLineItem::GetDisplayValue)
			]
		];
}

TSharedRef<FMDVMInspectorDragAndDropActionBase> FMDViewModelFieldDebugLineItem::CreateDragAndDropAction() const
{
	return FMDVMInspectorDragAndDropProperty::Create(
		PropertyPtr,
		GetViewModelAssignmentReference()
	);
}

FText FMDViewModelFieldDebugLineItem::GetDisplayValue() const
{
	return GeneratePropertyDisplayValue(PropertyPtr.Get(), ValuePtr);
}

void FMDViewModelFieldDebugLineItem::UpdateCachedChildren() const
{
	CachedChildren = TArray<FDebugTreeItemPtr>();

	if (const FProperty* Prop = PropertyPtr.Get())
	{
		const UStruct* PropertyStruct = nullptr;
		void* PropertyValue = nullptr;
		if (const FObjectPropertyBase* ObjectProp = CastField<FObjectPropertyBase>(Prop))
		{
			PropertyStruct = ObjectProp->PropertyClass;
			if (ValuePtr != nullptr)
			{
				PropertyValue = ObjectProp->GetObjectPropertyValue(ValuePtr);
			}
		}
		else if (const FStructProperty* StructProp = CastField<FStructProperty>(Prop))
		{
			PropertyStruct = StructProp->Struct;
			PropertyValue = ValuePtr;
		}
		else if (Prop->IsA<FInterfaceProperty>())
		{
			// Interface property won't have the class so we need the actual value to get it
			if (const FScriptInterface* InterfaceData = static_cast<const FScriptInterface*>(ValuePtr))
			{
				if (UObject* Object = InterfaceData->GetObject())
				{
					PropertyStruct = Object->GetClass();
					PropertyValue = Object;
				}
			}
		}

		if (PropertyStruct != nullptr)
		{
			for (TFieldIterator<FProperty> It(PropertyStruct); It; ++It)
			{
				if (const FProperty* ChildProp = *It)
				{
					void* ChildValuePtr = PropertyValue != nullptr ? ChildProp->ContainerPtrToValuePtr<void>(PropertyValue) : nullptr;
					if (!CachedPropertyItems.Contains(ChildProp->GetFName()))
					{
						TSharedPtr<FMDViewModelFieldDebugLineItem> Item = MakeShared<FMDViewModelFieldDebugLineItem>(BlueprintEditorPtr, Assignment, ChildProp, ChildValuePtr);
						Item->SetDisplayText(ChildProp->GetDisplayNameText(), ChildProp->GetToolTipText());
						Item->UpdateDebugging(bIsDebugging, DebugViewModel);
						CachedPropertyItems.Add(ChildProp->GetFName(), Item);
					}
					else
					{
						TSharedPtr<FMDViewModelFieldDebugLineItem> Item = StaticCastSharedPtr<FMDViewModelFieldDebugLineItem>(CachedPropertyItems[ChildProp->GetFName()]);
						Item->UpdateDebugging(bIsDebugging, DebugViewModel);
						Item->UpdateViewModel(Assignment);
						if (Item->GetValuePtr() != ChildValuePtr)
						{
							Item->UpdateValuePtr(ChildValuePtr);
						}
					}
				}
			}
		}
		else if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(Prop))
		{
			if (ValuePtr != nullptr)
			{
				TMap<FName, FDebugTreeItemPtr> CacheCopy = CachedPropertyItems;
				CachedPropertyItems.Reset();

				FScriptArrayHelper Helper = FScriptArrayHelper(ArrayProp, ValuePtr);
				for (int32 i = 0; i < Helper.Num(); ++i)
				{
					void* ChildValuePtr = Helper.GetRawPtr(i);
					const FText ElementDisplayName = FText::Format(INVTEXT("[{0}]"), FText::AsNumber(i));
					const FName PropertyName = *FString::Printf(TEXT("%d"), i);
					if (!CacheCopy.Contains(PropertyName))
					{
						TSharedPtr<FMDViewModelFieldDebugLineItem> Item = MakeShared<FMDViewModelFieldDebugLineItem>(BlueprintEditorPtr, Assignment, ArrayProp->Inner, ChildValuePtr);
						Item->SetDisplayText(ElementDisplayName, Prop->GetToolTipText());
						Item->UpdateDebugging(bIsDebugging, DebugViewModel);
						CachedPropertyItems.Add(PropertyName, Item);
					}
					else
					{
						TSharedPtr<FMDViewModelFieldDebugLineItem> Item = StaticCastSharedPtr<FMDViewModelFieldDebugLineItem>(CacheCopy[PropertyName]);
						Item->UpdateDebugging(bIsDebugging, DebugViewModel);
						Item->UpdateViewModel(Assignment);
						if (Item->GetValuePtr() != ChildValuePtr)
						{
							Item->UpdateValuePtr(ChildValuePtr);
						}

						CachedPropertyItems.Add(PropertyName, Item);
					}
				}
			}
			else
			{
				CachedPropertyItems.Reset();
			}
		}
		else if (const FSetProperty* SetProp = CastField<FSetProperty>(Prop))
		{
			if (ValuePtr != nullptr)
			{
				TMap<FName, FDebugTreeItemPtr> CacheCopy = CachedPropertyItems;
				CachedPropertyItems.Reset();

				FScriptSetHelper Helper = FScriptSetHelper(SetProp, ValuePtr);
				for (int32 i = 0; i < Helper.Num(); ++i)
				{
					void* ChildValuePtr = Helper.GetElementPtr(i);
					const FText ElementDisplayName = FText::Format(INVTEXT("[{0}]"), FText::AsNumber(i));
					const FName PropertyName = *FString::Printf(TEXT("%d"), i);
					if (!CacheCopy.Contains(PropertyName))
					{
						TSharedPtr<FMDViewModelFieldDebugLineItem> Item = MakeShared<FMDViewModelFieldDebugLineItem>(BlueprintEditorPtr, Assignment, SetProp->ElementProp, ChildValuePtr);
						Item->SetDisplayText(ElementDisplayName, Prop->GetToolTipText());
						Item->UpdateDebugging(bIsDebugging, DebugViewModel);
						CachedPropertyItems.Add(PropertyName, Item);
					}
					else
					{
						TSharedPtr<FMDViewModelFieldDebugLineItem> Item = StaticCastSharedPtr<FMDViewModelFieldDebugLineItem>(CacheCopy[PropertyName]);
						Item->UpdateDebugging(bIsDebugging, DebugViewModel);
						Item->UpdateViewModel(Assignment);
						if (Item->GetValuePtr() != ChildValuePtr)
						{
							Item->UpdateValuePtr(ChildValuePtr);
						}

						CachedPropertyItems.Add(PropertyName, Item);
					}
				}
			}
			else
			{
				CachedPropertyItems.Reset();
			}
		}
		else if (const FMapProperty* MapProp = CastField<FMapProperty>(Prop))
		{
			if (ValuePtr != nullptr)
			{
				TMap<FName, FDebugTreeItemPtr> CacheCopy = CachedPropertyItems;
				CachedPropertyItems.Reset();

				FScriptMapHelper Helper = FScriptMapHelper(MapProp, ValuePtr);
				for (int32 i = 0; i < Helper.Num(); ++i)
				{
					// Key
					void* ChildKeyPtr = Helper.GetKeyPtr(i);
					const FText KeyDisplayName = FText::Format(INVTEXT("Key[{0}]"), FText::AsNumber(i));
					const FName KeyPropertyName = *FString::Printf(TEXT("Key[%d]"), i);
					if (!CacheCopy.Contains(KeyPropertyName))
					{
						TSharedPtr<FMDViewModelFieldDebugLineItem> Item = MakeShared<FMDViewModelFieldDebugLineItem>(BlueprintEditorPtr, Assignment, MapProp->KeyProp, ChildKeyPtr);
						Item->SetDisplayText(KeyDisplayName, Prop->GetToolTipText());
						Item->UpdateDebugging(bIsDebugging, DebugViewModel);
						CachedPropertyItems.Add(KeyPropertyName, Item);
					}
					else
					{
						TSharedPtr<FMDViewModelFieldDebugLineItem> Item = StaticCastSharedPtr<FMDViewModelFieldDebugLineItem>(CacheCopy[KeyPropertyName]);
						Item->UpdateDebugging(bIsDebugging, DebugViewModel);
						Item->UpdateViewModel(Assignment);
						if (Item->GetValuePtr() != ChildKeyPtr)
						{
							Item->UpdateValuePtr(ChildKeyPtr);
						}

						CachedPropertyItems.Add(KeyPropertyName, Item);
					}

					// Value
					void* ChildValuePtr = Helper.GetValuePtr(i);
					const FText ValueDisplayName = FText::Format(INVTEXT("Value[{0}]"), FText::AsNumber(i));
					const FName ValuePropertyName = *FString::Printf(TEXT("Value[%d]"), i);
					if (!CacheCopy.Contains(ValuePropertyName))
					{
						TSharedPtr<FMDViewModelFieldDebugLineItem> Item = MakeShared<FMDViewModelFieldDebugLineItem>(BlueprintEditorPtr, Assignment, MapProp->ValueProp, ChildValuePtr);
						Item->SetDisplayText(ValueDisplayName, Prop->GetToolTipText());
						Item->UpdateDebugging(bIsDebugging, DebugViewModel);
						CachedPropertyItems.Add(ValuePropertyName, Item);
					}
					else
					{
						TSharedPtr<FMDViewModelFieldDebugLineItem> Item = StaticCastSharedPtr<FMDViewModelFieldDebugLineItem>(CacheCopy[ValuePropertyName]);
						Item->UpdateDebugging(bIsDebugging, DebugViewModel);
						Item->UpdateViewModel(Assignment);
						if (Item->GetValuePtr() != ChildValuePtr)
						{
							Item->UpdateValuePtr(ChildValuePtr);
						}

						CachedPropertyItems.Add(ValuePropertyName, Item);
					}
				}
			}
			else
			{
				CachedPropertyItems.Reset();
			}
		}
	}

	CachedPropertyItems.GenerateValueArray(CachedChildren.GetValue());
}

FDebugLineItem* FMDViewModelFieldDebugLineItem::Duplicate() const
{
	FMDViewModelFieldDebugLineItem* Item = new FMDViewModelFieldDebugLineItem(BlueprintEditorPtr, Assignment, PropertyPtr.Get(), ValuePtr, bIsFieldNotify);
	Item->SetDisplayText(DisplayName, Description);
	Item->UpdateDebugging(bIsDebugging, DebugViewModel);
	Item->UpdateValuePtr(GetValuePtr());

	return Item;
}

bool FMDViewModelFieldDebugLineItem::CanCreateNodes() const
{
	return FMDViewModelDebugLineItemBase::CanCreateNodes() || GetAddOrViewBoundFunctionIndex() == 0;
}

FString FMDViewModelFieldDebugLineItem::GenerateSearchString() const
{
	FString Result;

	if (bIsFieldNotify && PropertyPtr.IsValid())
	{
		const UMDVMNode_ViewModelFieldNotify* Node = FMDViewModelGraphStatics::FindExistingViewModelFieldNotifyNode(BlueprintPtr.Get(), PropertyPtr->GetFName(), Assignment);
		if (IsValid(Node))
		{
			Result += Node->GetFindReferenceSearchString();
		}
	}

	if (Assignment.IsAssignmentValid())
	{
		FMemberReference VariableReference;
		VariableReference.SetFromField<FProperty>(PropertyPtr.Get(), false, Assignment.ViewModelClass.LoadSynchronous());
		const FString VariableString = VariableReference.GetReferenceSearchString(VariableReference.IsLocalScope() ? nullptr : Assignment.ViewModelClass.LoadSynchronous());
		if (Result.IsEmpty())
		{
			Result = VariableString;
		}
		else
		{
			Result += TEXT(" || ") + VariableString;
		}
	}

	return Result;
}

FFieldVariant FMDViewModelFieldDebugLineItem::GetFieldForDefinitionNavigation() const
{
	return FFieldVariant(PropertyPtr.Get());
}

int32 FMDViewModelFieldDebugLineItem::GetShouldDisplayFieldNotifyIndex() const
{
	const bool bShouldDisplayFieldNotify = bIsFieldNotify && !bIsDebugging;
	return bShouldDisplayFieldNotify ? 0 : 1;
}

FReply FMDViewModelFieldDebugLineItem::OnAddOrViewBoundFunctionClicked() const
{
	check(bIsFieldNotify);

	if (PropertyPtr.IsValid())
	{
		FMDViewModelGraphStatics::OnViewModelFieldNotifyRequestedForBlueprint(BlueprintPtr.Get(), PropertyPtr->GetFName(), Assignment);
	}

	return FReply::Handled();
}

int32 FMDViewModelFieldDebugLineItem::GetAddOrViewBoundFunctionIndex() const
{
	check(bIsFieldNotify);

	return (!PropertyPtr.IsValid() || FMDViewModelGraphStatics::DoesBlueprintBindToViewModelFieldNotify(BlueprintPtr.Get(), PropertyPtr->GetFName(), Assignment))
		? 0 : 1;
}

void FMDViewModelFieldDebugLineItem::UpdateValuePtr(void* InValuePtr)
{
	ValuePtr = InValuePtr;

	// Clear out any child value ptr's so that they don't try to read the wrong data
	for (const auto& Pair : CachedPropertyItems)
	{
		const TSharedPtr<FMDViewModelFieldDebugLineItem> ChildItem = StaticCastSharedPtr<FMDViewModelFieldDebugLineItem>(Pair.Value);
		ChildItem->UpdateValuePtr(nullptr);
	}

	// Invalidate the children so they update
	CachedChildren.Reset();
}

bool FMDViewModelFieldDebugLineItem::CanDrag() const
{
	if (const FProperty* Property = GetProperty())
	{
		const UClass* PropertyOwnerClass = Property->GetOwnerVariant().Get<UClass>();
		const UClass* AssignmentClass = Assignment.ViewModelClass.Get();
		if (IsValid(PropertyOwnerClass) && IsValid(AssignmentClass))
		{
			return PropertyOwnerClass->IsChildOf(AssignmentClass) || AssignmentClass->IsChildOf(PropertyOwnerClass);
		}
	}

	return false;
}
