#include "ViewModelTab/MDViewModelFieldInspector.h"

#include "BlueprintEditor.h"
#include "EdGraphSchema_K2.h"
#include "EdGraphSchema_K2_Actions.h"
#include "PropertyInfoViewStyle.h"
#include "WidgetBlueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetDebugUtilities.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "MDViewModelGraph.h"
#include "UObject/WeakFieldPtr.h"
#include "ViewModel/MDViewModelBase.h"
#include "Widgets/Images/SLayeredImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SWidgetSwitcher.h"


void FMDViewModelDebugLineItemBase::UpdateViewModelName(const FName& InViewModelName)
{
	ViewModelName = InViewModelName;
}

bool FMDViewModelDebugLineItemBase::HasChildren() const
{
	if (!CachedChildren.IsSet())
	{
		UpdateCachedChildren();
	}

	return CachedChildren.GetValue().Num() > 0;
}

void FMDViewModelDebugLineItemBase::GatherChildrenBase(TArray<FDebugTreeItemPtr>& OutChildren, const FString& InSearchString, bool bRespectSearch)
{
	if (!CachedChildren.IsSet())
	{
		UpdateCachedChildren();
	}

	OutChildren.Append(CachedChildren.GetValue());
}

TSharedRef<SWidget> FMDViewModelDebugLineItemBase::GenerateNameWidget(TSharedPtr<FString> InSearchString)
{
	return SNew(PropertyInfoViewStyle::STextHighlightOverlay)
		.FullText(this, &FMDViewModelDebugLineItemBase::GetDisplayName)
		.HighlightText(this, &FDebugLineItem::GetHighlightText, InSearchString)
		[
			SNew(STextBlock)
				.ToolTipText(this, &FDebugLineItem::GetDescription)
				.Text(this, &FMDViewModelDebugLineItemBase::GetDisplayName)
		];
}

FText FMDViewModelDebugLineItemBase::GetDisplayName() const
{
	return DisplayName;
}

FText FMDViewModelDebugLineItemBase::GetDescription() const
{
	return Description;
}

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

		return SNew(SLayeredImage, SecondaryIcon, SecondaryColor)
			.Image(IconBrush)
			.ColorAndOpacity(BaseColor)
			.ToolTipText(UEdGraphSchema_K2::TypeToText(ItemProperty));
	}

	return FDebugLineItem::GetNameIcon();
}

TSharedRef<SWidget> FMDViewModelFieldDebugLineItem::GenerateValueWidget(TSharedPtr<FString> InSearchString)
{
	return SNew(SWidgetSwitcher)
	.WidgetIndex(this, &FMDViewModelFieldDebugLineItem::GetShouldDisplayFieldNotifyIndex)
	+SWidgetSwitcher::Slot()
	[
		SNew(SButton)
		.ContentPadding(FMargin(3.0, 2.0))
		.OnClicked(this, &FMDViewModelFieldDebugLineItem::OnAddOrViewBoundFunctionClicked)
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
	];
}

FText FMDViewModelFieldDebugLineItem::GetDisplayValue() const
{
	const FProperty* Property = PropertyPtr.Get();
	if (Property != nullptr && ValuePtr != nullptr)
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
							MakeShared<FMDViewModelFieldDebugLineItem>(ChildProp, ChildValuePtr, ChildProp->GetDisplayNameText(), ChildProp->GetToolTipText()));
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
							MakeShared<FMDViewModelFieldDebugLineItem>(ArrayProp->Inner, ChildValuePtr, ElementDisplayName, Prop->GetToolTipText()));
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
							MakeShared<FMDViewModelFieldDebugLineItem>(SetProp->ElementProp, ChildValuePtr, ElementDisplayName, Prop->GetToolTipText()));
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
							MakeShared<FMDViewModelFieldDebugLineItem>(MapProp->KeyProp, ChildKeyPtr, KeyDisplayName, Prop->GetToolTipText()));
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
							MakeShared<FMDViewModelFieldDebugLineItem>(MapProp->ValueProp, ChildValuePtr, ValueDisplayName, Prop->GetToolTipText()));
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

int32 FMDViewModelFieldDebugLineItem::GetShouldDisplayFieldNotifyIndex() const
{
	return (ValuePtr == nullptr && bIsFieldNotify) ? 0 : 1;
}

