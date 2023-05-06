// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "Widgets/SCompoundWidget.h"

class UMDViewModelBase;
struct FMDViewModelEditorAssignment;
class SMDViewModelDetails;
class FWidgetBlueprintEditor;

/**
 *
 */
class MDVIEWMODELEDITOR_API SMDViewModelEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMDViewModelEditor)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<FWidgetBlueprintEditor> BlueprintEditor);

private:
	void OnSetObjectBeingDebugged(UObject* Object);
	void OnViewModelSelected(FMDViewModelEditorAssignment* Assignment);

	EVisibility GetViewModelDetailsVisibility() const;

	void OnViewModelChanged();

	TWeakObjectPtr<UObject> ObjectBeingDebugged;
	TSubclassOf<UMDViewModelBase> SelectedViewModelClass;
	FName SelectedViewModelName = NAME_None;

	TSharedPtr<SMDViewModelDetails> ViewModelDetailsWidget;
};
