#include "BlueprintExtensions/MDViewModelActorBlueprintExtension.h"

#include "Components/MDViewModelAssignmentComponent.h"
#include "Engine/InheritableComponentHandler.h"
#include "Engine/SCS_Node.h"
#include "KismetCompiler.h"

namespace MDVMABPE
{
	static const FName ComponentName = TEXT("MDViewModelAssignments");
	
	UMDViewModelAssignmentComponent* GetAssignmentComponent(UBlueprint* Blueprint, UBlueprintGeneratedClass* BPClass)
	{
		TArray<UBlueprintGeneratedClass*> Hierarchy;
		UBlueprint::GetBlueprintHierarchyFromClass(BPClass, Hierarchy);
		
		// Find the most authoritative assignment component (our nearest parent)
		USCS_Node* ComponentNode = nullptr;
		for (UBlueprintGeneratedClass* Class : Hierarchy)
		{
			USCS_Node* CurrentComponentNode = Class->SimpleConstructionScript->FindSCSNode(ComponentName);
			if (Class != BPClass && CurrentComponentNode != nullptr)
			{
				constexpr bool bCreateIfNecessary = true;
				UInheritableComponentHandler* ICH = Blueprint->GetInheritableComponentHandler(bCreateIfNecessary);

				// TODO - CreateOverridenComponentTemplate fails during compilation :/ (Crash) - try moving this all to UMDViewModelBlueprintCompilerExtension::HandleActorBlueprintCompiled
				const FComponentKey Key(CurrentComponentNode);
				
				auto* Component = Cast<UMDViewModelAssignmentComponent>(ICH->CreateOverridenComponentTemplate(Key));

				// Clean up any pre-existing components, which can happen if we had assignments before our parents did
				const TArray<USCS_Node*> AllNodes = BPClass->SimpleConstructionScript->GetAllNodes();
				for (USCS_Node* Node : AllNodes)
				{
					if (Node->ComponentTemplate->IsA<UMDViewModelAssignmentComponent>())
					{
						BPClass->SimpleConstructionScript->RemoveNode(Node);
					}
				}
				
				return Component;
			}
			else if (Class == BPClass)
			{
				ComponentNode = CurrentComponentNode;
			}
		}

		// We don't have any parent assignments, use our existing component or create one if needed
		if (!IsValid(ComponentNode))
		{
			ComponentNode = Blueprint->SimpleConstructionScript->CreateNode(UMDViewModelAssignmentComponent::StaticClass(), ComponentName);
			ComponentNode->ComponentTemplate->CreationMethod = EComponentCreationMethod::Native;
			Blueprint->SimpleConstructionScript->AddNode(ComponentNode);
		}

		return Cast<UMDViewModelAssignmentComponent>(ComponentNode->ComponentTemplate);
	}
}
