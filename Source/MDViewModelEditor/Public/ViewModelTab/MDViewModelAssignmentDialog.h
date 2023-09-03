#pragma once

#include "UObject/StrongObjectPtr.h"
#include "Widgets/SCompoundWidget.h"

class UBlueprint;
struct FMDViewModelEditorAssignment;
class UMDViewModelWidgetBlueprintExtension;
struct FMDViewModelClassItem;
class UMDViewModelAssignmentEditorObject;

enum class EMDVMDialogMode : uint8
{
	Add,
	Edit,
	Duplicate
};

/**
 *
 */
class MDVIEWMODELEDITOR_API SMDViewModelAssignmentDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMDViewModelAssignmentDialog)
		: _Mode(EMDVMDialogMode::Add)
		{
		}

		SLATE_ARGUMENT(EMDVMDialogMode, Mode)
		SLATE_ARGUMENT(TWeakObjectPtr<UMDViewModelWidgetBlueprintExtension>, BPExtensionPtr)
		SLATE_ARGUMENT(TSharedPtr<FMDViewModelEditorAssignment>, EditorItem)

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs, const TSharedRef<SWindow>& InParentWindow);

	UBlueprint* GetBlueprint() const;

	static void OpenAssignmentDialog(UMDViewModelWidgetBlueprintExtension* BPExtension);
	static void OpenEditDialog(UMDViewModelWidgetBlueprintExtension* BPExtension, TSharedPtr<FMDViewModelEditorAssignment> EditorItem);
	static void OpenDuplicateDialog(UMDViewModelWidgetBlueprintExtension* BPExtension, TSharedPtr<FMDViewModelEditorAssignment> EditorItem);

private:
	static void OpenDialog_Internal(UMDViewModelWidgetBlueprintExtension* BPExtension, TSharedPtr<FMDViewModelEditorAssignment> EditorItem = nullptr, bool bDuplicateItem = false);
	static void OnDialogClosed(const TSharedRef<SWindow>& Window);
	static TWeakPtr<SWindow> ActiveDialogWindowPtr;
	
	EVisibility GetAddVisibility() const;
	EVisibility GetSaveVisibility() const;
	EVisibility GetAssignmentErrorVisibility() const;

	FReply OnAddClicked() const;
	FReply OnSaveClicked() const;

	bool DoesAssignmentHaveError() const;
	FText GetAssignmentError() const;

	EMDVMDialogMode Mode = EMDVMDialogMode::Add;

	TSharedPtr<FMDViewModelEditorAssignment> EditorItem;

	TSharedPtr<SWindow> ParentWindow;
	TStrongObjectPtr<UMDViewModelAssignmentEditorObject> EditorObject;
	TWeakObjectPtr<UMDViewModelWidgetBlueprintExtension> BPExtensionPtr;
	TOptional<FName> OriginalAssignmentName;
};
