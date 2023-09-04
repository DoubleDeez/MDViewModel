#include "BlueprintExtensions/MDViewModelActorBlueprintExtension.h"

#include "Components/MDViewModelAssignmentComponent.h"
#include "Engine/SCS_Node.h"
#include "KismetCompiler.h"

void UMDViewModelActorBlueprintExtension::HandleGenerateFunctionGraphs(FKismetCompilerContext* CompilerContext)
{
	if ( CompilerContext == nullptr || !IsValid(CompilerContext->Blueprint))
	{
		return;
	}
	
	static const FName ComponentName = TEXT("MDViewModelAssignments");
	UBlueprint* Blueprint = CompilerContext->Blueprint;
	if (IsValid(Blueprint->SimpleConstructionScript))
	{
		USCS_Node* ComponentNode = Blueprint->SimpleConstructionScript->FindSCSNode(ComponentName);
		if (HasAssignments())
		{
			if (!IsValid(ComponentNode))
			{
				ComponentNode = Blueprint->SimpleConstructionScript->CreateNode(UMDViewModelAssignmentComponent::StaticClass(), ComponentName);
				ComponentNode->ComponentTemplate->CreationMethod = EComponentCreationMethod::Native;
				Blueprint->SimpleConstructionScript->AddNode(ComponentNode);
			}

			if (auto* Component = Cast<UMDViewModelAssignmentComponent>(ComponentNode->ComponentTemplate))
			{
				TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> CompiledAssignments;
				GetAllAssignments(CompiledAssignments);
				
				Component->SetAssignments(CompiledAssignments);
			}
		}
		else if (IsValid(ComponentNode))
		{
			Blueprint->SimpleConstructionScript->RemoveNode(ComponentNode);
		}
	}
}

void UMDViewModelActorBlueprintExtension::SearchParentAssignments(TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& OutViewModelAssignments, TSubclassOf<UMDViewModelBase> ViewModelClass, const FGameplayTag& ProviderTag, const FName& ViewModelName) const
{
	// TODO - Actor parent assignments
}