FReply FMDViewModelFieldDebugLineItem::OnAddOrViewBoundFunctionClicked() const
{
	check(bIsFieldNotify);

	if (PropertyPtr.IsValid())
	{
		FMDViewModelGraphModule::OnViewModelFieldNotifyRequestedForBlueprint(WidgetBP.Get(), PropertyPtr->GetFName(), ViewModelClass, ViewModelName);
	}

	return FReply::Handled();
}

int32 FMDViewModelFieldDebugLineItem::GetAddOrViewBoundFunctionIndex() const
{
	check(bIsFieldNotify);

	return (!PropertyPtr.IsValid() || FMDViewModelGraphModule::DoesBlueprintBindToViewModelFieldNotify(WidgetBP.Get(), PropertyPtr->GetFName(), ViewModelClass, ViewModelName))
		? 0 : 1;
}

TSharedRef<SWidget> FMDViewModelFunctionDebugLineItem::GetNameIcon()
{
	if (const UFunction* Function = FunctionPtr.Get())
	{
		FLinearColor ReturnValueColor = FLinearColor::White;
		FText ToolTipText = Function->GetToolTipText();
		if (const FProperty* ReturnProperty = Function->GetReturnProperty())
		{
			const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
			FEdGraphPinType PinType;
			if (K2Schema->ConvertPropertyToPinType(ReturnProperty, PinType))
			{
				ReturnValueColor = K2Schema->GetPinTypeColor(PinType);
			}

			ToolTipText = UEdGraphSchema_K2::TypeToText(ReturnProperty);
		}

		return SNew(SImage)
			.Image(FAppStyle::Get().GetBrush(TEXT("GraphEditor.Function_16x")))
			.ColorAndOpacity(ReturnValueColor)
			.ToolTipText(ToolTipText);
	}

	return FDebugLineItem::GetNameIcon();
}

TSharedRef<SWidget> FMDViewModelFunctionDebugLineItem::GenerateValueWidget(TSharedPtr<FString> InSearchString)
{
	return SNew(SWidgetSwitcher)
	.WidgetIndex(this, &FMDViewModelFunctionDebugLineItem::GetShouldDisplayFieldNotifyIndex)
	+SWidgetSwitcher::Slot()
	[
		SNew(SButton)
		.ContentPadding(FMargin(3.0, 2.0))
		.OnClicked(this, &FMDViewModelFunctionDebugLineItem::OnAddOrViewBoundFieldNotifyFunctionClicked)
		[
			SNew(SWidgetSwitcher)
			.WidgetIndex(this, &FMDViewModelFunctionDebugLineItem::GetAddOrViewBoundFieldNotifyFunctionIndex)
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
				.ToolTipText(INVTEXT("Create a BP event bound to this view model field notify function"))
			]
		]
	]
	+SWidgetSwitcher::Slot()
	[
		SNew(STextBlock)
		.Text(this, &FMDViewModelFunctionDebugLineItem::GetDescription)
		.ToolTipText(this, &FMDViewModelFunctionDebugLineItem::GetDescription)
	];
}

void FMDViewModelFunctionDebugLineItem::UpdateIsDebugging(bool InIsDebugging)
{
	bIsDebugging = InIsDebugging;
}

void FMDViewModelFunctionDebugLineItem::UpdateCachedChildren() const
{
	CachedChildren = TArray<FDebugTreeItemPtr>();

	if (const UFunction* Function = FunctionPtr.Get())
	{
		for (TFieldIterator<const FProperty> It(Function); It; ++It)
		{
			if (const FProperty* ParamProp = *It)
			{
				if (!CachedPropertyItems.Contains(ParamProp->GetFName()))
				{
					CachedPropertyItems.Add(ParamProp->GetFName(),
						MakeShared<FMDViewModelFieldDebugLineItem>(ParamProp, nullptr, ParamProp->GetDisplayNameText(), ParamProp->GetToolTipText()));
				}
			}
		}
	}

	CachedPropertyItems.GenerateValueArray(CachedChildren.GetValue());
}

int32 FMDViewModelFunctionDebugLineItem::GetShouldDisplayFieldNotifyIndex() const
{
	return (!bIsDebugging && bIsFieldNotify) ? 0 : 1;
}

FReply FMDViewModelFunctionDebugLineItem::OnAddOrViewBoundFieldNotifyFunctionClicked() const
{
	check(bIsFieldNotify);

	if (FunctionPtr.IsValid())
	{
		FMDViewModelGraphModule::OnViewModelFieldNotifyRequestedForBlueprint(WidgetBP.Get(), FunctionPtr->GetFName(), ViewModelClass, ViewModelName);
	}

	return FReply::Handled();
}

