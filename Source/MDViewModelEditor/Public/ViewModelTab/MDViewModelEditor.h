// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorUndoClient.h"
#include "Templates/SubclassOf.h"
#include "Widgets/SCompoundWidget.h"

class SMDViewModelList;
class UMDViewModelBase;
struct FMDViewModelEditorAssignment;
class SMDViewModelDetails;
class FWidgetBlueprintEditor;

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

	void Construct(const FArguments& InArgs, TSharedPtr<FWidgetBlueprintEditor> BlueprintEditor);

	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;

private:
	void OnSetObjectBeingDebugged(UObject* Object);
	void OnViewModelSelected(FMDViewModelEditorAssignment* Assignment);

	void OnViewModelChanged();

	TWeakObjectPtr<UObject> ObjectBeingDebugged;
	TSubclassOf<UMDViewModelBase> SelectedViewModelClass;
	FName SelectedViewModelName = NAME_None;

	TSharedPtr<SMDViewModelList> ViewModelListWidget;
	TSharedPtr<SMDViewModelDetails> ViewModelDetailsWidget;
};
