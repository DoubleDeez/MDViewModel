#include "ViewModelTab/MDViewModelEditor.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "BlueprintEditor.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Nodes/MDVMNode_CallFunctionBase.h"
#include "Nodes/MDVMNode_DynamicBindingBase.h"
#include "Nodes/MDVMNode_GetProperty.h"
#include "Nodes/MDVMNode_GetViewModel.h"
#include "Nodes/MDVMNode_SetProperty.h"
#include "Nodes/MDVMNode_SetViewModel.h"
#include "Nodes/MDVMNode_SetViewModelOfClass.h"
#include "Util/MDViewModelEditorAssignment.h"
#include "Util/MDViewModelFunctionLibrary.h"
#include "ViewModel/MDViewModelBase.h"
#include "ViewModelTab/MDViewModelDetails.h"
#include "ViewModelTab/MDViewModelList.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SSplitter.h"


SMDViewModelEditor::~SMDViewModelEditor()
{
	if (const FAssetRegistryModule* AssetRegistryModulePtr = FModuleManager::GetModulePtr<FAssetRegistryModule>(AssetRegistryConstants::ModuleName))
	{
		if (IAssetRegistry* AssetRegistryPtr = AssetRegistryModulePtr->TryGet())
		{
			AssetRegistryPtr->OnAssetRemoved().RemoveAll(this);
			AssetRegistryPtr->OnAssetRenamed().RemoveAll(this);
		}
	}

	FBlueprintEditorUtils::OnRenameVariableReferencesEvent.RemoveAll(this);
}

void SMDViewModelEditor::Construct(const FArguments& InArgs, const TSharedPtr<FBlueprintEditor>& BlueprintEditor)
{
	UBlueprint* Blueprint = BlueprintEditor->GetBlueprintObj();
	if (!ensureMsgf(IsValid(Blueprint), TEXT("Attempting to edit an invalid blueprint")))
	{
		return;
	}

	EditedBlueprintPtr = Blueprint;
	Blueprint->OnSetObjectBeingDebugged().AddSP(this, &SMDViewModelEditor::OnSetObjectBeingDebugged);

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName).Get();
	AssetRegistry.OnAssetRemoved().AddSP(this, &SMDViewModelEditor::OnAssetRemoved);
	AssetRegistry.OnAssetRenamed().AddSP(this, &SMDViewModelEditor::OnAssetRenamed);

	FBlueprintEditorUtils::OnRenameVariableReferencesEvent.AddSP(this, &SMDViewModelEditor::OnRenameVariable);

	TArray<UBlueprint*> Hierarchy;
	UBlueprint::GetBlueprintHierarchyFromClass(Blueprint->GeneratedClass, Hierarchy);
	for (UBlueprint* BP : Hierarchy)
	{
		BP->OnCompiled().AddSP(this, &SMDViewModelEditor::OnBlueprintCompiled);
	}

	ChildSlot
	[
		SNew(SSplitter)
		.Orientation(Orient_Horizontal)
		+SSplitter::Slot()
		.MinSize(300.f)
		.Value(0.15f)
		[
			SAssignNew(ViewModelListWidget, SMDViewModelList, BlueprintEditor)
			.OnViewModelSelected(this, &SMDViewModelEditor::OnViewModelSelected)
		]
		+SSplitter::Slot()
		[
			SAssignNew(ViewModelDetailsWidget, SMDViewModelDetails, BlueprintEditor)
		]
	];

	OnSetObjectBeingDebugged(Blueprint->GetObjectBeingDebugged());
}

void SMDViewModelEditor::PostUndo(bool bSuccess)
{
	RefreshEditor(false);
}

void SMDViewModelEditor::PostRedo(bool bSuccess)
{
	RefreshEditor(false);
}

void SMDViewModelEditor::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	// If the debug object we're looking at gets GCd, the BP won't fire its delegate so we need to check manually
	if (bIsDebugging && !ObjectBeingDebugged.IsValid())
	{
		OnSetObjectBeingDebugged(nullptr);
	}
}

