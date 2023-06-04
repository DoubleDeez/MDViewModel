#pragma once


#include "WorkflowOrientedApp/WorkflowTabFactory.h"

class FWidgetBlueprintEditor;

struct FMDViewModelSummoner : public FWorkflowTabFactory
{
	static const FName TabID;
	static const FName DrawerID;

	FMDViewModelSummoner(TSharedPtr<FWidgetBlueprintEditor> BlueprintEditor);

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;

private:
	TWeakPtr<FWidgetBlueprintEditor> WeakWidgetBlueprintEditor;
};

