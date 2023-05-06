#include "ViewModelTab/MDViewModelFieldInspector.h"

#include "BlueprintEditor.h"
#include "EdGraphSchema_K2.h"
#include "Kismet2/KismetDebugUtilities.h"
#include "UObject/WeakFieldPtr.h"
#include "ViewModel/MDViewModelBase.h"


bool FMDViewModelFieldDebugLineItem::HasChildren() const
{
	if (!CachedChildren.IsSet())
	{
		UpdateCachedChildren();
	}

	return CachedChildren.GetValue().Num() > 0;
}

void FMDViewModelFieldDebugLineItem::GatherChildrenBase(TArray<FDebugTreeItemPtr>& OutChildren, const FString& InSearchString, bool bRespectSearch)
{
	if (!CachedChildren.IsSet())
	{
		UpdateCachedChildren();
	}

	OutChildren.Append(CachedChildren.GetValue());
}

TSharedRef<SWidget> FMDViewModelFieldDebugLineItem::GetNameIcon()
{
	if (const FProperty* ItemProperty = PropertyPtr.Get())
	{
		FSlateColor BaseColor;
		FSlateColor UnusedColor;
		FSlateBrush const* UnusedIcon = nullptr;
		const FSlateBrush* IconBrush = FBlueprintEditor::GetVarIconAndColorFromProperty(
			ItemProperty,
			BaseColor,
			UnusedIcon,
			UnusedColor
		);

		return SNew(SImage)
			.Image(IconBrush)
			.ColorAndOpacity(BaseColor)
			.ToolTipText(UEdGraphSchema_K2::TypeToText(ItemProperty));
	}

	return FDebugLineItem::GetNameIcon();
}

TSharedRef<SWidget> FMDViewModelFieldDebugLineItem::GenerateValueWidget(TSharedPtr<FString> InSearchString)
{
	return SNew(STextBlock)
		.Text(this, &FMDViewModelFieldDebugLineItem::GetDisplayValue)
		.ToolTipText(this, &FMDViewModelFieldDebugLineItem::GetDisplayValue);
}

