#pragma once

#include "UObject/StrongObjectPtr.h"
#include "UObject/WeakInterfacePtr.h"
#include "ViewModelTab/MDViewModelAssignmentEditorObject.h"
#include "Widgets/SCompoundWidget.h"

class IMDViewModelAssignableInterface;
class UBlueprint;
struct FMDViewModelEditorAssignment;
struct FMDViewModelClassItem;

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
		SLATE_ARGUMENT(UBlueprint*, Blueprint)
		SLATE_ARGUMENT(TSharedPtr<FMDViewModelEditorAssignment>, EditorItem)

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs, const TSharedRef<SWindow>& InParentWindow);

	UBlueprint* GetBlueprint() const;

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnAssignmentAdded, const FMDViewModelEditorAssignment&);
	FOnAssignmentAdded OnAssignmentAdded;

	static void OpenAssignmentDialog(UBlueprint* Blueprint);
	static void OpenEditDialog(UBlueprint* Blueprint, TSharedPtr<FMDViewModelEditorAssignment> EditorItem);
	static void OpenDuplicateDialog(UBlueprint* Blueprint, TSharedPtr<FMDViewModelEditorAssignment> EditorItem);

	static TSharedPtr<SMDViewModelAssignmentDialog> GetActiveDialog();

private:
	static void OpenDialog_Internal(UBlueprint* Blueprint, TSharedPtr<FMDViewModelEditorAssignment> EditorItem = nullptr, bool bDuplicateItem = false);
	static void OnDialogClosed(const TSharedRef<SWindow>& Window);
	static TWeakPtr<SWindow> ActiveDialogWindowPtr;
	
	EVisibility GetAddVisibility() const;
	EVisibility GetSaveVisibility() const;
	EVisibility GetAssignmentErrorVisibility() const;

	FReply OnAddClicked() const;
	FReply OnSaveClicked() const;

	bool DoesAssignmentHaveError() const;
	FText GetAssignmentError() const;
	
	IMDViewModelAssignableInterface* GetExtension(bool bCreateIfNecessary = false) const; 

	EMDVMDialogMode Mode = EMDVMDialogMode::Add;

	TSharedPtr<FMDViewModelEditorAssignment> EditorItem;

	TSharedPtr<SWindow> ParentWindow;
	TStrongObjectPtr<UMDViewModelAssignmentEditorObject> EditorObject;
	TWeakObjectPtr<UBlueprint> BlueprintPtr;
	TOptional<FName> OriginalAssignmentName;
};
