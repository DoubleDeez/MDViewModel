#pragma once

#include "Templates/SubclassOf.h"
#include "Modules/ModuleManager.h"

class UBlueprint;
class UMDViewModelBase;
class UMDViewModelBlueprintCompilerExtension;
class FMDViewModelGraphPanelPinFactory;
class FWorkflowAllowedTabSet;
class FWidgetBlueprintApplicationMode;

class FMDViewModelEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void HandleRegisterBlueprintEditorTab(const FWidgetBlueprintApplicationMode& ApplicationMode, FWorkflowAllowedTabSet& TabFactories);

	void HandleActivateMode(FWidgetBlueprintApplicationMode& InDesignerMode);

	void HandleDeactivateMode(FWidgetBlueprintApplicationMode& InDesignerMode);

	// Use a weak ptr even though we add it to root, it can be destroyed before the module shuts down
	TWeakObjectPtr<UMDViewModelBlueprintCompilerExtension> CompilerExtensionPtr = nullptr;
	
	TSharedPtr<FMDViewModelGraphPanelPinFactory> ViewModelGraphPanelPinFactory;
};
