#pragma once

#include "CoreMinimal.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SCompoundWidget.h"

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
	void OnClassListSelectionChanged(TSharedPtr<FMDViewModelClassItem> Item, ESelectInfo::Type SelectInfo);

	static void OpenAssignmentDialog(UMDViewModelWidgetBlueprintExtension* BPExtension);
	static void OpenEditDialog(UMDViewModelWidgetBlueprintExtension* BPExtension, TSharedPtr<FMDViewModelEditorAssignment> EditorItem);

protected:
	FReply OnAddClicked() const;
	FReply OnSaveClicked() const;

	bool bIsEditMode = false;

	TSharedPtr<FMDViewModelEditorAssignment> EditorItem;

	TSharedPtr<SWindow> ParentWindow;
	TStrongObjectPtr<UMDViewModelAssignmentEditorObject> EditorObject;
	TSharedPtr<FMDViewModelClassItem> SelectedViewModelItem;
	TWeakObjectPtr<UMDViewModelWidgetBlueprintExtension> BPExtensionPtr;

};
