#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "Modules/ModuleManager.h"

class UBlueprint;
class UMDViewModelBase;
class UMDViewModelBlueprintCompilerExtension;
class FWorkflowAllowedTabSet;
class FWidgetBlueprintApplicationMode;

class FMDViewModelEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	// Delegates as a dirty workaround for circular dependency issues
	DECLARE_DELEGATE_RetVal_FourParams(bool, FDoesBlueprintBindToViewModelFieldNotify, const UBlueprint*, const FName&, TSubclassOf<UMDViewModelBase>, const FName&);
	FDoesBlueprintBindToViewModelFieldNotify DoesBlueprintBindToViewModelFieldNotify;
	DECLARE_DELEGATE_FourParams(FOnViewModelFieldNotifyRequestedForBlueprint, const UBlueprint*, const FName&, TSubclassOf<UMDViewModelBase>, const FName&);
	FOnViewModelFieldNotifyRequestedForBlueprint OnViewModelFieldNotifyRequestedForBlueprint;
	DECLARE_DELEGATE_RetVal_FourParams(bool, FDoesBlueprintBindToViewModelEvent, const UBlueprint*, const FName&, TSubclassOf<UMDViewModelBase>, const FName&);
	FDoesBlueprintBindToViewModelEvent DoesBlueprintBindToViewModelEvent;
	DECLARE_DELEGATE_FourParams(FOnViewModelEventRequestedForBlueprint, const UBlueprint*, const FName&, TSubclassOf<UMDViewModelBase>, const FName&);
	FOnViewModelEventRequestedForBlueprint OnViewModelEventRequestedForBlueprint;

private:
	void HandleRegisterBlueprintEditorTab(const FWidgetBlueprintApplicationMode& ApplicationMode, FWorkflowAllowedTabSet& TabFactories);

	void HandleActivateMode(FWidgetBlueprintApplicationMode& InDesignerMode);

	void HandleDeactivateMode(FWidgetBlueprintApplicationMode& InDesignerMode);

	UMDViewModelBlueprintCompilerExtension* CompilerExtension = nullptr;
};
