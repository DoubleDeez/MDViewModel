#pragma once

#include "EditorUndoClient.h"
#include "Templates/SubclassOf.h"
#include "Util/MDViewModelAssignmentReference.h"
#include "Widgets/SCompoundWidget.h"

class FBlueprintEditor;
struct FMDViewModelEditorAssignment;
class SMDViewModelDetails;
class SMDViewModelList;
class UMDViewModelBase;

/**
 *
 */
class MDVIEWMODELEDITOR_API SMDViewModelEditor : public SCompoundWidget, public FSelfRegisteringEditorUndoClient
{
public:
	SLATE_BEGIN_ARGS(SMDViewModelEditor)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedPtr<FBlueprintEditor>& BlueprintEditor);

	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	void OnSetObjectBeingDebugged(UObject* Object);
	void OnViewModelSelected(FMDViewModelEditorAssignment* Assignment);

	void OnViewModelChanged();
	void OnBlueprintCompiled(UBlueprint* BP);

	bool bIsDebugging = false;

	TWeakObjectPtr<UObject> ObjectBeingDebugged;
	FMDViewModelAssignmentReference SelectedAssignment;

	TSharedPtr<SMDViewModelList> ViewModelListWidget;
	TSharedPtr<SMDViewModelDetails> ViewModelDetailsWidget;
};
