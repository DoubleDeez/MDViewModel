#include "WidgetExtensions/MDViewModelBlueprintCompilerExtension.h"

#include "MDViewModelModule.h"
#include "WidgetBlueprint.h"
#include "WidgetExtensions/MDViewModelWidgetBlueprintExtension.h"
#include "WidgetExtensions/MDViewModelWidgetClassExtension.h"

void UMDViewModelBlueprintCompilerExtension::ProcessBlueprintCompiled(const FKismetCompilerContext& CompilationContext, const FBlueprintCompiledData& Data)
{
	Super::ProcessBlueprintCompiled(CompilationContext, Data);

	if (const UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(CompilationContext.Blueprint))
	{
		const bool bAlreadyHasExtension = WidgetBP->GetExtensions().ContainsByPredicate([](TObjectPtr<UBlueprintExtension> Extension)
		{
			if (const auto* VMExtension = Cast<UMDViewModelWidgetBlueprintExtension>(Extension))
			{
				return VMExtension->HasAssignments();
			}

			return false;
		});

		if (!bAlreadyHasExtension)
		{
			if (UWidgetBlueprintGeneratedClass* WidgetClass = Cast<UWidgetBlueprintGeneratedClass>(CompilationContext.NewClass))
			{
				if (FMDViewModelModule::DoesClassOrSuperClassHaveAssignments(WidgetClass->GetSuperClass()))
				{
					// We can't add a blueprint extension since we're mid-compile here
					// So instead we want to add a Class extension, but we need a non-const FWidgetBlueprintCompilerContext to do that
					// which is why we end up with this gross-ness
					FWidgetBlueprintCompilerContext& WidgetCompilationContext = const_cast<FWidgetBlueprintCompilerContext&>(
						static_cast<const FWidgetBlueprintCompilerContext&>(CompilationContext)
					);

					UMDViewModelWidgetClassExtension* ClassExtension = NewObject<UMDViewModelWidgetClassExtension>(WidgetClass);
					WidgetCompilationContext.AddExtension(WidgetClass, ClassExtension);
				}
			}
		}
	}
}
