#include "ViewModelTab/FieldInspector/MDViewModelDebugLineItemBase.h"

#include "BlueprintEditor.h"
#include "BlueprintModes/WidgetBlueprintApplicationModes.h"
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "HAL/FileManager.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "PropertyInfoViewStyle.h"
#include "SourceCodeNavigation.h"
#include "ViewModel/MDViewModelBase.h"
#include "ViewModelTab/FieldInspector/DragAndDrop/MDVMInspectorDragAndDropActionBase.h"
#include "WidgetBlueprint.h"

FMDViewModelDebugLineItemBase::FMDViewModelDebugLineItemBase(const FText& DisplayName, const FText& Description, TWeakObjectPtr<UMDViewModelBase> DebugViewModel, const TWeakPtr<FBlueprintEditor>& BlueprintEditorPtr, bool bIsFieldNotify, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& ViewModelName)
	: FDebugLineItem(DLT_Watch)
	, DisplayName(DisplayName)
	, Description(Description)
	, bIsFieldNotify(bIsFieldNotify)
	, BlueprintEditorPtr(BlueprintEditorPtr)
	, BlueprintPtr(BlueprintEditorPtr.IsValid() ? BlueprintEditorPtr.Pin()->GetBlueprintObj() : nullptr)
	, ViewModelClass(ViewModelClass)
	, DebugViewModel(DebugViewModel)
	, ViewModelName(ViewModelName)
{
}

void FMDViewModelDebugLineItemBase::UpdateViewModel(const FName& InViewModelName, TSubclassOf<UMDViewModelBase> InViewModelClass)
{
	ViewModelName = InViewModelName;
	ViewModelClass = InViewModelClass;
}

TSharedRef<FMDVMInspectorDragAndDropActionBase> FMDViewModelDebugLineItemBase::CreateDragAndDropAction() const
{
	return {};
}

FMDViewModelAssignmentReference FMDViewModelDebugLineItemBase::GetViewModelAssignmentReference() const
{
	FMDViewModelAssignmentReference Reference;
	Reference.ViewModelClass = ViewModelClass;
	Reference.ViewModelName = ViewModelName;
	return Reference;
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
{const FFieldVariant Field = GetFieldForDefinitionNavigation();
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
		else
		{
			// TODO - Find actual Function Graph/Event Node
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
		else
		{
			FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(Prop->GetOwnerUObject());
		}
	}
}
