#pragma once

#include "BlueprintCompilerExtension.h"
#include "MDViewModelBlueprintCompilerExtension.generated.h"

class UWidgetBlueprint;

/**
 * Blueprint Compiler Extension to inject View Model assignments when compiling Blueprints
 * Widget Blueprints: MDViewModelClassExtensions when compiling widgets with super classes that have view model assignments
 * Actor Blueprints: TODO
 */
UCLASS()
class MDVIEWMODELGRAPH_API UMDViewModelBlueprintCompilerExtension : public UBlueprintCompilerExtension
{
	GENERATED_BODY()

public:
	virtual void ProcessBlueprintCompiled(const FKismetCompilerContext& CompilationContext, const FBlueprintCompiledData& Data) override;

private:
	void HandleWidgetBlueprintCompiled(const UWidgetBlueprint& WidgetBP, const FKismetCompilerContext& CompilationContext) const;
	void HandleActorBlueprintCompiled(UBlueprint& Blueprint, const FKismetCompilerContext& CompilationContext) const;
};
