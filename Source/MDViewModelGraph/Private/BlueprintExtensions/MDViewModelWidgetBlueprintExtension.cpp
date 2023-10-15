#include "BlueprintExtensions/MDViewModelWidgetBlueprintExtension.h"

#include "Runtime/Launch/Resources/Version.h"
#include "Util/MDViewModelEditorAssignment.h"
#include "WidgetExtensions/MDViewModelWidgetClassExtension.h"


void UMDViewModelWidgetBlueprintExtension::PostLoad()
{
	Super::PostLoad();

	for (FMDViewModelEditorAssignment& Assignment : Assignments)
	{
		Assignment.Assignment.UpdateViewModelClassName();
	}
}

void UMDViewModelWidgetBlueprintExtension::HandleBeginCompilation(FWidgetBlueprintCompilerContext& InCreationContext)
{
	Super::HandleBeginCompilation(InCreationContext);

	CompilerContext = &InCreationContext;
}

void UMDViewModelWidgetBlueprintExtension::HandleFinishCompilingClass(UWidgetBlueprintGeneratedClass* Class)
{
	Super::HandleFinishCompilingClass(Class);

	TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> CompiledAssignments;
	GetAllAssignments(CompiledAssignments);
	if (CompilerContext != nullptr && !CompiledAssignments.IsEmpty())
	{
		CompiledAssignments.Reserve(Assignments.Num());
		for (const FMDViewModelEditorAssignment& Assignment : Assignments)
		{
			CompiledAssignments.Add(Assignment.Assignment, Assignment.Data);
		}

		// Apply parent assignments last so they overwrite any collisions in this BP
		TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> ParentAssignments;
		GetParentAssignments(ParentAssignments);
		CompiledAssignments.Append(ParentAssignments);

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
