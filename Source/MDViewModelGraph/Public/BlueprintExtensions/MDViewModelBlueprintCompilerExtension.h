#pragma once

#include "BlueprintCompilerExtension.h"
#include "MDViewModelBlueprintCompilerExtension.generated.h"

class IMDViewModelAssignableInterface;
class UWidgetBlueprint;

/**
 * Blueprint Compiler Extension to inject View Model assignments when compiling Blueprints
 * Widget Blueprints: Injects MDViewModelClassExtensions when compiling widgets with super classes that have view model assignments
 * Actor Blueprints: Adds a UMDViewModelAssignmentComponent to the Actor if the parent doesn't have one, and adds the view model assignments to it
 */
UCLASS()
class MDVIEWMODELGRAPH_API UMDViewModelBlueprintCompilerExtension : public UBlueprintCompilerExtension
{
	GENERATED_BODY()

public:
	virtual void ProcessBlueprintCompiled(const FKismetCompilerContext& CompilationContext, const FBlueprintCompiledData& Data) override;

	void HandleActorBlueprintPreCompile(IMDViewModelAssignableInterface* Extension, UBlueprintGeneratedClass* BPClass) const;

private:
	void HandleWidgetBlueprintCompiled(IMDViewModelAssignableInterface* Extension, UWidgetBlueprint& WidgetBP, const FKismetCompilerContext& CompilationContext) const;
	void HandleActorBlueprintCompiled(IMDViewModelAssignableInterface* Extension, UBlueprint& Blueprint, const FKismetCompilerContext& CompilationContext) const;
};
