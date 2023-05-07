#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "Templates/SubclassOf.h"
#include "Widgets/SCompoundWidget.h"

class UMDViewModelBase;
class SMDViewModelFieldInspector;
/**
 *
 */
class MDVIEWMODELEDITOR_API SMDViewModelDetails : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMDViewModelDetails)
		{
		}

		SLATE_ARGUMENT(TSubclassOf<UMDViewModelBase>, ViewModelClass)
		SLATE_ARGUMENT(UMDViewModelBase*, DebugViewModel)

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	void UpdateViewModel(TSubclassOf<UMDViewModelBase> ViewModelClass, UMDViewModelBase* DebugViewModel);

private:
	TSharedPtr<SMDViewModelFieldInspector> PropertyInspector;
	TSharedPtr<SMDViewModelFieldInspector> EventInspector;
	TSharedPtr<SMDViewModelFieldInspector> CommandInspector;
};