int32 FMDViewModelFunctionDebugLineItem::GetAddOrViewBoundFieldNotifyFunctionIndex() const
{
	check(bIsFieldNotify);

	return (!FunctionPtr.IsValid() || FMDViewModelGraphModule::DoesBlueprintBindToViewModelFieldNotify(WidgetBP.Get(), FunctionPtr->GetFName(), ViewModelClass, ViewModelName))
		? 0 : 1;
}

TSharedRef<SWidget> FMDViewModelEventDebugLineItem::GenerateValueWidget(TSharedPtr<FString> InSearchString)
{
	return SNew(SButton)
	.ContentPadding(FMargin(3.0, 2.0))
	.OnClicked(this, &FMDViewModelEventDebugLineItem::OnAddOrViewBoundFunctionClicked)
	[
		SNew(SWidgetSwitcher)
		.WidgetIndex(this, &FMDViewModelEventDebugLineItem::GetAddOrViewBoundFunctionIndex)
		+ SWidgetSwitcher::Slot()
		[
			SNew(SImage)
			.ColorAndOpacity(FSlateColor::UseForeground())
			.Image(FAppStyle::Get().GetBrush("Icons.SelectInViewport"))
			.ToolTipText(INVTEXT("Focus the existing bound function."))
		]
		+ SWidgetSwitcher::Slot()
		[
			SNew(SImage)
			.ColorAndOpacity(FSlateColor::UseForeground())
			.Image(FAppStyle::Get().GetBrush("Icons.Plus"))
			.ToolTipText(INVTEXT("Create a BP event bound to this view model event"))
		]
	];
}

FReply FMDViewModelEventDebugLineItem::OnAddOrViewBoundFunctionClicked() const
{
	if (WeakDelegateProp.IsValid())
	{
		FMDViewModelGraphModule::OnViewModelEventRequestedForBlueprint(WidgetBP.Get(), WeakDelegateProp->GetFName(), ViewModelClass, ViewModelName);
	}

	return FReply::Handled();
}

int32 FMDViewModelEventDebugLineItem::GetAddOrViewBoundFunctionIndex() const
{
	return (!WeakDelegateProp.IsValid() || FMDViewModelGraphModule::DoesBlueprintBindToViewModelEvent(WidgetBP.Get(), WeakDelegateProp->GetFName(), ViewModelClass, ViewModelName))
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
					Item = MakeShared<FMDViewModelFieldDebugLineItem>(Prop, ValuePtr, Prop->GetDisplayNameText(), Prop->GetToolTipText(), bIsFieldNotify, WidgetBPPtr.Get(), ViewModelClass, ViewModelName);
				}
				else
				{
					Item->UpdateViewModelName(ViewModelName);
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
						Item = MakeShared<FMDViewModelEventDebugLineItem>(DelegateProp, false, WidgetBPPtr.Get(), ViewModelClass, ViewModelName);
					}
					else
					{
						Item->UpdateViewModelName(ViewModelName);
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

			const bool bIsFieldNotify = FieldNotifySupportedNames.Contains(Func->GetFName());
			const bool bShouldAddFunction = [&]()
			{
				if (bIncludeFieldNotifyFunctions && bIsFieldNotify)
				{
					return true;
				}

				const bool bIsPure = Func->HasAnyFunctionFlags(FUNC_BlueprintPure);
				if (bIncludeBlueprintPure && bIsPure)
				{
					return true;
				}

				if (!bIsPure && bIncludeBlueprintCallable && Func->HasAnyFunctionFlags(FUNC_BlueprintCallable))
				{
					return true;
				}

				return false;
			}();

			if (bShouldAddFunction)
			{
				TSharedPtr<FMDViewModelFunctionDebugLineItem>& Item = FunctionTreeItems.FindOrAdd(Func);
				if (!Item.IsValid())
				{
					Item = MakeShared<FMDViewModelFunctionDebugLineItem>(Func, Func->GetDisplayNameText(), Func->GetToolTipText(), bIsFieldNotify, WidgetBPPtr.Get(), ViewModelClass, ViewModelName);
				}
				else
				{
					Item->UpdateViewModelName(ViewModelName);
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
