#include "ViewModelTab/FieldInspector/MDViewModelFieldDebugLineItem.h"

#include "BlueprintEditor.h"
#include "EdGraphSchema_K2.h"
#include "Kismet2/KismetDebugUtilities.h"
#include "Nodes/MDVMNode_ViewModelFieldNotify.h"
#include "Util/MDViewModelGraphStatics.h"
#include "ViewModel/MDViewModelBase.h"
#include "ViewModelTab/FieldInspector/DragAndDrop/MDVMDragAndDropWrapperButton.h"
#include "ViewModelTab/FieldInspector/DragAndDrop/MDVMInspectorDragAndDropProperty.h"
#include "Widgets/Images/SLayeredImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SWidgetSwitcher.h"

bool FMDViewModelFieldDebugLineItem::Compare(const FDebugLineItem* BaseOther) const
{
	const FMDViewModelFieldDebugLineItem* Other = static_cast<const FMDViewModelFieldDebugLineItem*>(BaseOther);
	return PropertyPtr.Get() == Other->PropertyPtr.Get() && ValuePtr == Other->ValuePtr;
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
			.bCanDrag(true)
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
		.bCanDrag(true)
		[
			FMDViewModelDebugLineItemBase::GenerateNameWidget(InSearchString)
		];
}

TSharedRef<SWidget> FMDViewModelFieldDebugLineItem::GenerateValueWidget(TSharedPtr<FString> InSearchString)
{
	return SNew(SMDVMDragAndDropWrapperButton, StaticCastSharedRef<FMDViewModelFieldDebugLineItem>(AsShared()))
		.bCanDrag(true)
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
	const FProperty* Property = PropertyPtr.Get();
	if (DebugViewModel.IsValid() && Property != nullptr && ValuePtr != nullptr)
	{
		if (Property->IsA<FObjectProperty>() || Property->IsA<FInterfaceProperty>())
		{
			const UObject* Object = nullptr;
			if (const FObjectPropertyBase* ObjectPropertyBase = CastField<FObjectPropertyBase>(Property))
			{
				Object = ObjectPropertyBase->GetObjectPropertyValue(ValuePtr);
			}
			else if (Property->IsA<FInterfaceProperty>())
			{
				const FScriptInterface* InterfaceData = static_cast<const FScriptInterface*>(ValuePtr);
				Object = InterfaceData->GetObject();
			}

			if (Object != nullptr)
			{
				return FText::FromString(FString::Printf(TEXT("%s (Class: %s)"), *Object->GetName(), *Object->GetClass()->GetName()));
			}
			else
			{
				return INVTEXT("[None]");
			}
		}
		else if (Property->IsA<FStructProperty>())
		{
			if (!CachedChildren.IsSet())
			{
				UpdateCachedChildren();
			}

			return FText::Format(INVTEXT("{0} {0}|plural(one=member,other=members)"), FText::AsNumber(CachedChildren.GetValue().Num()));
		}
		else
		{
			TSharedPtr<FPropertyInstanceInfo> DebugInfo;
			FKismetDebugUtilities::GetDebugInfoInternal(DebugInfo, Property, ValuePtr);
			return DebugInfo->Value;
		}
	}

	return FText::GetEmpty();
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
						CachedPropertyItems.Add(ChildProp->GetFName(),
							MakeShared<FMDViewModelFieldDebugLineItem>(ChildProp, ChildValuePtr, ChildProp->GetDisplayNameText(), ChildProp->GetToolTipText(), DebugViewModel, BlueprintEditorPtr));
					}
					else
					{
						TSharedPtr<FMDViewModelFieldDebugLineItem> Item = StaticCastSharedPtr<FMDViewModelFieldDebugLineItem>(CachedPropertyItems[ChildProp->GetFName()]);
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
						CachedPropertyItems.Add(PropertyName,
							MakeShared<FMDViewModelFieldDebugLineItem>(ArrayProp->Inner, ChildValuePtr, ElementDisplayName, Prop->GetToolTipText(), DebugViewModel, BlueprintEditorPtr));
					}
					else
					{
						TSharedPtr<FMDViewModelFieldDebugLineItem> Item = StaticCastSharedPtr<FMDViewModelFieldDebugLineItem>(CacheCopy[PropertyName]);
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
						CachedPropertyItems.Add(PropertyName,
							MakeShared<FMDViewModelFieldDebugLineItem>(SetProp->ElementProp, ChildValuePtr, ElementDisplayName, Prop->GetToolTipText(), DebugViewModel, BlueprintEditorPtr));
					}
					else
					{
						TSharedPtr<FMDViewModelFieldDebugLineItem> Item = StaticCastSharedPtr<FMDViewModelFieldDebugLineItem>(CacheCopy[PropertyName]);
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
						CachedPropertyItems.Add(KeyPropertyName,
							MakeShared<FMDViewModelFieldDebugLineItem>(MapProp->KeyProp, ChildKeyPtr, KeyDisplayName, Prop->GetToolTipText(), DebugViewModel, BlueprintEditorPtr));
					}
					else
					{
						TSharedPtr<FMDViewModelFieldDebugLineItem> Item = StaticCastSharedPtr<FMDViewModelFieldDebugLineItem>(CacheCopy[KeyPropertyName]);
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
						CachedPropertyItems.Add(ValuePropertyName,
							MakeShared<FMDViewModelFieldDebugLineItem>(MapProp->ValueProp, ChildValuePtr, ValueDisplayName, Prop->GetToolTipText(), DebugViewModel, BlueprintEditorPtr));
					}
					else
					{
						TSharedPtr<FMDViewModelFieldDebugLineItem> Item = StaticCastSharedPtr<FMDViewModelFieldDebugLineItem>(CacheCopy[ValuePropertyName]);
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
	return new FMDViewModelFieldDebugLineItem(PropertyPtr.Get(), ValuePtr, DisplayName, Description, DebugViewModel, BlueprintEditorPtr, bIsFieldNotify, ViewModelClass, ViewModelName);
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
		const UMDVMNode_ViewModelFieldNotify* Node = FMDViewModelGraphStatics::FindExistingViewModelFieldNotifyNode(BlueprintPtr.Get(), PropertyPtr->GetFName(), ViewModelClass, ViewModelName);
		if (IsValid(Node))
		{
			Result += Node->GetFindReferenceSearchString();
		}
	}

	{
		FMemberReference VariableReference;
		VariableReference.SetFromField<FProperty>(PropertyPtr.Get(), false, ViewModelClass);
		const FString VariableString = VariableReference.GetReferenceSearchString(VariableReference.IsLocalScope() ? nullptr : ViewModelClass);
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

int32 FMDViewModelFieldDebugLineItem::GetShouldDisplayFieldNotifyIndex() const
{
	const bool bHasFieldNotifyValue = !DebugViewModel.IsValid() && ValuePtr == nullptr && bIsFieldNotify; 
	return bHasFieldNotifyValue ? 0 : 1;
}

FReply FMDViewModelFieldDebugLineItem::OnAddOrViewBoundFunctionClicked() const
{
	check(bIsFieldNotify);

	if (PropertyPtr.IsValid())
	{
		FMDViewModelGraphStatics::OnViewModelFieldNotifyRequestedForBlueprint(BlueprintPtr.Get(), PropertyPtr->GetFName(), ViewModelClass, ViewModelName);
	}

	return FReply::Handled();
}

int32 FMDViewModelFieldDebugLineItem::GetAddOrViewBoundFunctionIndex() const
{
	check(bIsFieldNotify);

	return (!PropertyPtr.IsValid() || FMDViewModelGraphStatics::DoesBlueprintBindToViewModelFieldNotify(BlueprintPtr.Get(), PropertyPtr->GetFName(), ViewModelClass, ViewModelName))
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
