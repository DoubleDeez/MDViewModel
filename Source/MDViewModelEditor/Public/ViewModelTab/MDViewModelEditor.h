// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "EditorUndoClient.h"
#include "Templates/SubclassOf.h"
#include "Widgets/SCompoundWidget.h"

class SMDViewModelList;
class UMDViewModelBase;
struct FMDViewModelEditorAssignment;
class SMDViewModelDetails;
class FBlueprintEditor;

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

private:
	void OnSetObjectBeingDebugged(UObject* Object);
	void OnViewModelSelected(FMDViewModelEditorAssignment* Assignment);

	void OnViewModelChanged();
	void OnBlueprintCompiled(UBlueprint* BP);

	TWeakObjectPtr<UObject> ObjectBeingDebugged;
	TSubclassOf<UMDViewModelBase> SelectedViewModelClass;
	FName SelectedViewModelName = NAME_None;

	TSharedPtr<SMDViewModelList> ViewModelListWidget;
	TSharedPtr<SMDViewModelDetails> ViewModelDetailsWidget;
};
