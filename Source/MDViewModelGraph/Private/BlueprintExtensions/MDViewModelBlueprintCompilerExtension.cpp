#include "BlueprintExtensions/MDViewModelBlueprintCompilerExtension.h"

#include "Util/MDViewModelUtils.h"
#include "WidgetBlueprint.h"
#include "BlueprintExtensions/MDViewModelWidgetBlueprintExtension.h"
#include "WidgetExtensions/MDViewModelWidgetClassExtension.h"

void UMDViewModelBlueprintCompilerExtension::ProcessBlueprintCompiled(const FKismetCompilerContext& CompilationContext, const FBlueprintCompiledData& Data)
{
	Super::ProcessBlueprintCompiled(CompilationContext, Data);

	UBlueprint* Blueprint = CompilationContext.Blueprint;
	const UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(Blueprint);
	if (IsValid(WidgetBP))
	{
		HandleWidgetBlueprintCompiled(*WidgetBP, CompilationContext);
	}
	else if (IsValid(Blueprint) && IsValid(Blueprint->GeneratedClass) && Blueprint->GeneratedClass->IsChildOf<AActor>())
	{
		HandleActorBlueprintCompiled(*Blueprint, CompilationContext);
	}
}

void UMDViewModelBlueprintCompilerExtension::HandleWidgetBlueprintCompiled(const UWidgetBlueprint& WidgetBP, const FKismetCompilerContext& CompilationContext) const
{
	const bool bAlreadyHasExtension = WidgetBP.GetExtensions().ContainsByPredicate([](TObjectPtr<UBlueprintExtension> Extension)
	{
		if (const auto* VMExtension = Cast<IMDViewModelAssignableInterface>(Extension))
		{
			return !VMExtension->GetAssignments().IsEmpty();
		}

		return false;
	});

	if (!bAlreadyHasExtension)
	{
		if (UWidgetBlueprintGeneratedClass* WidgetClass = Cast<UWidgetBlueprintGeneratedClass>(CompilationContext.NewClass))
		{
			if (MDViewModelUtils::DoesClassOrSuperClassHaveAssignments(WidgetClass->GetSuperClass()))
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

void UMDViewModelBlueprintCompilerExtension::HandleActorBlueprintCompiled(UBlueprint& Blueprint, const FKismetCompilerContext& CompilationContext) const
{
	const bool bAlreadyHasExtension = Blueprint.GetExtensions().ContainsByPredicate([](TObjectPtr<UBlueprintExtension> Extension)
	{
		if (const auto* VMExtension = Cast<IMDViewModelAssignableInterface>(Extension))
		{
			return VMExtension->HasAssignments();
		}

		return false;
	});

	if (!bAlreadyHasExtension)
	{
		// TODO - Check parent for assignments and inject a component/remove component
	}
}
