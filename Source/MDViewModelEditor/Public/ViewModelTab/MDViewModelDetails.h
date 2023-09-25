#pragma once

#include "Styling/SlateBrush.h"
#include "Templates/SubclassOf.h"
#include "Util/MDViewModelAssignmentReference.h"
#include "Widgets/SCompoundWidget.h"

class FBlueprintEditor;
class UMDViewModelBase;
class SMDViewModelFieldInspector;

/**
 *
 */
class MDVIEWMODELEDITOR_API SMDViewModelDetails : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMDViewModelDetails)
		{}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedPtr<FBlueprintEditor>& BlueprintEditor);

	void UpdateViewModel(const FMDViewModelAssignmentReference& Assignment, bool bIsDebugging, UMDViewModelBase* DebugViewModel);

private:
	TSharedPtr<SMDViewModelFieldInspector> PropertyInspector;
	TSharedPtr<SMDViewModelFieldInspector> EventInspector;
	TSharedPtr<SMDViewModelFieldInspector> CommandInspector;
	TSharedPtr<SMDViewModelFieldInspector> HelperInspector;
};
