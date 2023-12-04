#include "ViewModelTab/FieldInspector/MDViewModelDebugLineItemBase.h"

#include "BlueprintEditor.h"
#include "BlueprintModes/WidgetBlueprintApplicationModes.h"
#include "EdGraph/EdGraph.h"
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "HAL/FileManager.h"
#include "Kismet2/KismetDebugUtilities.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "PropertyInfoViewStyle.h"
#include "SourceCodeNavigation.h"
#include "ViewModelTab/FieldInspector/DragAndDrop/MDVMDragAndDropWrapperButton.h"
#include "ViewModelTab/FieldInspector/DragAndDrop/MDVMInspectorDragAndDropActionBase.h"
#include "WidgetBlueprint.h"

FMDViewModelDebugLineItemBase::FMDViewModelDebugLineItemBase(const TWeakPtr<FBlueprintEditor>& BlueprintEditorPtr, const FMDViewModelAssignmentReference& Assignment)
	: FDebugLineItem(DLT_Watch)
	, BlueprintEditorPtr(BlueprintEditorPtr)
	, Assignment(Assignment)
	, BlueprintPtr(BlueprintEditorPtr.IsValid() ? BlueprintEditorPtr.Pin()->GetBlueprintObj() : nullptr)
{
}

void FMDViewModelDebugLineItemBase::UpdateViewModel(const FMDViewModelAssignmentReference& InAssignment)
{
	Assignment = InAssignment;
}

void FMDViewModelDebugLineItemBase::UpdateDebugging(bool InIsDebugging, TWeakObjectPtr<UMDViewModelBase> InDebugViewModel)
{
	const bool bDidChange = bIsDebugging != InIsDebugging || DebugViewModel != InDebugViewModel;
	bIsDebugging = InIsDebugging;
	DebugViewModel = InDebugViewModel;

	if (bDidChange)
	{
		OnDebuggingChanged();
	}
}

void FMDViewModelDebugLineItemBase::SetDisplayText(const FText& Name, const FText& Desc)
{
	DisplayName = Name;
	Description = Desc;
}

TSharedRef<FMDVMInspectorDragAndDropActionBase> FMDViewModelDebugLineItemBase::CreateDragAndDropAction() const
{
	return {};
}

bool FMDViewModelDebugLineItemBase::CanDrag() const
{
	return false;
}

bool FMDViewModelDebugLineItemBase::HasChildren() const
{
	if (!CachedChildren.IsSet())
	{
		UpdateCachedChildren();
	}

	return CachedChildren.GetValue().Num() > 0;
}