FText FMDViewModelFieldDebugLineItem::GetDisplayValue() const
{
	const TTuple<const FProperty*, void*> PropertyInstance = GetPropertyInstance();
	if (PropertyInstance.Key != nullptr && PropertyInstance.Value != nullptr)
	{
		if (PropertyInstance.Key->IsA<FObjectProperty>() || PropertyInstance.Key->IsA<FInterfaceProperty>())
		{
			const UObject* Object = nullptr;
			if (const FObjectPropertyBase* ObjectPropertyBase = CastField<FObjectPropertyBase>(PropertyInstance.Key))
			{
				Object = ObjectPropertyBase->GetObjectPropertyValue(PropertyInstance.Value);
			}
			else if (PropertyInstance.Key->IsA<FInterfaceProperty>())
			{
				const FScriptInterface* InterfaceData = static_cast<const FScriptInterface*>(PropertyInstance.Value);
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
		else if (PropertyInstance.Key->IsA<FStructProperty>())
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
			FKismetDebugUtilities::GetDebugInfoInternal(DebugInfo, PropertyInstance.Key, PropertyInstance.Value);
			return DebugInfo->Value;
		}
	}

	return INVTEXT("[No Value]");
}

void FMDViewModelFieldDebugLineItem::UpdateCachedChildren() const
{
	CachedChildren = TArray<FDebugTreeItemPtr>();

	if (const FProperty* ItemProperty = PropertyPtr.Get())
	{
		const TTuple<const FProperty*, void*> PropertyInstance = GetPropertyInstance();
		const UStruct* PropertyStruct = nullptr;
		void* PropertyValue = nullptr;
		if (const FObjectPropertyBase* ObjectProp = CastField<FObjectPropertyBase>(ItemProperty))
		{
			PropertyStruct = ObjectProp->PropertyClass;
			if (PropertyInstance.Value != nullptr)
			{
				PropertyValue = ObjectProp->GetObjectPropertyValue(PropertyInstance.Value);
			}
		}
		else if (const FStructProperty* StructProp = CastField<FStructProperty>(ItemProperty))
		{
			PropertyStruct = StructProp->Struct;
			PropertyValue = PropertyInstance.Value;
		}
		else if (ItemProperty->IsA<FInterfaceProperty>())
		{
			// Interface property won't have the class so we need the actual value to get it
			if (const FScriptInterface* InterfaceData = static_cast<const FScriptInterface*>(PropertyInstance.Value))
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
						CachedPropertyItems.Add(ChildProp->GetFName(), MakeShared<FMDViewModelFieldDebugLineItem>(ChildProp, ChildValuePtr));
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
		else if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(ItemProperty))
		{
			if (PropertyInstance.Value != nullptr)
			{
				TMap<FName, FDebugTreeItemPtr> CacheCopy = CachedPropertyItems;
				CachedPropertyItems.Reset();

				FScriptArrayHelper Helper = FScriptArrayHelper(ArrayProp, PropertyInstance.Value);
				for (int32 i = 0; i < Helper.Num(); ++i)
				{
					void* ChildValuePtr = Helper.GetRawPtr(i);
					const FText ElementDisplayName = FText::Format(INVTEXT("[{0}]"), FText::AsNumber(i));
					const FName PropertyName = *FString::Printf(TEXT("%d"), i);
					if (!CacheCopy.Contains(PropertyName))
					{
						CachedPropertyItems.Add(PropertyName, MakeShared<FMDViewModelFieldDebugLineItem>(ArrayProp->Inner, ChildValuePtr, ElementDisplayName));
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
		else if (const FSetProperty* SetProp = CastField<FSetProperty>(ItemProperty))
		{
			if (PropertyInstance.Value != nullptr)
			{
				TMap<FName, FDebugTreeItemPtr> CacheCopy = CachedPropertyItems;
				CachedPropertyItems.Reset();

				FScriptSetHelper Helper = FScriptSetHelper(SetProp, PropertyInstance.Value);
				for (int32 i = 0; i < Helper.Num(); ++i)
				{
					void* ChildValuePtr = Helper.GetElementPtr(i);
					const FText ElementDisplayName = FText::Format(INVTEXT("[{0}]"), FText::AsNumber(i));
					const FName PropertyName = *FString::Printf(TEXT("%d"), i);
					if (!CacheCopy.Contains(PropertyName))
					{
						CachedPropertyItems.Add(PropertyName, MakeShared<FMDViewModelFieldDebugLineItem>(SetProp->ElementProp, ChildValuePtr, ElementDisplayName));
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
		else if (const FMapProperty* MapProp = CastField<FMapProperty>(ItemProperty))
		{
			if (PropertyInstance.Value != nullptr)
			{
				TMap<FName, FDebugTreeItemPtr> CacheCopy = CachedPropertyItems;
				CachedPropertyItems.Reset();

				FScriptMapHelper Helper = FScriptMapHelper(MapProp, PropertyInstance.Value);
				for (int32 i = 0; i < Helper.Num(); ++i)
				{
					// Key
					void* ChildKeyPtr = Helper.GetKeyPtr(i);
					const FText KeyDisplayName = FText::Format(INVTEXT("Key[{0}]"), FText::AsNumber(i));
					const FName KeyPropertyName = *FString::Printf(TEXT("Key[%d]"), i);
					if (!CacheCopy.Contains(KeyPropertyName))
					{
						CachedPropertyItems.Add(KeyPropertyName, MakeShared<FMDViewModelFieldDebugLineItem>(MapProp->KeyProp, ChildKeyPtr, KeyDisplayName));
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
						CachedPropertyItems.Add(ValuePropertyName, MakeShared<FMDViewModelFieldDebugLineItem>(MapProp->ValueProp, ChildValuePtr, ValueDisplayName));
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

TTuple<const FProperty*, void*> FMDViewModelFieldDebugLineItem::GetPropertyInstance() const
{
	return { PropertyPtr.Get(), ValuePtr };
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

void SMDViewModelFieldInspector::SetReferences(TSubclassOf<UMDViewModelBase> InViewModelClass, UMDViewModelBase* InDebugViewModel)
{
	ViewModelClass = InViewModelClass;
	DebugViewModel = InDebugViewModel;

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

	UMDViewModelBase* PropertyOuter = bIsDebugging ? DebugViewModel.Get() : ViewModelClass->GetDefaultObject<UMDViewModelBase>();

	for (TFieldIterator<const FProperty> It(ViewModelClass); It; ++It)
	{
		if (const FProperty* Prop = *It)
		{
			if (Prop->HasAnyPropertyFlags(CPF_BlueprintVisible))
			{
				void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(PropertyOuter);
				TSharedPtr<FMDViewModelFieldDebugLineItem>& Item = TreeItems.FindOrAdd(Prop);
				if (!Item.IsValid())
				{
					Item = MakeShared<FMDViewModelFieldDebugLineItem>(Prop, ValuePtr);
				}
				else
				{
					Item->UpdateValuePtr(ValuePtr);
				}

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
