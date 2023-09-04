#pragma once

#include "Templates/SubclassOf.h"
#include "Modules/ModuleManager.h"

class FBlueprintEditor;
class FLayoutExtender;
class FMDViewModelGraphPanelPinFactory;
class FWidgetBlueprintApplicationMode;
class FWorkflowAllowedTabSet;
class UBlueprint;
class UMDViewModelBase;
class UMDViewModelBlueprintCompilerExtension;

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

	// Use a weak ptr even though we add it to root, it can be destroyed before the module shuts down
	TWeakObjectPtr<UMDViewModelBlueprintCompilerExtension> CompilerExtensionPtr = nullptr;
	
	TSharedPtr<FMDViewModelGraphPanelPinFactory> ViewModelGraphPanelPinFactory;
};