void FMDViewModelDebugLineItemBase::ExtendContextMenu(FMenuBuilder& MenuBuilder, bool bInDebuggerTab)
{
	if (!GenerateSearchString().IsEmpty())
	{
		MenuBuilder.AddMenuEntry(
			INVTEXT("Find References"),
			INVTEXT("Search for references to this field in this blueprint."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Find")),
			FUIAction(
				FExecuteAction::CreateSP(this, &FMDViewModelDebugLineItemBase::OnFindReferencesClicked)
			)
		);
	}

	if (GetFieldForDefinitionNavigation().IsValid())
	{
		MenuBuilder.AddMenuEntry(
			INVTEXT("Go To Definition"),
			INVTEXT("Open the C++ file or Blueprint where this item is defined."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.ArrowRight")),
			FUIAction(
				FExecuteAction::CreateSP(this, &FMDViewModelDebugLineItemBase::NavigateToDefinitionField)
			)
		);
	}
}

void FMDViewModelDebugLineItemBase::GatherChildrenBase(TArray<FDebugTreeItemPtr>& OutChildren, const FString& InSearchString, bool bRespectSearch)
{
	if (CanHaveChildren())
	{
		if (!CachedChildren.IsSet())
		{
			UpdateCachedChildren();
		}

		OutChildren.Append(CachedChildren.GetValue());
	}
}

TSharedRef<SWidget> FMDViewModelDebugLineItemBase::GenerateNameWidget(TSharedPtr<FString> InSearchString)
{
	return SNew(SMDVMDragAndDropWrapperButton, StaticCastSharedRef<FMDViewModelDebugLineItemBase>(AsShared()))
		.bCanDrag(this, &FMDViewModelDebugLineItemBase::CanDrag)
		[
			SNew(PropertyInfoViewStyle::STextHighlightOverlay)
			.FullText(this, &FMDViewModelDebugLineItemBase::GetDisplayName)
			.HighlightText(this, &FDebugLineItem::GetHighlightText, InSearchString)
			[
				SNew(STextBlock)
					.ToolTipText(this, &FDebugLineItem::GetDescription)
					.Text(this, &FMDViewModelDebugLineItemBase::GetDisplayName)
			]
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

bool FMDViewModelDebugLineItemBase::CanCreateNodes() const
{
	return !GEditor->bIsSimulatingInEditor && GEditor->PlayWorld == nullptr;
}

void FMDViewModelDebugLineItemBase::OnFindReferencesClicked() const
{
	if (const TSharedPtr<FBlueprintEditor> BPEditor = BlueprintEditorPtr.Pin())
	{
		// Widget BP's have the find window in graph mode
		if (IsValid(Cast<UWidgetBlueprint>(BPEditor->GetBlueprintObj())))
		{
			BPEditor->SetCurrentMode(FWidgetBlueprintApplicationModes::GraphMode);
		}

		BPEditor->SummonSearchUI(true, GenerateSearchString());
	}
}

void FMDViewModelDebugLineItemBase::NavigateToDefinitionField() const
{
	const FFieldVariant Field = GetFieldForDefinitionNavigation();
	if (const UFunction* Func = Cast<UFunction>(Field.ToUObject()))
	{
		if (Func->IsNative())
		{
			// Try navigating directly to the line it's declared on
			if (FSourceCodeNavigation::CanNavigateToFunction(Func) && FSourceCodeNavigation::NavigateToFunction(Func))
			{
				return;
			}

			// Failing that, fall back to the older method which will still get the file open assuming it exists
			FString NativeParentClassHeaderPath;
			if (FSourceCodeNavigation::FindClassHeaderPath(Func, NativeParentClassHeaderPath) && (IFileManager::Get().FileSize(*NativeParentClassHeaderPath) != INDEX_NONE))
			{
				const FString AbsNativeParentClassHeaderPath = FPaths::ConvertRelativePathToFull(NativeParentClassHeaderPath);
				FSourceCodeNavigation::OpenSourceFile(AbsNativeParentClassHeaderPath);
			}
		}
		else if (UBlueprint* BP = Func->GetTypedOuter<UBlueprint>())
		{
			TObjectPtr<UEdGraph>* FuncGraphPtr = BP->FunctionGraphs.FindByPredicate([Func](const UEdGraph* Graph)
			{
				return IsValid(Graph) && Graph->GetFName() == Func->GetFName();
			});

			if (FuncGraphPtr == nullptr || !IsValid(*FuncGraphPtr))
			{
				FuncGraphPtr = BP->EventGraphs.FindByPredicate([Func](const UEdGraph* Graph)
				{
					return IsValid(Graph) && Graph->GetFName() == Func->GetFName();
				});
			}

			if (FuncGraphPtr != nullptr && IsValid(*FuncGraphPtr))
			{
				FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(*FuncGraphPtr);
			}
		}
		else
		{
			FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(Func);
		}
	}
	else if (const FProperty* Prop = CastField<const FProperty>(Field.ToField()))
	{
		if (Prop->IsNative())
		{
			// Try navigating directly to the line it's declared on
			if (FSourceCodeNavigation::CanNavigateToProperty(Prop) && FSourceCodeNavigation::NavigateToProperty(Prop))
			{
				return;
			}

			// Failing that, fall back to the older method which will still get the file open assuming it exists
			FString NativeParentClassHeaderPath;
			if (FSourceCodeNavigation::FindClassHeaderPath(Prop->GetOwnerUField(), NativeParentClassHeaderPath) && (IFileManager::Get().FileSize(*NativeParentClassHeaderPath) != INDEX_NONE))
			{
				const FString AbsNativeParentClassHeaderPath = FPaths::ConvertRelativePathToFull(NativeParentClassHeaderPath);
				FSourceCodeNavigation::OpenSourceFile(AbsNativeParentClassHeaderPath);
			}
		}
		else if (const UBlueprint* BP = Func->GetTypedOuter<UBlueprint>())
		{
			FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(BP);
		}
		else
		{
			FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(Prop->GetOwnerUObject());
		}
	}
}

FText FMDViewModelDebugLineItemBase::GeneratePropertyDisplayValue(const FProperty* Property, void* ValuePtr) const
{
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
