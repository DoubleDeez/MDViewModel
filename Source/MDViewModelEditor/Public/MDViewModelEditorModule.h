#pragma once

#include "Templates/SubclassOf.h"
#include "Modules/ModuleManager.h"

class SMDVMConfigEditor;
class FBlueprintEditor;
class FLayoutExtender;
class FMDViewModelGraphPanelPinFactory;
class FWidgetBlueprintApplicationMode;
class FWorkflowAllowedTabSet;
class UBlueprint;
class UMDViewModelBase;

class FMDViewModelEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void HandleRegisterBlueprintEditorTab(const FWidgetBlueprintApplicationMode& InApplicationMode, FWorkflowAllowedTabSet& TabFactories);

	void HandleActivateMode(FWidgetBlueprintApplicationMode& InApplicationMode);

	void HandleDeactivateMode(FWidgetBlueprintApplicationMode& InApplicationMode);
	
	void RegisterBlueprintEditorLayout(FLayoutExtender& Extender);
	void RegisterBlueprintEditorTab(FWorkflowAllowedTabSet& TabFactories, FName InModeName, TSharedPtr<FBlueprintEditor> BlueprintEditor);
	void RegisterBlueprintEditorDrawer(UObject* Asset);
	
	TSharedPtr<FMDViewModelGraphPanelPinFactory> ViewModelGraphPanelPinFactory;
	TSharedPtr<SMDVMConfigEditor> ViewModelConfigEditor;
};
