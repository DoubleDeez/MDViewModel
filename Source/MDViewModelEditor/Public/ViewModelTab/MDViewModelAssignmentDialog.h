#pragma once

#include "UObject/StrongObjectPtr.h"
#include "Widgets/SCompoundWidget.h"

class UWidgetBlueprint;
struct FMDViewModelEditorAssignment;
class UMDViewModelWidgetBlueprintExtension;
struct FMDViewModelClassItem;
class UMDViewModelAssignmentEditorObject;

/**
 *
 */
class MDVIEWMODELEDITOR_API SMDViewModelAssignmentDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMDViewModelAssignmentDialog)
		: _bIsEditMode(false)
		{
		}

		SLATE_ARGUMENT(bool, bIsEditMode)
		SLATE_ARGUMENT(TWeakObjectPtr<UMDViewModelWidgetBlueprintExtension>, BPExtensionPtr)
		SLATE_ARGUMENT(TSharedPtr<FMDViewModelEditorAssignment>, EditorItem)

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs, const TSharedRef<SWindow>& InParentWindow);

	UWidgetBlueprint* GetWidgetBlueprint() const;

	static void OpenAssignmentDialog(UMDViewModelWidgetBlueprintExtension* BPExtension);
	static void OpenEditDialog(UMDViewModelWidgetBlueprintExtension* BPExtension, TSharedPtr<FMDViewModelEditorAssignment> EditorItem);

protected:
	EVisibility GetAddVisibility() const;
	EVisibility GetSaveVisibility() const;

	FReply OnAddClicked() const;
	FReply OnSaveClicked() const;

	bool IsAssignmentUnique() const;

	bool bIsEditMode = false;

	TSharedPtr<FMDViewModelEditorAssignment> EditorItem;

	TSharedPtr<SWindow> ParentWindow;
	TStrongObjectPtr<UMDViewModelAssignmentEditorObject> EditorObject;
	TWeakObjectPtr<UMDViewModelWidgetBlueprintExtension> BPExtensionPtr;
	TOptional<FName> OriginalAssignmentName;
};
