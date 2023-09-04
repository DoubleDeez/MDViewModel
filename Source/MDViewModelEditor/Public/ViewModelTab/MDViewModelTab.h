#pragma once

#include "WidgetDrawerConfig.h"
#include "WorkflowOrientedApp/WorkflowTabFactory.h"

class SMDViewModelEditor;
class FBlueprintEditor;

struct FMDViewModelSummoner : public FWorkflowTabFactory
{
	static const FName TabID;
	static const FName DrawerID;

	FMDViewModelSummoner(const TSharedPtr<FBlueprintEditor>& BlueprintEditor, bool bIsDrawer);

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;
	TSharedRef<SWidget> GetOrCreateViewModelEditor() const;

	static TSharedRef<SDockTab> StaticSpawnTab(const FSpawnTabArgs& Args, TWeakPtr<FBlueprintEditor> WeakBlueprintEditor);
	static FWidgetDrawerConfig CreateDrawerConfig(const TSharedRef<FBlueprintEditor>& BlueprintEditor);

private:
	TWeakPtr<FBlueprintEditor> WeakBlueprintEditor;
	const bool bIsDrawer;

	mutable TSharedPtr<SMDViewModelEditor> ViewModelEditor;
};

