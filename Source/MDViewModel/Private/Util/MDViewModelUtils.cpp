#include "Util/MDViewModelUtils.h"

#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Components/MDViewModelAssignmentComponent.h"
#include "Engine/Engine.h"
#include "Engine/InheritableComponentHandler.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Runtime/Launch/Resources/Version.h"
#include "UObject/Class.h"
#include "UObject/Object.h"
#include "ViewModelProviders/MDViewModelProviderBase.h"
#include "WidgetExtensions/MDViewModelWidgetClassExtension.h"

namespace MDViewModelUtils
{
	const FName DefaultViewModelName = TEXT("Default");

	UMDViewModelProviderBase* FindViewModelProvider(const FGameplayTag& ProviderTag)
	{
		if (GEngine != nullptr)
		{
			UMDViewModelProviderBase* const* ProviderPtr = GEngine->GetEngineSubsystemArray<UMDViewModelProviderBase>().FindByPredicate([&ProviderTag](const UMDViewModelProviderBase* Provider)
			{
				return IsValid(Provider) && Provider->GetProviderTag() == ProviderTag;
			});

			if (ProviderPtr != nullptr)
			{
				return *ProviderPtr;
			}
		}

		return nullptr;
	}

	void GetViewModelAssignments(UClass* ObjectClass, TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& OutViewModelAssignments)
	{
		// Searching without a filter is equivalent to just getting the assignments
		SearchViewModelAssignments(ObjectClass, OutViewModelAssignments);
	}

	void SearchViewModelAssignments(UClass* ObjectClass, TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& OutViewModelAssignments, TSubclassOf<UMDViewModelBase> ViewModelClass, const FGameplayTag& ProviderTag, const FName& ViewModelName)
	{
		if (const IMDVMCompiledAssignmentsInterface* CompiledAssignments = GetCompiledAssignmentsInterface(ObjectClass))
		{
			CompiledAssignments->SearchAssignments(OutViewModelAssignments, ViewModelClass, ProviderTag, ViewModelName);
		}
	}

	bool HasViewModelAssignments(UClass* ObjectClass)
	{
		TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> ViewModelAssignments;
		GetViewModelAssignments(ObjectClass, ViewModelAssignments);
		return !ViewModelAssignments.IsEmpty();
	}

	IMDVMCompiledAssignmentsInterface* GetCompiledAssignmentsInterface(UClass* ObjectClass)
	{
		if (UWidgetBlueprintGeneratedClass* WBPGC = Cast<UWidgetBlueprintGeneratedClass>(ObjectClass))
		{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 2
			return WBPGC->GetExtension<UMDViewModelWidgetClassExtension>();
#else
			return WBPGC->GetExtension<UMDViewModelWidgetClassExtension>(false);
#endif
		}
		else
		{
			auto FindPred = [](const UActorComponent* ComponentTemplate)
			{
				return IsValid(ComponentTemplate) && ComponentTemplate->Implements<UMDVMCompiledAssignmentsInterface>();
			};
			
			// Actors can have assignments inherited from their parent without having their own UMDVMCompiledAssignmentsInterface so we have to climb the hierarchy
			while (UBlueprintGeneratedClass* BPGC = Cast<UBlueprintGeneratedClass>(ObjectClass))
			{
				if (const TObjectPtr<UActorComponent>* ComponentPtr = BPGC->ComponentTemplates.FindByPredicate(FindPred))
				{
					return Cast<IMDVMCompiledAssignmentsInterface>(*ComponentPtr);
				}

				const USCS_Node* const* NodePtr = BPGC->SimpleConstructionScript->GetAllNodes().FindByPredicate([&FindPred](const USCS_Node* Node)
				{
					return IsValid(Node) && FindPred(Node->ComponentTemplate);
				});

				if (NodePtr != nullptr)
				{
					return Cast<IMDVMCompiledAssignmentsInterface>((*NodePtr)->ComponentTemplate);
				}

				const UInheritableComponentHandler* IHC = BPGC->GetInheritableComponentHandler();
				if (IsValid(IHC))
				{
					TArray<UActorComponent*> ComponentTemplates;
					IHC->GetAllTemplates(ComponentTemplates);
					if (UActorComponent* const* ComponentPtr = ComponentTemplates.FindByPredicate(FindPred))
					{
						return Cast<IMDVMCompiledAssignmentsInterface>(*ComponentPtr);
					}
				}
				
				ObjectClass = BPGC->GetSuperClass();
			}
		}

		return nullptr;
	}
}