void SMDViewModelEditor::OnSetObjectBeingDebugged(UObject* Object)
{
	if (IMDViewModelRuntimeInterface* VMRuntime = MDViewModelUtils::GetViewModelRuntimeInterface(ObjectBeingDebugged.Get()))
	{
		VMRuntime->StopListeningForAnyViewModelChanged(this);
	}

	ObjectBeingDebugged = Object;

	bIsDebugging = ObjectBeingDebugged.IsValid();
	SetCanTick(bIsDebugging);

	if (IMDViewModelRuntimeInterface* VMRuntime = MDViewModelUtils::GetViewModelRuntimeInterface(ObjectBeingDebugged.Get()))
	{
		VMRuntime->ListenForAnyViewModelChanged(FSimpleDelegate::CreateSP(this, &SMDViewModelEditor::OnViewModelChanged));
	}

	OnViewModelChanged();
}

void SMDViewModelEditor::OnViewModelSelected(FMDViewModelEditorAssignment* Assignment)
{
	if (Assignment != nullptr)
	{
		SelectedAssignment = FMDViewModelAssignmentReference(Assignment->Assignment);
	}
	else
	{
		SelectedAssignment = {};
	}

	OnViewModelChanged();
}

void SMDViewModelEditor::OnViewModelChanged()
{
	if (ViewModelDetailsWidget.IsValid())
	{
		bool bIsValid = false;
		UMDViewModelBase* DebugViewModel = UMDViewModelFunctionLibrary::BP_GetViewModel(ObjectBeingDebugged.Get(), SelectedAssignment, bIsValid);

		ViewModelDetailsWidget->UpdateViewModel(SelectedAssignment, ObjectBeingDebugged.IsValid(), DebugViewModel);
	}
}

void SMDViewModelEditor::OnBlueprintCompiled(UBlueprint* BP)
{
	RefreshEditor(true);
}

void SMDViewModelEditor::OnAssetRemoved(const FAssetData& AssetData)
{
	RefreshEditor(true);
}

void SMDViewModelEditor::OnAssetRenamed(const FAssetData& AssetData, const FString& OldName)
{
	RefreshEditor(true);
}

void SMDViewModelEditor::OnRenameVariable(UBlueprint* Blueprint, UClass* VariableClass, const FName& OldVariableName, const FName& NewVariableName)
{
	if (IsValid(VariableClass) && EditedBlueprintPtr.IsValid() && EditedBlueprintPtr.Get() != Blueprint && VariableClass->IsChildOf<UMDViewModelBase>())
	{
		TArray<UEdGraph*> AllGraphs;
		EditedBlueprintPtr->GetAllGraphs(AllGraphs);

		auto IsMDVMNode = [](const UEdGraphNode* Node)
		{
			static TSet<UClass*> NodeClasses = {
				UMDVMNode_DynamicBindingBase::StaticClass(),
				UMDVMNode_CallFunctionBase::StaticClass(),
				UMDVMNode_GetProperty::StaticClass(),
				UMDVMNode_SetProperty::StaticClass(),
				UMDVMNode_GetViewModel::StaticClass(),
				UMDVMNode_SetViewModel::StaticClass(),
				UMDVMNode_SetViewModelOfClass::StaticClass()
			};

			for (const UClass* Class : NodeClasses)
			{
				if (IsValid(Node) && Node->GetClass()->IsChildOf(Class))
				{
					return true;
				}
			}

			return false;
		};

		for (UEdGraph* CurrentGraph : AllGraphs)
		{
			if (IsValid(CurrentGraph))
			{
				for (UEdGraphNode* Node : CurrentGraph->Nodes)
				{
					if (IsMDVMNode(Node))
					{
						Cast<UK2Node>(Node)->HandleVariableRenamed(Blueprint, VariableClass, CurrentGraph, OldVariableName, NewVariableName);
					}
				}
			}
		}

		RefreshEditor(true);
	}
}

void SMDViewModelEditor::RefreshEditor(bool bRefreshDetails)
{
	if (ViewModelListWidget.IsValid())
	{
		ViewModelListWidget->RefreshList();
	}

	if (bRefreshDetails)
	{
		OnViewModelChanged();
	}
}
