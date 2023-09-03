#pragma once


#include "WorkflowOrientedApp/WorkflowTabFactory.h"

class FBlueprintEditor;

struct FMDViewModelSummoner : public FWorkflowTabFactory
{
	static const FName TabID;
	static const FName DrawerID;

	FMDViewModelSummoner(const TSharedPtr<FBlueprintEditor>& BlueprintEditor);

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;

private:
	TWeakPtr<FBlueprintEditor> WeakBlueprintEditor;
};

