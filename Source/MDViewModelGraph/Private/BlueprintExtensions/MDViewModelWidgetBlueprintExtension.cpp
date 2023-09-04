#include "BlueprintExtensions/MDViewModelWidgetBlueprintExtension.h"

#include "Launch/Resources/Version.h"
#include "Util/MDViewModelEditorAssignment.h"
#include "WidgetExtensions/MDViewModelWidgetClassExtension.h"


void UMDViewModelWidgetBlueprintExtension::HandleBeginCompilation(FWidgetBlueprintCompilerContext& InCreationContext)
{
	Super::HandleBeginCompilation(InCreationContext);

	CompilerContext = &InCreationContext;
}

void UMDViewModelWidgetBlueprintExtension::HandleFinishCompilingClass(UWidgetBlueprintGeneratedClass* Class)
{
	Super::HandleFinishCompilingClass(Class);

	if (CompilerContext != nullptr && !Assignments.IsEmpty())
	{
		// TODO - Consider whether the parent assignments should be copied into the class extension here (see UMDViewModelWidgetClassExtension::GetThisAndAncestorAssignments)
		TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> CompiledAssignments;
		CompiledAssignments.Reserve(Assignments.Num());
		for (const FMDViewModelEditorAssignment& Assignment : Assignments)
		{
			CompiledAssignments.Add(Assignment.Assignment, Assignment.Data);
		}

		UMDViewModelWidgetClassExtension* ClassExtension = NewObject<UMDViewModelWidgetClassExtension>(Class);
		ClassExtension->SetAssignments(CompiledAssignments);

		CompilerContext->AddExtension(Class, ClassExtension);
	}
}

void UMDViewModelWidgetBlueprintExtension::HandleEndCompilation()
{
	Super::HandleEndCompilation();

	CompilerContext = nullptr;
}

void UMDViewModelWidgetBlueprintExtension::SearchParentAssignments(TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& OutViewModelAssignments, TSubclassOf<UMDViewModelBase> ViewModelClass, const FGameplayTag& ProviderTag, const FName& ViewModelName) const
{
	UClass* SuperClass = GetWidgetBlueprint()->SkeletonGeneratedClass->GetSuperClass();
	if (UWidgetBlueprintGeneratedClass* WBGC = Cast<UWidgetBlueprintGeneratedClass>(SuperClass))
	{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 2
		if (UMDViewModelWidgetClassExtension* Extension = WBGC->GetExtension<UMDViewModelWidgetClassExtension>())
#else
		if (UMDViewModelWidgetClassExtension* Extension = WBGC->GetExtension<UMDViewModelWidgetClassExtension>(false))
#endif
		{
			Extension->SearchAssignments(OutViewModelAssignments, ViewModelClass, ProviderTag, ViewModelName);
		}
	}
}
