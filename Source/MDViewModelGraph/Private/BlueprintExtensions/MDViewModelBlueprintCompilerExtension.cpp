#include "BlueprintExtensions/MDViewModelBlueprintCompilerExtension.h"

#include "BlueprintExtensions/MDViewModelWidgetBlueprintExtension.h"
#include "Components/MDViewModelAssignmentComponent.h"
#include "Engine/SCS_Node.h"
#include "Interfaces/MDViewModelSupportedInterface.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Util/MDViewModelGraphStatics.h"
#include "Util/MDViewModelUtils.h"
#include "WidgetBlueprint.h"
#include "WidgetExtensions/MDViewModelWidgetClassExtension.h"


void UMDViewModelBlueprintCompilerExtension::ProcessBlueprintCompiled(const FKismetCompilerContext& CompilationContext, const FBlueprintCompiledData& Data)
{
	Super::ProcessBlueprintCompiled(CompilationContext, Data);

	UBlueprint* Blueprint = CompilationContext.Blueprint;
	if (IsValid(Blueprint))
	{
		const TObjectPtr<UBlueprintExtension>* ExtensionPtr = Blueprint->GetExtensions().FindByPredicate([](TObjectPtr<UBlueprintExtension> Extension)
		{
			return IsValid(Extension) && Extension->Implements<UMDViewModelAssignableInterface>();
		});

		IMDViewModelAssignableInterface* Extension = (ExtensionPtr != nullptr) ? Cast<IMDViewModelAssignableInterface>(*ExtensionPtr) : nullptr;
		UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(Blueprint);
		if (IsValid(WidgetBP))
		{
			HandleWidgetBlueprintCompiled(Extension, *WidgetBP, CompilationContext);
		}
		else if (IsValid(CompilationContext.NewClass) && CompilationContext.NewClass->IsChildOf<AActor>())
		{
			// TODO - Is this necessary? Can we get by with only FMDViewModelGraphModule::OnBlueprintPreCompile for actors?
			HandleActorBlueprintCompiled(Extension, *Blueprint, CompilationContext);
		}
		else if (IsValid(CompilationContext.NewClass) && CompilationContext.NewClass->ImplementsInterface(UMDViewModelSupportedInterface::StaticClass()))
		{
			// Do nothing for general objects, compilation is handled by FMDViewModelGraphModule::OnBlueprintPreCompile
		}
	}
}

void UMDViewModelBlueprintCompilerExtension::HandleActorBlueprintPreCompile(IMDViewModelAssignableInterface* Extension, UBlueprintGeneratedClass* BPClass) const
{
	UBlueprint* Blueprint = UBlueprint::GetBlueprintFromClass(BPClass);
	if (!IsValid(Blueprint))
	{
		return;
	}

	TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> CompiledAssignments;
	FMDViewModelGraphStatics::GetViewModelAssignmentsForBlueprint(Blueprint, CompiledAssignments);
	if (!CompiledAssignments.IsEmpty())
	{
		if (UMDViewModelAssignmentComponent* Component = FMDViewModelGraphStatics::GetOrCreateAssignmentComponentTemplate(BPClass))
		{
			Component->SetAssignments(CompiledAssignments);
		}
		else if (Extension != nullptr && !Extension->GetAssignments().IsEmpty())
		{
#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 2
			Blueprint->Message_Warn(TEXT("[MDVM] Blueprint @@ could not compile its view model assignments, it may need to be resaved in the editor. This can happen when introducing assignments in a parent blueprint."), Blueprint);
#else
			Blueprint->Message_Warn(FString::Printf(TEXT("[MDVM] Blueprint [%s] could not compile its view model assignments, it may need to be resaved in the editor. This can happen when introducing assignments in a parent blueprint."), *Blueprint->GetName()));
#endif
		}
	}
	else
	{
		if (Extension != nullptr)
		{
			// No assignments, remove the extension
			Blueprint->RemoveExtension(Cast<UBlueprintExtension>(Extension));
		}

		if (IsValid(BPClass->SimpleConstructionScript))
		{
			// No assignments, so remove the component
			const TArray<USCS_Node*> AllNodes = BPClass->SimpleConstructionScript->GetAllNodes();
			for (USCS_Node* Node : AllNodes)
			{
				if (Node->ComponentTemplate->IsA<UMDViewModelAssignmentComponent>())
				{
					BPClass->SimpleConstructionScript->RemoveNode(Node);
				}
			}
		}
	}
}

void UMDViewModelBlueprintCompilerExtension::HandleGeneralBlueprintPreCompile(IMDViewModelAssignableInterface* Extension, UBlueprintGeneratedClass* BPClass) const
{
	UBlueprint* Blueprint = UBlueprint::GetBlueprintFromClass(BPClass);
	IMDVMCompiledAssignmentsInterface* CompiledAssignmentsInterface = MDViewModelUtils::GetCompiledAssignmentsInterface(BPClass);
	if (!IsValid(Blueprint) || CompiledAssignmentsInterface == nullptr)
	{
		return;
	}

	TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> CompiledAssignments;
	FMDViewModelGraphStatics::GetViewModelAssignmentsForBlueprint(Blueprint, CompiledAssignments);

	CompiledAssignmentsInterface->SetAssignments(CompiledAssignments);

	if (CompiledAssignments.IsEmpty() && Extension != nullptr)
	{
		// No assignments, remove the extension
		Blueprint->RemoveExtension(Cast<UBlueprintExtension>(Extension));
	}
}

void UMDViewModelBlueprintCompilerExtension::HandleWidgetBlueprintCompiled(IMDViewModelAssignableInterface* Extension, UWidgetBlueprint& WidgetBP, const FKismetCompilerContext& CompilationContext) const
{
	TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> ViewModelAssignments;
	FMDViewModelGraphStatics::GetViewModelAssignmentsForBlueprint(&WidgetBP, ViewModelAssignments);

	if (ViewModelAssignments.IsEmpty())
	{
		// No assignments, remove the extension (if it exists)
		WidgetBP.RemoveExtension(Cast<UBlueprintExtension>(Extension));
	}
	else
	{
		if (UWidgetBlueprintGeneratedClass* WidgetClass = Cast<UWidgetBlueprintGeneratedClass>(CompilationContext.NewClass))
		{
			// We want to add a Class extension here, but we need a non-const FWidgetBlueprintCompilerContext to do that
			// which is why we end up with this gross-ness
			FWidgetBlueprintCompilerContext& WidgetCompilationContext = const_cast<FWidgetBlueprintCompilerContext&>(
				static_cast<const FWidgetBlueprintCompilerContext&>(CompilationContext)
			);

			UMDViewModelWidgetClassExtension* ClassExtension = NewObject<UMDViewModelWidgetClassExtension>(WidgetClass);
			ClassExtension->SetAssignments(ViewModelAssignments);
			WidgetCompilationContext.AddExtension(WidgetClass, ClassExtension);
		}
	}
}

void UMDViewModelBlueprintCompilerExtension::HandleActorBlueprintCompiled(IMDViewModelAssignableInterface* Extension, UBlueprint& Blueprint, const FKismetCompilerContext& CompilationContext) const
{
	HandleActorBlueprintPreCompile(Extension, CompilationContext.NewClass);
}

void UMDViewModelBlueprintCompilerExtension::HandleGeneralBlueprintCompiled(IMDViewModelAssignableInterface* Extension, UBlueprint& Blueprint, const FKismetCompilerContext& CompilationContext) const
{
	HandleGeneralBlueprintPreCompile(Extension, CompilationContext.NewClass);
}
